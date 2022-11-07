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
#include <thread>
#include <chrono>
#include <memory>
#include <limits>
#include "sha1/sha1.h"

std::string hexDecode(const std::string& value) {
	size_t hashLength = value.length();
	std::string decodedHexString;
	for (size_t i = 0; i < hashLength; i += 2)
	{
		std::string byte = value.substr(i, 2);
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
			received_bytes = slave_socket.recv(received, 8192, 0, 100000);

			for (size_t i = 0; i < received_bytes; ++i)
				received_buffer.push_back(received[i]);

		} while (received_bytes > 0);
	}
	catch (std::exception&) {
		throw;
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
	bit_message(const int id, const std::string& payload = "") : id(id), payload(payload) { ; }

	std::string create_message(const unsigned int length) {
		unsigned long message_length = htonl(length);
		//unsigned long message_id = htonl(id);

		char temp[5];

		std::memcpy(temp, &message_length, sizeof(unsigned int));
		std::memcpy(temp + 4, &id, sizeof(char));
		std::string buffer;
		for (int i = 0; i < 5; ++i)
			buffer += (char)temp[i];

		return buffer + payload;
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
		: id(id), ip(ip), port(port), connect_socket(std::move(socket)), status("") {}

	peer(const std::string& id, const std::string& ip, const unsigned long long port, socket_wrapper&& socket)
		: id(id), ip(ip), port(static_cast<unsigned short>(port)), connect_socket(std::move(socket)), status("") {}

	std::string get_ip() const { return ip; }
	std::string get_bitfield() const { return bitfield; }
	std::string get_status() const { return status; }
	void set_status(const std::string& new_status) { status = new_status; }

	std::string perform_handshake(const std::string& handshake_message) const {
		connect_socket.connect(address_family::IPV4, port, ip);
//		connect_socket.set_nonblocking_mode();

		connect_socket.send(handshake_message.c_str(), static_cast<int>(handshake_message.size()));

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
	
	static void check_hash(const std::string& answer, const std::string& info_hash) {
		/*
		 * len_protocol(2B) + protocol(19B) + reserved(8B) + info_hash(20B) + peer_id(20B)
		 */
		std::string peer_hash = answer.substr(28, 20);
		std::string returned = hexDecode(info_hash);
		if (peer_hash != returned)
			throw std::runtime_error("Hashes mismatch");
	}

	void establish_peer_connection(const std::string& info_hash, const std::string& client_id) {
		const auto handshake_msg = create_handshake_message({ info_hash, client_id });
		try {
			std::string handshake_answer = perform_handshake(handshake_msg);

			if (handshake_answer.empty())
				throw wrong_connection("Can't connect to peer with ip [" + ip + "].");

			try {
				check_hash(handshake_answer, info_hash);
			}
			catch (std::runtime_error&) {
				throw std::runtime_error("Hashes mismatch in peer[" + ip + "] answer");
			}

			status = "handshaked";

//			std::cout << "peer[" << ip << "] handshaked\n";

			update_bitfield(handshake_answer.substr(68));
		}
		catch (std::runtime_error& e) {
			status = "";

			if (std::string(e.what()).find("10060") != std::string::npos) {
				throw wrong_connection("Can't connect to peer with ip [" + get_ip() + "].");
			}
			else
				throw;
		}
	}

	std::string get_bitfield(const std::string& answer) {
		/*
		 * length(4B) + msg_id(1B) + bitfield(until end)
		 */

		std::vector<bit_message> messages;

		// Get 'bitfield' (id = 5), always exists
		std::string length_str = answer.substr(0, 4);
		unsigned int length = 0;
		for (size_t i = 0; i < 4; ++i) {
			length += int((length_str[3 - i] + 256) % 256) * int(std::pow(256, i));
		}

		int message_id = int(answer.substr(4, 1)[0]);
		std::string payload = answer.substr(5, length - 1u);

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
					index += int((payload[3 - j] + 256) % 256) * int(std::pow(256, j));
				}
				transformed_bitfield[index] = '1';
			}
			//else if (messages[i].get_id() == bit_message::unchoke) {  // if peer send 'unchoke' after 'have' messages before 'interested'
				//set_status("unchoked");
			//}
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
		std::cout << "peer[" << ip << "] update bitfield\n";

	}

	void send_interested() {
		std::string message = bit_message(bit_message::interested).create_message(1u);

		connect_socket.send(message.c_str(), static_cast<int>(message.size()));
	}

	bit_message receive_message() const {
		std::string answer = receive_via_socket(connect_socket);

		if (answer.empty()) {
			return bit_message(bit_message::keep_alive);
		}

		std::string length_str = answer.substr(0, 4);
		size_t length = 0;
		for (size_t i = 0; i < 4; ++i) {
			length += int((length_str[3 - i] + 256) % 256) * int(std::pow(256, i));
		}

		int message_id = int(answer.substr(4, 1)[0]);
		std::string payload = answer.substr(5, length - 1u);

		return bit_message(message_id, payload);
	}

	void request_piece(const unsigned int piece_index, const unsigned int piece_length, const unsigned int piece_count, const unsigned int file_size) const {
		char temp[12];

		// Needs to convert little-endian to big-endian
		unsigned int index = htonl(piece_index);
		unsigned int offset = htonl(0u);
		unsigned int length = htonl(piece_length);
		if (piece_index == piece_count - 1) {
			length = htonl(file_size % piece_length);
			if (length == htonl(0))
				length = htonl(piece_length);
		}
		std::memcpy(temp, &index, sizeof(unsigned int));
		std::memcpy(temp + 4, &offset, sizeof(unsigned int));
		std::memcpy(temp + 8, &length, sizeof(unsigned int));
		std::string payload;
		for (int i = 0; i < 12; ++i)
			payload += (char)temp[i];

		std::string request = bit_message(bit_message::request, payload).create_message(13);
		connect_socket.send(request.c_str(), static_cast<int>(request.size()));
	}

	struct hash {
		const auto operator()(const std::shared_ptr<peer>& p) const {
			return std::hash<std::string>{}(p->id);
		}
	};

private:
	std::string id;
	std::string ip;
	unsigned short port;
	std::string bitfield = "";
	socket_wrapper connect_socket;
	std::string status;
};


class piece_manager {
public:
	piece_manager(const std::vector<std::string>& hashes, const size_t piece_length, const size_t file_size) 
		: piece_length(piece_length), piece_downloaded(0), file_size(file_size) {
		for (size_t i = 0; i < hashes.size(); ++i) {
			pieces.push_back({ "", hashes[i], "not downloaded" });
		}
	}

	void add_peer(const std::shared_ptr<peer>& p) {
		peers.push_back(p);
	}

	size_t get_next_download_index(const std::shared_ptr<peer>& p) {
		for (int i = pieces.size() - 1; i >= 0; --i) {
			if ((pieces[i].status == "not downloaded") && p->get_bitfield()[i] == '1') {
				set_piece_status(i, "downloading");
				return i;
			}
		}

		return ULLONG_MAX;
	}

	void set_piece_status(const size_t index, const std::string& new_status) {
		pieces[index].status = new_status;
	}

	void peer_thread(const std::shared_ptr<peer>& p, const std::string& info_hash, const std::string& client_id) {
		while (!is_complete.load()) {
			try {
				p->establish_peer_connection(info_hash, client_id);
//				std::cout << "peer[" << p->get_ip() << "] status[" << p->get_status() << "]\n";

				p->send_interested();
				
				size_t index = ULLONG_MAX;
				while (!is_complete.load()) {
					bit_message message(bit_message::unchoke);
					if (p->get_status() != "unchoked") {
						message = p->receive_message();
						std::cout << "peer[" << p->get_ip() << "] receive message[" << std::to_string(message.get_id()) << "]\n";
					}

//					mutex.lock();
//					std::cout << "peer[" << p->get_ip() << "] receive message[" << std::to_string(message.get_id()) << "]\n";
//					mutex.unlock();

					if (message.get_id() == bit_message::keep_alive) {
						continue;
					}

					if (message.get_id() == bit_message::choke) {
						continue;
					}
					else if (message.get_id() == bit_message::unchoke) {
//						std::cout << "peer[" << p->get_ip() << "] unchoked\n";

						p->set_status("unchoked");
					}
					else if (message.get_id() == bit_message::piece) {
						std::string data = message.get_payload().substr(8);  // index + begin + data
						if (hexDecode(sha1(data)) != pieces[index].hash) {
							mutex.lock();
							set_piece_status(index, "not downloaded");
							std::cout << "Piece[" << std::to_string(index) << "] hash mismatch!\n";
							mutex.unlock();
						}
						else {
							std::cout << "peer[" << p->get_ip() << "] download [" << index << "] piece\n";
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
					mutex.lock();
					index = get_next_download_index(p);
					mutex.unlock();

					if (index == ULLONG_MAX) {
						continue;
					}
					p->request_piece(index, piece_length, pieces.size(), file_size);
					p->set_status("requested piece");
					std::cout << "peer[" << p->get_ip() << "] request [" << std::to_string(index) << "]\n";
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

	void download_pieces(const std::string& info_hash, const std::string& client_id) {
		for (size_t i = 0; i < std::min<size_t>(std::thread::hardware_concurrency(), peers.size()); ++i) {
			std::thread peer_thread(
				&piece_manager::peer_thread, this, std::cref(peers[i]), std::cref(info_hash), std::cref(client_id)
			);
			peer_thread.detach();
		}
		while (!is_complete.load()) {
			std::cout << "downloaded " << std::to_string(piece_downloaded) << "\n";
			using namespace std::chrono_literals;
			std::this_thread::sleep_for(10000ms);
		}
		std::string buffer;
		for (size_t i = 0; i < pieces.size(); ++i) {
			buffer += pieces[i].data;
		}
		std::string filename = "C:\\Users\\meshc\\Downloads\\ComputerNetworks_own.txt";
		std::ofstream out;          // поток для записи
		out.open(filename, std::ios::binary); // окрываем файл для записи
		if (out.is_open())
		{
			out << buffer << std::endl;
		}

		out.close();
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
	size_t piece_length;
	size_t file_size;

	std::atomic<size_t> piece_downloaded = 0;
	std::atomic<bool> is_complete = false;
	std::mutex mutex;
};

class client {
public:
	client(const std::string& path_to_file) : id(generate_client_id()), file(path_to_file) { ; }

	void start_download() {
		try {
			std::vector<std::shared_ptr<peer>> peers = get_peers();
			piece_manager manager(file.split_piece_hashes(), file.get_piece_length(), file.get_file_size());
			for (auto& p : peers) {
				manager.add_peer(p);
			}
			manager.download_pieces(file.get_info_hash(), id);
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

	std::vector<std::shared_ptr<peer>> get_peers() {
		try {
			const auto announce_url = file.get_announce();
			const auto info_hash = file.get_info_hash();
			const auto tracker_response = get_peers_request({ announce_url, id, info_hash,
				0ull, 0ull, file.get_file_size(), 6881});
			const auto response_dict = std::get<bencode::dict>(bencode::decode(tracker_response));

			bencode::list peers_list = std::get<bencode::list>(response_dict->find("peers")->second);
			std::vector<std::shared_ptr<peer>> peers;
			std::vector<std::string> peer_ips, peer_ids;
			std::vector<unsigned short> peer_ports;

			for (auto& p : peers_list) {
				peers.push_back(std::make_shared<peer>(
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

private:
	std::string id;
	torrent_file file;
};


int main() {
	const std::string path = "C:\\Users\\meshc\\Downloads\\ComputerNetworks.torrent";
	client cl(path);
	cl.start_download();
}

/*
 */