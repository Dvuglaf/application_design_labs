#ifndef _PEER_H
#define _PEER_H
#include <string>
#include <sstream>
#include "bit_message.h"
#include "utils/utils.h"
#include "utils/socket/socket.h"


class peer {
private:
	struct handshake_params {
		std::string info_hash;
		std::string client_id;
	};

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

	static std::string get_bitfield(const std::string& answer) {
		/*
		 * general scheme: length(4B) + bitfield_msg_id(1B) + payload(length)
		 *
		 * several peers:  length(4B) + msg_id(1B) + bitfield(length) + length(4B) + have_msg_id(1B) + payload(length) +  length(4B) + unchoke_msg_id(1B)
		 */

		std::vector<bit_message> messages;

		// Get 'bitfield' (id = 5), always exists
		std::string length_str = answer.substr(0, 4);
		uint32_t length = get_number_from_raw<uint32_t>(length_str);
		uint8_t message_id = get_number_from_raw<uint8_t>(answer.substr(4, 1));
		std::string payload = answer.substr(5, length - 1);

		messages.push_back(bit_message(bit_message::message_id(message_id), payload));

		// Get 'have' messages (id = 4) if exists
		for (size_t i = 4 + length + 5; i < answer.size(); i += 9) {
			message_id = get_number_from_raw<uint8_t>(answer.substr(i - 1, 1));
			payload = answer.substr(i, 4);
			messages.push_back(bit_message(bit_message::message_id(message_id), payload));
		}

		// Form bitfield as string with '0' and '1', i_th symbol of bitfield shows if the peer has i_th piece
		std::string bitfield = messages[0].get_payload();
		std::string transformed_bitfield;

		for (char c : bitfield) {  // transform only bit_field
			for (int i = 7; i >= 0; --i) {  // from network order -> reverse
				transformed_bitfield += std::to_string((int(c) & (1 << i)) >> i);  // get neccessary bit in byte
			}
		}

		for (size_t i = 1; i < messages.size(); ++i) {  // go through 'have' messages and update transformed_bit_field 
			if (messages[i].get_id() == bit_message::have) {
				size_t index = get_number_from_raw<size_t>(payload);
				payload = messages[i].get_payload();
				transformed_bitfield[index] = '1';
			}
		}

		return transformed_bitfield;
	}

public:
	peer(const std::string& id, const std::string& ip, const uint16_t port, socket_wrapper&& socket)
		: _id(id), _ip(ip), _port(port), _connect_socket(std::move(socket)), _status("") { ; }

	std::string get_ip() const noexcept { return _ip; }
	std::string get_bitfield() const noexcept { return _bitfield; }
	std::string get_status() const noexcept { return _status; }
	void set_status(const std::string& new_status) { _status = new_status; }

	std::string perform_handshake(const std::string& handshake_message) {
		if (!_connect_socket.is_connected()) {
			_connect_socket.connect(address_family::IPV4, _port, _ip);
		}

		_connect_socket.send(handshake_message.c_str(), static_cast<int>(handshake_message.size()));

		return receive_via_socket(_connect_socket);
	}

	void establish_peer_connection(const handshake_params& params) {
		const std::string handshake_request = create_handshake_message({ params.info_hash, params.client_id });
		try {
			std::string handshake_answer = perform_handshake(handshake_request);

			if (handshake_answer.empty())
				throw wrong_connection("Handshake failed with peer with ip [" + _ip + "].");

			try {
				check_hash(handshake_answer, params.info_hash);
			}
			catch (std::runtime_error&) {
				throw std::runtime_error("Hashes mismatch in peer[" + _ip + "] answer");
			}

			_status = "handshaked";

			update_bitfield(handshake_answer.substr(68));
		}
		catch (std::runtime_error& e) {
			_status = "";

			if (std::string(e.what()).find("10060") != std::string::npos) {
				throw wrong_connection("Can't connect to peer with ip [" + _ip + "].");
			}
			else
				throw;
		}
	}

	void update_bitfield(const std::string& answer) {
		if (answer.size() != 0)
			_bitfield = get_bitfield(answer);
		else {
			std::string bitfield_answer = receive_via_socket(_connect_socket);
			_bitfield = get_bitfield(bitfield_answer);
		}

	}

	bit_message receive_message() const {
		std::string answer = receive_via_socket(_connect_socket);

		if (answer.empty()) {
			return bit_message(bit_message::keep_alive);
		}

		/*
		 * general scheme: length(4B) + msg_id(1B) + payload(length)
		 */

		std::string length_str = answer.substr(0, 4);
		uint32_t length = get_number_from_raw<uint32_t>(length_str);
		int8_t message_id = get_number_from_raw<int8_t>(answer.substr(4, 1));
		std::string payload = answer.substr(5, length - 1);

		return bit_message(bit_message::message_id(message_id), payload);
	}

	void send_interested() const {
		std::string message = bit_message::create_message(1, bit_message::interested, "");

		_connect_socket.send(message.c_str(), static_cast<int>(message.size()));
	}

	void request_piece(const uint32_t piece_index, const uint32_t piece_length,
		const uint32_t piece_count, const uint64_t file_size) const
	{
		// Needs to convert little-endian to big-endian
		auto index = htonl(piece_index);
		auto offset = htonl(0);
		auto length = htonl(piece_length);

		if (piece_index == piece_count - 1) {
			length = htonl(file_size % piece_length);  // size of last piece can be not equal to piece_length
			if (length == htonl(0))
				length = htonl(piece_length);
		}

		/*
		 * piece request: length[13](4B) + msg_id[6](1B) + payload(12B)
		 */

		char temp[12];
		std::memcpy(temp, &index, sizeof(index));
		std::memcpy(temp + 4, &offset, sizeof(offset));
		std::memcpy(temp + 8, &length, sizeof(length));

		std::string payload;
		for (size_t i = 0; i < 12; ++i)
			payload += (char)temp[i];

		std::string request = bit_message::create_message(13, bit_message::request, payload);
		_connect_socket.send(request.c_str(), static_cast<int>(request.size()));
	}

private:
	std::string _id;
	std::string _ip;
	uint16_t _port;
	std::string _bitfield;
	socket_wrapper _connect_socket;
	std::string _status;
};
#endif