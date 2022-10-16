#include <vector>
#include <string>
#include <exception>
#include <iostream>
#include "utils.h"
#include "socket.h"

std::vector<std::basic_string<unsigned char>> split(const std::basic_string<unsigned char>& string, const unsigned char delimiter) {
	std::vector<std::basic_string<unsigned char>> tokens;
	size_t previous = 0, position = 0;
	do {
		position = string.find(delimiter, previous);
		if (position == std::basic_string<unsigned char>::npos) position = string.length();
		std::basic_string<unsigned char> token = string.substr(previous, position - previous);
		if (!token.empty()) tokens.push_back(token);
		previous = position + 1;
	} while (position < string.length() && previous < string.length());
	return tokens;
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