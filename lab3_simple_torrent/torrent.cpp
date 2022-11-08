#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <random>
#include <cpr/cpr.h>
#include <iomanip>
#include "bencode.hpp"
#include "torrent_file.h"
#include "socket.h"
#include <thread>
#include <chrono>
#include <memory>
#include <limits>
#include "sha1/sha1.h"
#include <type_traits>
#include <cstdint>

std::string hex_decode(const std::string& value) {
	size_t hash_length = value.length();
	std::string decoded_hex_string;
	for (size_t i = 0; i < hash_length; i += 2)
	{
		std::string byte = value.substr(i, 2);
		char c = (char)strtol(byte.c_str(), nullptr, 16);
		decoded_hex_string.push_back(c);
	}
	return decoded_hex_string;
}

std::string receive_via_socket(const socket_wrapper& slave_socket) {
	int received_bytes = 0;
	std::string received_buffer;
	try {
		do {
			char received[8192];

			received_bytes = slave_socket.recv(received, 8192, 0, 10000);
			
			if (received_bytes == -1) {  // timeout - try again
				received_bytes = 1;
				continue;
			}

			for (size_t i = 0; i < received_bytes; ++i)
				received_buffer.push_back(received[i]);

			if (received_bytes > 0) {

				if (int(received_buffer[0]) == 19) // handshake message
					break;

				if (received_bytes > 4) {

					std::string length_str = received_buffer.substr(0, 4);
					unsigned int length = 0;
					for (size_t i = 0; i < 4; ++i) {
						length += int((length_str[3 - i] + 256) % 256) * int(std::pow(256, i));
					}

					if (length == received_buffer.substr(4).size())  // read all data from peer reply
						break;

				}

			}

		} while (received_bytes > 0);
	}
	catch (std::exception&) {
		throw;
	}
	return received_buffer;
}

class wrong_connection : public std::runtime_error {  // for socket's connection errors
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
		piece = 7
	};

public:
	bit_message(const message_id id, const std::string& payload = "") : id(id), payload(payload) { ; }

	static std::string create_message(const uint32_t length, const uint8_t id, const std::string& payload) {
		auto message_length = htonl(length);

		char temp[5];

		std::memcpy(temp, &message_length, sizeof(message_length));
		std::memcpy(temp + 4, &id, sizeof(id));
		std::string buffer;
		for (size_t i = 0; i < 5; ++i)
			buffer += (char)temp[i];

		return buffer + payload;
	}

	int8_t get_id() const noexcept { return id; }
	std::string get_payload() const noexcept { return payload; }

private:
	int8_t id;
	std::string payload;
};


class peer {
private:
	struct handshake_params {
		std::string info_hash;
		std::string client_id;
	};
public:
	peer(const std::string& id, const std::string& ip, const uint16_t port, socket_wrapper&& socket)
		: id(id), ip(ip), port(port), connect_socket(std::move(socket)), status("") {}

	std::string get_ip() const { return ip; }
	std::string get_bitfield() const { return bitfield; }
	std::string get_status() const { return status; }
	void set_status(const std::string& new_status) { status = new_status; }

	std::string perform_handshake(const std::string& handshake_message) {
		if (!connect_socket.is_connected()) {
			connect_socket.connect(address_family::IPV4, port, ip);
		}

		connect_socket.send(handshake_message.c_str(), static_cast<int>(handshake_message.size()));

		return receive_via_socket(connect_socket);
	}

	static std::string create_handshake_message(const handshake_params& params) {
		/*
		 * handshake: len_protocol(2B) + protocol(19B) + reserved(8B) + info_hash(20B) + peer_id(20B)
		 */
		const std::string protocol = "BitTorrent protocol";
		std::stringstream buffer;
		buffer << (char)19;
		buffer << protocol;
		std::string reserved;
		for (size_t i = 0; i < 8; ++i)
			reserved.push_back('\0');
		buffer << reserved;
		buffer << hex_decode(params.info_hash);
		buffer << params.client_id;
		assert(buffer.str().length() == protocol.length() + 49);
		return buffer.str();
	}
	
	static void check_hash(const std::string& answer, const std::string& info_hash) {
		/*
		 * answer: len_protocol(2B) + protocol(19B) + reserved(8B) + info_hash(20B) + peer_id(20B)
		 */
		std::string peer_hash = answer.substr(28, 20);
		std::string file_hash = hex_decode(info_hash);
		if (peer_hash != file_hash)
			throw std::runtime_error("Hashes mismatch");
	}

	void establish_peer_connection(const std::string& info_hash, const std::string& client_id) {
		const std::string handshake_request = create_handshake_message({ info_hash, client_id });
		try {
			std::string handshake_answer = perform_handshake(handshake_request);

			if (handshake_answer.empty())
				throw wrong_connection("Handshake failed with peer with ip [" + ip + "].");

			try {
				check_hash(handshake_answer, info_hash);
			}
			catch (std::runtime_error&) {
				throw std::runtime_error("Hashes mismatch in peer[" + ip + "] answer");
			}

			status = "handshaked";

			update_bitfield(handshake_answer.substr(68));
		}
		catch (std::runtime_error& e) {
			status = "";

			if (std::string(e.what()).find("10060") != std::string::npos) {
				throw wrong_connection("Can't connect to peer with ip [" + ip + "].");
			}
			else
				throw;
		}
	}

	std::string get_bitfield(const std::string& answer) {
		/*
		 * general scheme: length(4B) + bitfield_msg_id(1B) + payload(length)
		 * 
		 * several peers:  length(4B) + msg_id(1B) + bitfield(length) + length(4B) + have_msg_id(1B) + payload(length) +  length(4B) + unchoke_msg_id(1B)
		 */

		std::vector<bit_message> messages;

		// Get 'bitfield' (id = 5), always exists
		std::string length_str = answer.substr(0, 4);
		uint32_t length = 0;
		for (size_t i = 0; i < 4; ++i) {
			length += uint32_t((length_str[3 - i] + 256) % 256) * uint32_t(std::pow(256, i));
		}

		uint8_t message_id = uint8_t(answer.substr(4, 1)[0]);
		std::string payload = answer.substr(5u, length - 1u);

		messages.push_back(bit_message(bit_message::message_id(message_id), payload));

		// Get 'have' messages (id = 4) if exists
		for (size_t i = 4u + length + 5u; i < answer.size(); i += 9) {
			message_id = int(answer.substr(i - 1, 1)[0]);
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

		for (size_t i = 1; i < messages.size(); ++i) {  // go through 'have' messages and update transformed_bit_field 
			if (messages[i].get_id() == bit_message::have) {
				size_t index = 0;
				std::string payload = messages[i].get_payload();
				for (size_t j = 0; j < 4; ++j) {
					index += size_t((payload[3 - j] + 256) % 256) * size_t(std::pow(256, j));
				}
				transformed_bitfield[index] = '1';
			}
		}

		return transformed_bitfield;
	}

	void update_bitfield(const std::string& answer) { 
		if (answer.size() != 0)
			bitfield = get_bitfield(answer); 
		else {
			std::string bitfield_answer = receive_via_socket(connect_socket);
			bitfield = get_bitfield(bitfield_answer);
		}

	}

	void send_interested() {
		std::string message = bit_message::create_message(1u, bit_message::interested, "");

		connect_socket.send(message.c_str(), static_cast<int>(message.size()));
	}

	bit_message receive_message() const {
		std::string answer = receive_via_socket(connect_socket);

		if (answer.empty()) {
			return bit_message(bit_message::keep_alive);
		}

		/*
		 * general scheme: length(4B) + msg_id(1B) + payload(length) 
		 */

		std::string length_str = answer.substr(0, 4);
		size_t length = 0;
		for (size_t i = 0; i < 4; ++i) {
			length += int((length_str[3 - i] + 256) % 256) * int(std::pow(256, i));
		}

		int8_t message_id = int8_t(answer.substr(4, 1)[0]);
		std::string payload = answer.substr(5, length - 1u);

		return bit_message(bit_message::message_id(message_id), payload);
	}

	void request_piece(const uint32_t piece_index, const uint32_t piece_length, 
		const uint32_t piece_count, const uint64_t file_size) const 
	{
		char temp[12];

		// Needs to convert little-endian to big-endian
		auto index = htonl(piece_index);
		auto offset = htonl(0u);
		auto length = htonl(piece_length);

		if (piece_index == piece_count - 1) {
			length = htonl(file_size % piece_length);  // size of last piece can be not equal to piece_length
			if (length == htonl(0))  
				length = htonl(piece_length);
		}
		std::memcpy(temp, &index, sizeof(index));
		std::memcpy(temp + 4, &offset, sizeof(offset));
		std::memcpy(temp + 8, &length, sizeof(length));

		std::string payload;
		for (size_t i = 0; i < 12; ++i)
			payload += (char)temp[i];

		std::string request = bit_message::create_message(13u, bit_message::request, payload);
		connect_socket.send(request.c_str(), static_cast<int>(request.size()));
	}

private:
	std::string id;
	std::string ip;
	uint16_t port;
	std::string bitfield = "";
	socket_wrapper connect_socket;
	std::string status;
};


class piece_manager {
public:
	piece_manager(const std::vector<std::string>& hashes, const uint32_t piece_length, const uint64_t file_size) 
		: piece_length(piece_length), file_size(file_size), piece_downloaded(0) 
	{
		for (size_t i = 0; i < hashes.size(); ++i) {
			pieces.push_back({ "", hashes[i], "not downloaded" });
		}
	}

	void add_peer(const std::shared_ptr<peer>& p) {
		peers.push_back(p);
	}

	uint32_t get_next_download_index(const std::shared_ptr<peer>& p) {
		mutex.lock();
		for (uint32_t i = 0; i < pieces.size(); ++i) {
			if ((pieces[i].status == "not downloaded") && p->get_bitfield()[i] == '1') {
				set_piece_status(i, "downloading");
				mutex.unlock();
				return i;
			}
		}

		mutex.unlock();
		return ULONG_MAX;
	}

	void set_piece_status(const size_t index, const std::string& new_status) {
		pieces[index].status = new_status;
	}

	void peer_download_pieces(const std::shared_ptr<peer>& p, const std::string& info_hash, const std::string& client_id) {
		while (!is_complete.load()) {
			try {
				p->establish_peer_connection(info_hash, client_id);
				p->send_interested();
				
				uint32_t index = ULONG_MAX;
				while (!is_complete.load()) {

					bit_message message = p->receive_message();

					if (message.get_id() == bit_message::keep_alive) {
						p->set_status("keep alive");
						p->send_interested();
						continue;
					}

					if (message.get_id() == bit_message::choke) {
						continue;
					}
					else if (message.get_id() == bit_message::unchoke) {
						p->set_status("unchoked");  // request piece
					}
					else if (message.get_id() == bit_message::piece) {
						std::string data = message.get_payload().substr(8);  // index + begin + data
						if (hex_decode(sha1(data)) != pieces[index].hash) {
							mutex.lock();
							set_piece_status(index, "not downloaded");
							mutex.unlock();
						}
						else {
							p->set_status("received piece");

							set_piece_status(index, "downloaded");
							++piece_downloaded;
							pieces[index].data = data;

							if (piece_downloaded.load() == pieces.size()) {
								is_complete.store(true);
								return;
							}
						}
					}
					index = get_next_download_index(p);

					if (index == ULONG_MAX)
						continue;
					p->request_piece(index, piece_length, static_cast<uint32_t>(pieces.size()), file_size);
					p->set_status("requested piece");
				}

			}
			catch (wrong_connection& e) {
				std::cout << e.what() << std::endl;

				using namespace std::chrono_literals;
				std::this_thread::sleep_for(10000ms);
				continue;
			}
			catch (std::exception& e) {
				std::cout << e.what() << std::endl;
				return;
			}
		}
	}

	uint32_t get_active_peers() {
		uint32_t num = 0;
		for (const auto& p : peers) {
			if (p->get_status() == "requested piece")
				++num;
		}
		return num;
	}

	std::string download_pieces(const std::string& info_hash, const std::string& client_id) {
		time_t start_time = std::time(nullptr);
		time_t current_time = std::time(nullptr);
		double diff = std::difftime(current_time, start_time);

		for (size_t i = 0; i < std::min<size_t>(std::thread::hardware_concurrency(), peers.size()); ++i) {  // create threads
			std::thread peer_thread(
				&piece_manager::peer_download_pieces, this, std::cref(peers[i]), std::cref(info_hash), std::cref(client_id)
			);
			peer_thread.detach();
		}

		while (!is_complete.load()) {
			current_time = std::time(nullptr);
			diff = std::difftime(current_time, start_time);

			auto percent = (float)piece_downloaded / pieces.size() * 100.f;

			std::cout << "peers [" << get_active_peers() << "/" << peers.size() << "]"
					  << " downloading [" << std::to_string(piece_downloaded) << "/" << pieces.size() << "] "
					  << percent << std::setprecision(percent < 10.f ? 3 : 4) << "% in " << diff << "s\r";

			using namespace std::chrono_literals;
			std::this_thread::sleep_for(1000ms);
		}
		std::cout << "\n";

		std::string file_data;
		for (size_t i = 0; i < pieces.size(); ++i) {
			file_data += pieces[i].data;
		}

		return file_data;
	}


private:
	struct piece {
		std::string data;
		std::string hash;
		std::string status;
	};
private:
	std::vector<std::shared_ptr<peer>> peers;  // peer: status
	std::vector<piece> pieces;
	uint32_t piece_length;
	uint64_t file_size;
	
	std::atomic<uint32_t> piece_downloaded = 0;
	std::atomic<bool> is_complete = false;
	std::mutex mutex;
};

class torrent_client {
public:
	torrent_client() : id(generate_client_id()) {}

	void start_download(const std::string& path_to_file, const std::string& path_to_output_dir) {
		try {
			file.add_file(path_to_file);

			std::vector<std::shared_ptr<peer>> peers = get_peers();
			std::cout << "get [" << peers.size() << "] peers from tracker\n";

			piece_manager manager(file.split_piece_hashes(), file.get_piece_length(), file.get_file_size());
			for (auto& p : peers) {
				manager.add_peer(p);
			}

			const std::string file_data = manager.download_pieces(file.get_info_hash(), id);
			std::cout << "downloading finished\n";
			std::cout << "saving to file...\n";

			std::string full_path = path_to_output_dir + "\\" + file.get_file_name();
			std::ofstream out;
			out.open(full_path, std::ios::binary);
			if (out.is_open())
			{
				out << file_data << std::endl;
			}
			out.close();

			std::cout << "saved\n";
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
		uint64_t uploaded;
		uint64_t downloaded;
		uint64_t left;
		uint16_t port;
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

	std::string get_peers_request(request_params& params) {
		cpr::Response res = cpr::Get(cpr::Url{ params.url }, cpr::Parameters{
					{ "info_hash", hex_decode(params.info_hash) },
					{ "peer_id", params.client_id },
					{ "port", std::to_string(params.port) },
					{ "uploaded", std::to_string(params.uploaded) },
					{ "downloaded", std::to_string(params.downloaded) },
					{ "left", std::to_string(params.left) },
					{ "compact", std::to_string(1) }
			}, cpr::Timeout{ cpr::Timeout(10000) }
		);
		return res.text;
	}

	std::vector<std::shared_ptr<peer>> get_peers() {
		try {
			const auto announce_url = file.get_announce();
			const auto info_hash = file.get_info_hash();
			request_params params{announce_url, id, info_hash,
				0ull, 0ull, file.get_file_size(), 6881};
			const auto tracker_response = get_peers_request(params);
			const auto response_dict = std::get<bencode::dict>(bencode::decode(tracker_response));

			std::vector<std::shared_ptr<peer>> peers;
			if (response_dict->find("peers")->second.index() == 2) {  // peers as bencoded data (list)
				bencode::list peers_list = std::get<bencode::list>(response_dict->find("peers")->second);

				for (auto& p : peers_list) {
					peers.push_back(std::make_shared<peer>(
						std::get<bencode::string>(std::get<bencode::dict>(p)->find("peer id")->second),
						std::get<bencode::string>(std::get<bencode::dict>(p)->find("ip")->second),
						std::get<bencode::integer>(std::get<bencode::dict>(p)->find("port")->second),
						socket_wrapper(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP)
						));
				}
			}
			else {  // peers are raw data
				bencode::string peers_string = std::get<bencode::string>(response_dict->find("peers")->second);
				for (size_t i = 0; i < peers_string.size(); i += 6) {
					std::string ip;
					ip += std::to_string(ntohs(peers_string[i])) + ".";
					ip += std::to_string(ntohs(peers_string[i + 1])) + ".";
					ip += std::to_string(ntohs(peers_string[i + 2])) + ".";
					ip += std::to_string(ntohs(peers_string[i + 3]));

					std::string port;
					port += std::to_string(ntohs(std::stoi(peers_string.substr(i + 4, 2))));

					peers.push_back(std::make_shared<peer>(
						id,
						ip,
						std::stoll(port),
						socket_wrapper(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP)
						));
				}
			}
			return peers;

		}
		catch (...) {
			throw;
		}
	}

private:
	std::string id;
	torrent_file file;
};


int main() {
	const std::string path_to_torrent_file = "C:\\Users\\aizee\\Downloads\\ComputerNetworks.torrent";
	torrent_client cl;
	cl.start_download(path_to_torrent_file, "C:\\Users\\aizee\\OneDrive\\Desktop");
}
