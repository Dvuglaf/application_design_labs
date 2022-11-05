#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <random>
#include <thread>
#include <cpr/cpr.h>
#include <iomanip>
#include "bencode.hpp"
#include "torrent_file.h"
#include "socket.h"

std::string hexDecode(const std::string& value) {
	int hashLength = value.length();
	std::string decodedHexString;
	for (int i = 0; i < hashLength; i += 2)
	{
		std::string byte = value.substr(i, 2);
		unsigned char z = (unsigned char)(int)strtol(byte.c_str(), nullptr, 16);
		char c = (char)(int)strtol(byte.c_str(), nullptr, 16);
		decodedHexString.push_back(c);
	}
	return decodedHexString;
}

std::string receive_via_socket(const socket_wrapper& slave_socket) {
	int received_bytes = 0;
	std::string received_buffer;
	try {
		do {
			char received[8192];
			received_bytes = slave_socket.recv(received, 8192);

			for (size_t i = 0; i < received_bytes; ++i)
				received_buffer.push_back(received[i]);

		} while (received_bytes > 0);
	}
	catch (std::exception& e) {
		std::cout << e.what();
	}
	return received_buffer;
}

inline constexpr unsigned char operator "" _u(char arg) noexcept {
	return static_cast<unsigned char>(arg);
}

class wrong_connection : public std::runtime_error {
public: 
	explicit wrong_connection(const std::string& what_arg) : std::runtime_error(what_arg), _msg(what_arg) { ; }
	explicit wrong_connection(const char* what_arg) : std::runtime_error(what_arg), _msg(what_arg) { ; }
	~wrong_connection() override { ; }
	const char* what() const noexcept override {
		return _msg.c_str();
	}
protected:
	std::string _msg;
};

class bit_message {
public:
	enum message_id {
		keep_alive = -1,
		choke = 0,
		unchoke = 1,
		interested = 2,
		not_interested = 3,
		have = 4,
		bit_field = 5,
		request = 6,
		piece = 7,
		cancel = 8,
		port = 9
	};

public:
	bit_message(const message_id id, const std::string& payload = "") : id(id), payload(payload) { ; }

	std::string create_message() {
		u_short message_length = htons(1u);

		std::string buffer;
		buffer += std::to_string(message_length);
		buffer += std::to_string(id);
		buffer += payload;
		return buffer;
	}

	int get_id() const noexcept { return id; }
	std::string get_payload() const noexcept { return payload; }

private:
	int id;
	std::string payload;
};

class peer {
private:
	struct handshake_params {
		std::string info_hash;
		std::string client_id;
	};
public:
	peer(const std::string& id, const std::string& ip, const unsigned short port, socket_wrapper&& socket)
		: id(id), ip(ip), port(port), connect_socket(std::move(socket)) {}

	peer(const std::string& id, const std::string& ip, const unsigned long long port, socket_wrapper&& socket)
		: id(id), ip(ip), port(static_cast<unsigned short>(port)), connect_socket(std::move(socket)) {}

	std::string get_ip() { return ip; }

	std::string perform_handshake(const std::string& handshake_message) {
		connect_socket.connect(address_family::IPV4, port, ip);

		const int bytes_sent_size = connect_socket.send(handshake_message.c_str(),
			static_cast<int>(handshake_message.size()));
		//connect_socket.shutdown(shutdown_type::SEND);
		return receive_via_socket(connect_socket);
	}

	static std::string create_handshake_message(const handshake_params& params) {
		const std::string protocol = "BitTorrent protocol";
		std::stringstream buffer;
		buffer << (char)19;
		buffer << protocol;
		std::string reserved;
		for (size_t i = 0; i < 8; ++i)
			reserved.push_back('\0');
		buffer << reserved;
		buffer << hexDecode(params.info_hash);
		buffer << params.client_id;
		assert(buffer.str().length() == protocol.length() + 49);
		return buffer.str();
	}

	static std::string get_bitfield(const std::string& answer) {
		/*
		 * length(4B) + msg_id(1B) + bitfield(until end)
		 */

		std::vector<bit_message> messages;

		// Get 'bitfield' (id = 5), always exists
		std::string length_str = answer.substr(0, 4);
		unsigned int length = 0;
		for (size_t i = 0; i < 4; ++i) {
			length += int(length_str[3 - i]) * int(std::pow(256, i));
		}

		int message_id = int(answer.substr(4, 1)[0]);
		std::string payload = answer.substr(5, length - 1);

		messages.push_back(bit_message(bit_message::message_id(message_id), payload));

		// Get 'having' messages (id = 4) if exist
		message_id = 4;
		for (size_t i = 4 + length + 5; i < answer.size(); i += 9) {
			payload = answer.substr(i, 4);
			messages.push_back(bit_message(bit_message::message_id(message_id), payload));
		}

		// Form bitfield as string with '0' and '1', i_th symbol of bitfield shows if the peer has i_th piece
		std::string bitfield = messages[0].get_payload();
		std::string transformed_bitfield;

		for (auto c : bitfield) {  // transform only bit_field
			for (int i = 7; i >= 0; --i) {
				transformed_bitfield += std::to_string((int(c) & (1 << i)) >> i);
			}
		}

		for (size_t i = 1; i < messages.size(); ++i) {  // go through 'having' messages and update transformed_bit_field 
			unsigned int index = 0;
			std::string payload = messages[i].get_payload();
			for (size_t j = 0; j < 4; ++j) {
				index += int((payload[3 - j] + 256) % 256) * int(std::pow(256, j));
			}
			transformed_bitfield[index] = '1';
		}

		return transformed_bitfield;
	}

	std::string get_bitfield() { return bitfield; }

	void update_bitfield(const std::string& answer) { 
		if (answer.size() != 0)
			bitfield = get_bitfield(answer); 
		else {
			std::string bitfield_answer = receive_via_socket(connect_socket);
			bitfield = get_bitfield(bitfield_answer);
		}
	}

private:
	std::string id;
	std::string ip;
	unsigned short port;
	std::string bitfield = "";
	socket_wrapper connect_socket;
};

class client {
public:
	client(const std::string& path_to_file) : id(generate_client_id()), file(path_to_file) { ; }

	void start_download() {
		try {
			std::vector<peer> peers = get_peers();
			for (int i = peers.size() - 1; i >= 0; --i) {
				try {
					establish_peer_connection(peers[i]);
					std::cout << "\nPEER_BF: " << peers[i].get_bitfield() << std::endl;
				}
				catch (wrong_connection& e) {
					std::cout << e.what() << std::endl;
					continue;
				}
			}
		}
		catch (std::exception& e) {
			std::cout << e.what();
		}
	}


private:
	struct request_params {
		std::string url;
		std::string client_id;
		std::string info_hash;
		unsigned long long uploaded;
		unsigned long long downloaded;
		unsigned long long left;
		unsigned short port;
	};

private:
	std::string generate_client_id() {
		std::string peer_id = "-UT2022-";

		// Generate 12 random numbers
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> distrib(1, 9);
		for (int i = 0; i < 12; i++)
			peer_id += std::to_string(distrib(gen));
		return peer_id;
	}

	std::string get_peers_request(const request_params& params) {
		cpr::Response res = cpr::Get(cpr::Url{ params.url }, cpr::Parameters{
					{ "info_hash", hexDecode(params.info_hash) },
					{ "peer_id", std::string(params.client_id) },
					{ "port", std::to_string(params.port) },
					{ "uploaded", std::to_string(params.uploaded) },
					{ "downloaded", std::to_string(params.downloaded) },
					{ "left", std::to_string(params.left) },
					{ "compact", std::to_string(1) }
			}, cpr::Timeout{ cpr::Timeout(10000) }
		);
		return res.text;
	}

	std::vector<peer> get_peers() {
		try {
			const auto announce_url = file.get_announce();
			const auto info_hash = file.get_info_hash();
			const auto tracker_response = get_peers_request({ announce_url, id, info_hash,
				0ull, 0ull, 2918443ull, 6881 });  // TODO: generialize
			const auto response_dict = std::get<bencode::dict>(bencode::decode(tracker_response));

			bencode::list peers_list = std::get<bencode::list>(response_dict->find("peers")->second);
			std::vector<peer> peers;
			std::vector<std::string> peer_ips, peer_ids;
			std::vector<unsigned short> peer_ports;

			for (auto& p : peers_list) {
				peers.push_back(peer(
					std::get<bencode::string>(std::get<bencode::dict>(p)->find("peer id")->second),
					std::get<bencode::string>(std::get<bencode::dict>(p)->find("ip")->second),
					std::get<bencode::integer>(std::get<bencode::dict>(p)->find("port")->second),
					socket_wrapper(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP)
				));
			}
			return peers;

		}
		catch (...) {
			throw;
		}
	}

	void check_hash(const std::string& answer) {
		/*
		 * len_protocol(2B) + protocol(19B) + reserved(8B) + info_hash(20B) + peer_id(20B)
		 */

		std::string peer_hash = answer.substr(28, 20);
		std::string returned = hexDecode(file.get_info_hash());
		if (peer_hash != returned)
			throw std::runtime_error("Hashes mismatch");
	}

	void establish_peer_connection(peer& peer) {
		const auto handshake_msg = peer.create_handshake_message({ file.get_info_hash(), id });
		try {
			std::string handshake_answer = peer.perform_handshake(handshake_msg);
			handshake_answer = peer.perform_handshake(handshake_msg);
			std::cout << "Hs answer: " << handshake_answer << std::endl;
			
			if (handshake_answer.empty())
				throw wrong_connection("Can't connect to peer with ip [" + peer.get_ip() + "].");

			check_hash(handshake_answer);

			peer.update_bitfield(handshake_answer.substr(68));
		}
		catch (std::runtime_error& e) {
			if (std::string(e.what()).find("10060") != std::string::npos) {
				throw wrong_connection("Can't connect to peer with ip [" + peer.get_ip() + "].");
			} 
			else
				throw;
		}
	}

private:
	std::string id;
	torrent_file file;
};


int main() {
	const std::string path = "C:\\Users\\aizee\\Downloads\\ComputerNetworks.torrent";
	client cl(path);
	cl.start_download();
}