#ifndef _UTILS_H
#define _UTILS_H
#include <string>
#include <vector>
#include <exception>
#include "socket/socket.h"

template <typename T>
T get_number_from_raw(const std::string& bytes);

std::string hex_decode(const std::string&);
std::string receive_via_socket(const socket_wrapper&);
class wrong_connection;


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

template <typename T>
T get_number_from_raw(const std::string& bytes) {  // bytes in reverse order (network order)
	T number = 0;

	for (size_t i = 0; i < bytes.size(); ++i) {
		number += T((bytes[bytes.size() - 1 - i] + 256) % 256) * T(std::pow(256, i));  // reverse order from end to start
	}

	return number;
}

std::string receive_via_socket(const socket_wrapper& slave_socket) try {
	int received_bytes = 0;
	std::string received_buffer;
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

			if (received_buffer[0] == 19) {  // handshake
				if (received_buffer.size() < 72)  // length of handshake message is 68 and bitfield length param is 4
					continue;
				else {
					std::string length_str = received_buffer.substr(68, 4);
					uint32_t length = get_number_from_raw<uint32_t>(length_str);

					if (length == received_buffer.substr(72).size())
						break;
					else if (received_buffer.substr(72).size() > length)  // lazy bitfield (bitfield + having messages)
						break;
					else
						continue;
				}
			}

			if (received_bytes > 4) {
				std::string length_str = received_buffer.substr(0, 4);
				uint32_t length = get_number_from_raw<uint32_t>(length_str);

				if (length == received_buffer.substr(4).size())  // read all data from peer reply
					break;
			}
		}

	} while (received_bytes > 0);
	return received_buffer;
}
catch (std::exception&) {
	throw;
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
#endif