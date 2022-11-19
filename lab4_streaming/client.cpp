#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include "utils/socket/socket.h"

const int16_t CLIENT_PORT = 7777;
const std::string LOCAL_IP = "127.0.0.1";
std::string buffer;
std::mutex mutex;

void server_connection() {
	const socket_wrapper server_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	server_socket.connect(address_family::IPV4, CLIENT_PORT, LOCAL_IP);
	std::cout << "Connect successful" << std::endl;

	while (true) {
		char received[8192];
		const int received_bytes = server_socket.recv(received, 8192);
		mutex.lock();
		buffer.clear();
		for (int i = 0; i < received_bytes; ++i) {
			buffer.push_back(received[i]);
		}
		std::cout << "Received: " << buffer << std::endl;
		mutex.unlock();
	}
}

void show() {
	return;
}

int main() {
	std::thread server(server_connection);
	server.detach();

	while (true) {
		;
	}
}