#pragma warning(disable:4996) 
#include <iostream>
#include <string>
#include <WinSock2.h>
#include <stdexcept>
#include "socket.h"


socket_wrapper::socket_wrapper(int net_protocol, int type, int protocol) {
	_socket = socket(net_protocol, type, protocol);
	if (_socket == INVALID_SOCKET)
		throw std::runtime_error(std::string("socket function failed with error ") + std::to_string(WSAGetLastError()));
}

socket_wrapper::socket_wrapper(SOCKET other) : _socket(other) {}

socket_wrapper::socket_wrapper(socket_wrapper&& other) noexcept {
	_socket = other._socket;
	other._socket = INVALID_SOCKET;
}

socket_wrapper& socket_wrapper::operator=(socket_wrapper&& other) noexcept {
	_socket = other._socket;
	other._socket = INVALID_SOCKET;
	return *this;
}

void socket_wrapper::bind(u_short net_protocol, u_short port, const std::string& ip) const {
	sockaddr_in socket_addr;

	socket_addr.sin_family = net_protocol;
	socket_addr.sin_port = htons(port);
	socket_addr.sin_addr.s_addr = inet_addr(ip.c_str());

	int result = ::bind(_socket, reinterpret_cast<SOCKADDR*>(&socket_addr), sizeof(socket_addr));
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("bind function failed with error ") + std::to_string(WSAGetLastError()));
	else
		std::cout << "bind successful" << std::endl;
}

void socket_wrapper::connect(u_short internet_protocol, u_short port, const std::string& ip) const {
	sockaddr_in socket_addr;

	socket_addr.sin_family = internet_protocol;
	socket_addr.sin_port = htons(port);
	socket_addr.sin_addr.s_addr = inet_addr(ip.c_str());

	const int result = ::connect(_socket, reinterpret_cast<SOCKADDR*>(&socket_addr), sizeof(socket_addr));
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("connect function failed with error ") + std::to_string(WSAGetLastError()));
	else
		std::cout << "connect successful" << std::endl;

}

void socket_wrapper::listen(int max_connections) const {
	const int result = ::listen(_socket, max_connections);
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("listen function failed with error ") + std::to_string(WSAGetLastError()));
	else
		std::cout << "listening on socket..." << std::endl;
}

socket_wrapper socket_wrapper::accept() const {
	SOCKET s = ::accept(_socket, NULL, NULL);

	if (s == INVALID_SOCKET)
		throw std::runtime_error(std::string("accept function failed with error ") + std::to_string(WSAGetLastError()));
	else
		std::cout << "client connected" << std::endl;

	return socket_wrapper(s);
}

void socket_wrapper::send(const std::string& buffer) const {
	const int result = ::send(_socket, buffer.c_str(), static_cast<int>(buffer.size()), 0);
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("send function failed with error ") + std::to_string(WSAGetLastError()));
	else 
		std::cout << "Bytes send: " << result << std::endl;	
}

std::string socket_wrapper::recv(size_t max_buff_size) const {
	std::string buffer(max_buff_size, '\0');

	int result = ::recv(_socket, buffer.data(), static_cast<int>(max_buff_size), 0);
	if (result < 0)
		throw std::runtime_error(std::string("recv function failed with error ") + std::to_string(WSAGetLastError()));

	const size_t num_bytes = buffer.find('\0');
	return buffer.substr(0, num_bytes);
}

void socket_wrapper::shutdown() const {
	const int result = ::shutdown(_socket, SD_BOTH);
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("shutdown function failed with error ") + std::to_string(WSAGetLastError()));
}
	
socket_wrapper::~socket_wrapper() {
	if (_socket != INVALID_SOCKET) {
		const int result = ::closesocket(_socket);
		if (result != 0)
			std::cerr << "closesocket function failed with error " << WSAGetLastError() << std::endl;
	}
}
