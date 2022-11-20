#pragma warning(disable:4996)  
#include <iostream>
#include <string>
#include <WinSock2.h>
#include <stdexcept>
#include "socket.h"


size_t socket_wrapper::_socket_count = 0;
WSADATA socket_wrapper::_wsa; 

socket_wrapper::socket_wrapper(int adress_family, int type, int protocol) {
	if (_socket_count == 0) {
		const int result = WSAStartup(MAKEWORD(2, 2), &_wsa);
		if (result != NO_ERROR)
			throw std::runtime_error(std::string("WSAStartup failed with error ") + std::to_string(WSAGetLastError()));
	}

	_socket = socket(adress_family, type, protocol);
	if (_socket == INVALID_SOCKET)
		throw std::runtime_error(std::string("socket function failed with error ") + std::to_string(WSAGetLastError()));

	++_socket_count;
}

socket_wrapper::socket_wrapper(SOCKET other) : _socket(other) {}

socket_wrapper::socket_wrapper(socket_wrapper&& other) noexcept {
	_socket = other._socket;
	other._socket = INVALID_SOCKET;
}

socket_wrapper& socket_wrapper::operator=(socket_wrapper&& other) noexcept {
	if (other._socket == this->_socket)
		return *this;

	this->~socket_wrapper();
	_socket = other._socket;
	other._socket = INVALID_SOCKET;
	return *this;
}

void socket_wrapper::bind(u_short adress_family, u_short port, const std::string& ip) const {
	sockaddr_in socket_addr;

	socket_addr.sin_family = adress_family;
	socket_addr.sin_port = htons(port);
	socket_addr.sin_addr.s_addr = inet_addr(ip.c_str());

	int result = ::bind(_socket, reinterpret_cast<SOCKADDR*>(&socket_addr), sizeof(socket_addr));
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("bind function failed with error ") + std::to_string(WSAGetLastError()));
}

void socket_wrapper::connect(u_short adress_family, u_short port, const std::string& ip) const {
	sockaddr_in socket_addr;

	socket_addr.sin_family = adress_family;
	socket_addr.sin_port = htons(port);
	socket_addr.sin_addr.s_addr = inet_addr(ip.c_str());

	const int result = ::connect(_socket, reinterpret_cast<SOCKADDR*>(&socket_addr), sizeof(socket_addr));
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("connect function failed with error ") + std::to_string(WSAGetLastError()));
}

void socket_wrapper::listen(int max_connections) const {
	const int result = ::listen(_socket, max_connections);
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("listen function failed with error ") + std::to_string(WSAGetLastError()));
}

socket_wrapper socket_wrapper::accept() const {
	SOCKET s = ::accept(_socket, NULL, NULL);

	if (s == INVALID_SOCKET)
		throw std::runtime_error(std::string("accept function failed with error ") + std::to_string(WSAGetLastError()));

	++_socket_count;

	return socket_wrapper(s);
}

int socket_wrapper::send(const char* buffer, int size) const {
	const int result = ::send(_socket, buffer, size, 0);
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("send function failed with error ") + std::to_string(WSAGetLastError()));

	return result;
}

int socket_wrapper::recv(char* buffer, int size) const {
	int result = ::recv(_socket, buffer, size, 0);
	if (result < 0)
		throw std::runtime_error(std::string("recv function failed with error ") + std::to_string(WSAGetLastError()));

	return result;
}

int socket_wrapper::select(int timeout_sec, int timeout_usec) const {
	// Set up the file descriptor set.
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(_socket, &fds);

	// Set up the struct timeval for the timeout.
	timeval tv;
	tv.tv_sec = timeout_sec;
	tv.tv_usec = timeout_usec;

	// Wait until timeout or data received.
	const int n = ::select(_socket, &fds, NULL, NULL, &tv);

	if (n == 0)  // timeout
		return -1;
	else if (n == SOCKET_ERROR)
		throw std::runtime_error(std::string("select function failed with error ") + std::to_string(WSAGetLastError()));
}

void socket_wrapper::shutdown(u_short type) const {
	const int result = ::shutdown(_socket, type);
	if (result == SOCKET_ERROR)
		throw std::runtime_error(std::string("shutdown function failed with error ") + std::to_string(WSAGetLastError()));
}
	
socket_wrapper::~socket_wrapper() {
	if (_socket != INVALID_SOCKET) {
		const int close_result = ::closesocket(_socket);
		if (close_result != 0)
			std::cerr << "closesocket function failed with error " << WSAGetLastError() << std::endl;

		--_socket_count;
	}

	if (_socket_count == 0) {
		const int clean_result = WSACleanup();
		if (clean_result)
			std::cerr << "wsacleanup function failed with error " << WSAGetLastError() << std::endl;
	}
}
