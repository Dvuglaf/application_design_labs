#include "utils.h"
#include <vector>
#include <string>
#include "socket.h"
#include <exception>
#include <iostream>
#include "chord.h"

std::vector<std::string> split(const std::string& string, const std::string& delimiter) {
	std::vector<std::string> tokens;
	size_t previous = 0, position = 0;
	do {
		position = string.find(delimiter, previous);
		if (position == std::string::npos) position = string.length();
		std::string token = string.substr(previous, position - previous);
		if (!token.empty()) tokens.push_back(token);
		previous = position + delimiter.length();
	} while (position < string.length() && previous < string.length());
	return tokens;
}

bool does_belong(unsigned long idx, unsigned long start, unsigned long end, bool left_included, bool right_included) {
	if (left_included && right_included) {  // [..., ...]
		if (start < end) {
			return (idx >= start && idx <= end);
		}
		else if (end < start) {
			return !(idx > end && idx < start);
		}
		else {
			return true;
		}
	}
	else if (left_included && !right_included) {  // [..., ...)
		if (start < end) {
			return (idx >= start && idx < end);
		}
		else if (end < start) {
			return !(idx >= end && idx < start);
		}
		else {
			return true;
		}
	}
	else if (!left_included && right_included) {  // (..., ...]
		if (start < end) {
			return (idx > start && idx <= end);
		}
		else if (end < start) {
			return !(idx > end && idx <= start);
		}
		else {
			return true;
		}
	}
	else {  // (..., ...)
		if (start < end) {
			return (idx > start && idx < end);
		}
		else if (end < start) {
			return !(idx >= end && idx <= start);
		}
		else {
			return !(idx == start);
		}
	}
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

void check_users_params(unsigned long id, const std::string& ip, unsigned long port) {
	if (id >= (1 << chord_node::_m)) {
		throw std::invalid_argument("Wrong ID. Possible value in [0.." +
			std::to_string(((1 << chord_node::_m) - 1)) + "].");
	}
	if (port > 65535) {
		throw std::invalid_argument("Wrong PORT. Possible value in [0.." +
			std::to_string(65535) + "].");
	}
	auto splitted_ip = split(ip, ".");
	if (splitted_ip.size() != 4)
		throw std::invalid_argument("Wrong IP.");
	for (const auto& digit : splitted_ip) {
		try {  // std::stoul can throw an exception
			if (!does_belong(std::stoul(digit), 0, 255, true, true))
				throw 0;
		}
		catch (...) {
			throw std::invalid_argument("Wrong IP.");
		}
	}
}

void check_users_params(const std::string& number) {
	try {
		auto pass = std::stoul(number);
	}
	catch (...) {
		throw std::invalid_argument("Not a number.");
	}
}
