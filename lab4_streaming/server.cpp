#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include "utils/socket/socket.h"
#include "utils/utils.hpp"

const int16_t CLIENT_PORT = 7777;
const int16_t REPEATER_PORT = 7778;
const std::string LOCAL_IP = "127.0.0.1";
std::mutex mutex;
std::string buffer;

using namespace std::chrono_literals;



void client_connection() {
	const socket_wrapper master_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	master_socket.bind(address_family::IPV4, CLIENT_PORT, LOCAL_IP);
	std::cout << "[client]Bind successful" << std::endl;

	master_socket.listen(SOMAXCONN);
	std::cout << "[client]Listening on socket..." << std::endl;

	const socket_wrapper client_socket = master_socket.accept();
	std::cout << "[client]Client connected" << std::endl;

	while (true) {
		mutex.lock();
		client_socket.send(buffer.c_str(), buffer.size());
		std::this_thread::sleep_for(1s);
		std::cout << "[client]Send: " << buffer << std::endl;
		mutex.unlock();
	}
}

void repeater_connection() {
	const socket_wrapper master_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	master_socket.bind(address_family::IPV4, REPEATER_PORT, LOCAL_IP);
	std::cout << "[repeater]Bind successful" << std::endl;

	master_socket.listen(SOMAXCONN);
	std::cout << "[repeater]Listening on socket..." << std::endl;

	const socket_wrapper repeater_socket = master_socket.accept();
	std::cout << "[repeater]Client connected" << std::endl;

	while (true) {

		char received[8192];
		const int received_bytes = repeater_socket.recv(received, 8192);
		mutex.lock();
		buffer.clear();
		for (int i = 0; i < received_bytes; ++i) {
			buffer.push_back(received[i]);
		}
		std::cout << "[repeater]Received: " << buffer << std::endl;
		mutex.unlock();
	}
}


int main() {
	std::thread client(client_connection);
	client.detach();
	std::thread repeater(repeater_connection);
	repeater.detach();

	while (true) {
		;
	}
	return 0;
}