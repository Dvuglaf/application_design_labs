#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include "utils/socket/socket.h"

const int16_t REPEATER_PORT = 7778;
const std::string LOCAL_IP = "127.0.0.1";
std::string buffer;
std::mutex mutex;

using namespace std::chrono_literals;


void server_connection() {
	const socket_wrapper server_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	server_socket.connect(address_family::IPV4, REPEATER_PORT, LOCAL_IP);
	std::cout << "Connect successful" << std::endl;

	while (true) {
		mutex.lock();
		server_socket.send(buffer.c_str(), buffer.size());
		std::this_thread::sleep_for(1s);
		std::cout << "SEND: " << buffer << std::endl;
		mutex.unlock();
	}
}

void get_frame() {
	int i = 0;
	while (true) {
		mutex.lock();
		buffer = std::to_string(i);
		mutex.unlock();
		++i;
	}
}

int main() {
	std::thread server(server_connection);
	server.detach();
	std::thread frame(get_frame);
	frame.detach();

	while (true) {
		;
	}
}