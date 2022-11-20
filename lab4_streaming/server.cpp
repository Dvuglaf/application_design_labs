#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include "utils/socket/socket.h"

const int16_t CLIENT_PORT = 7777;
const int16_t REPEATER_PORT = 7778;
const std::string LOCAL_IP = "127.0.0.1";
std::mutex mutex;

int main() {
	const socket_wrapper master_socket_1(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	master_socket_1.bind(address_family::IPV4, REPEATER_PORT, LOCAL_IP);
	std::cout << "[repeater]Bind successful" << std::endl;

	master_socket_1.listen(SOMAXCONN);
	std::cout << "[repeater]Listening on socket..." << std::endl;

	const socket_wrapper repeater_socket = master_socket_1.accept();
	std::cout << "[repeater]Client connected" << std::endl;

	const socket_wrapper master_socket_2(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	master_socket_2.bind(address_family::IPV4, CLIENT_PORT, LOCAL_IP);
	std::cout << "[client]Bind successful" << std::endl;

	master_socket_2.listen(SOMAXCONN);
	std::cout << "[client]Listening on socket..." << std::endl;

	const socket_wrapper client_socket = master_socket_2.accept();
	std::cout << "[client]Client connected" << std::endl;

	while (true) {
		int frame_size = 0;
		int received_bytes = repeater_socket.recv((char*)&frame_size, sizeof(int)); // size
		std::cout << "Received frame size: " << received_bytes << " bytes\n";
		std::cout << "Received frame size: " << frame_size << "\n";
		char* received = new char[frame_size];
		received_bytes = repeater_socket.recv(received, frame_size); // frame
		std::cout << "Received frame: " << received_bytes << " bytes\n";
		client_socket.send((char*)&frame_size, sizeof(int));
		client_socket.send(received, frame_size);
		delete[] received;
	}
}