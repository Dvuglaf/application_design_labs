#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include "utils/socket/socket.h"

const int16_t CLIENT_PORT = 7777;
const int16_t REPEATER_PORT = 7778;
const std::string LOCAL_IP = "127.0.0.1";
std::mutex mutex;
std::vector<socket_wrapper> clients;

void client_connections() {
	while (true) {
		const socket_wrapper master_socket_2(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);
		master_socket_2.bind(address_family::IPV4, CLIENT_PORT, LOCAL_IP);
		master_socket_2.listen(SOMAXCONN);

		socket_wrapper client_socket = master_socket_2.accept();
		std::cout << "New client connected" << std::endl;
		mutex.lock();
		clients.push_back(std::move(client_socket));
		mutex.unlock();
	}
}

int main() {
	const socket_wrapper master_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	master_socket.bind(address_family::IPV4, REPEATER_PORT, LOCAL_IP);
	master_socket.listen(SOMAXCONN);

	const socket_wrapper repeater_socket = master_socket.accept();
	std::cout << "Repeater connected" << std::endl;

	std::thread clients_connect(client_connections);
	clients_connect.detach();

	while (true) {
		int frame_size = 0;
		int received_bytes = repeater_socket.recv((char*)&frame_size, sizeof(int)); // size
		std::cout << "Received frame size: " << received_bytes << " bytes\n";
		std::cout << "Received frame size: " << frame_size << "\n";
		char* received = new char[frame_size];
		received_bytes = repeater_socket.recv(received, frame_size); // frame
		std::cout << "Received frame: " << received_bytes << " bytes\n";
		
		mutex.lock();
		for (const auto& client: clients) {
			client.send((char*)&frame_size, sizeof(int));
			client.send(received, frame_size);
		}
		mutex.unlock();
		delete[] received;
	}
}