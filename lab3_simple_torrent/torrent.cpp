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

class client {
public:
	client(const std::string& path_to_file) : id(generate_peer_id()), file(path_to_file) { ; }

	void start_download() {
		try {
			std::vector<peer> peers = get_peers();
			std::map<std::string, bit_message> dict;
			for (auto peer : peers) {
				try {
					dict.insert({ peer.ip, connect_to_peer(peer) });
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
		std::string peer_id;
		std::string info_hash;
		unsigned long long uploaded;
		unsigned long long downloaded;
		unsigned long long left;
		unsigned short port;
	};

	struct handshake_params {
		std::string info_hash;
		std::string client_id;
	};

	struct peer {
		std::string id;
		std::string ip;
		unsigned short port;

		peer(const std::string& id, const std::string& ip, const unsigned short port)
			: id(id), ip(ip), port(port) {
			;
		}
		peer(const std::string& id, const std::string& ip, const unsigned long long port)
			: id(id), ip(ip), port(static_cast<unsigned short>(port)) {
			;
		}
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


private:
	std::string generate_peer_id() {
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
					{ "peer_id", std::string(params.peer_id) },
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
			const auto peer_id = generate_peer_id();
			const auto info_hash = file.get_info_hash();
			const auto tracker_response = get_peers_request({ announce_url, peer_id, info_hash,
				0ull, 0ull, 2918443ull, 6881 });  // TODO: generialize
			const auto response_dict = std::get<bencode::dict>(bencode::decode(tracker_response));

			bencode::list peers_list = std::get<bencode::list>(response_dict->find("peers")->second);
			std::vector<peer> peers;
			std::vector<std::string> peer_ips, peer_ids;
			std::vector<unsigned short> peer_ports;

			for (auto& peer : peers_list) {
				peers.push_back({
					std::get<bencode::string>(std::get<bencode::dict>(peer)->find("peer id")->second),
					std::get<bencode::string>(std::get<bencode::dict>(peer)->find("ip")->second),
					std::get<bencode::integer>(std::get<bencode::dict>(peer)->find("port")->second)
					});
			}
			return peers;

		}
		catch (...) {
			throw;
		}
	}

	std::string perform_handshake(const std::string& handshake_message, const peer& peer) {
		const socket_wrapper connect_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);
		connect_socket.connect(address_family::IPV4, peer.port, peer.ip);

		const int bytes_sent_size = connect_socket.send(handshake_message.c_str(),
			static_cast<int>(handshake_message.size()));
		connect_socket.shutdown(shutdown_type::SEND);
		return receive_via_socket(connect_socket);
	}

	std::string create_handshake_message(const handshake_params& params) {
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

	bit_message handle_handshake_answer(const std::string& answer) {
		/* 
		 * len_protocol(2B) + protocol(19B) + reserved(8B) + info_hash(20B) + peer_id(20B) +
		 * length(4B) + msg_id(1B) + bitfield(until end)
		 */

		std::string peer_hash = answer.substr(28, 20);
		std::string returned = hexDecode(file.get_info_hash());
		if (peer_hash != returned)
			throw std::runtime_error("Hashes mismatch");

		int message_id = std::stoi(answer.substr(72, 1));
		std::string payload = answer.substr(73);
		return bit_message(bit_message::message_id(message_id), payload);
	}

	bit_message connect_to_peer(const peer& peer) {
		const auto handshake_msg = create_handshake_message({ file.get_info_hash(), peer.id });
		try {
			std::string handshake_answer = perform_handshake(handshake_msg, peer);
			std::cout << "Hs answer: " << handshake_answer << std::endl;
			
			if (handshake_answer.empty())
				return bit_message(bit_message::message_id::keep_alive);

			return handle_handshake_answer(handshake_answer);

		}
		catch (std::runtime_error& e) {
			if (std::string(e.what()).find("10060") != std::string::npos) {
				throw wrong_connection("Can't connect to peer with ip [" + peer.ip + "].");
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