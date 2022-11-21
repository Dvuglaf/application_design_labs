#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include "utils/socket/socket.h"

using namespace std::chrono_literals;

const int16_t CLIENT_PORT = 7777;
const int16_t REPEATER_PORT = 7778;
static std::string SERVER_IP;

const std::string HTTP_HEADER = "HTTP/1.0 200 OK\r\n";
const std::string HTTP_PARAMS = "Server: Mozarella/2.2\r\n"
					   			"Accept-Range: bytes\r\n"
								"Connection: close\r\n"
								"Content-Type: multipart/x-mixed-replace; boundary=mjpegstream\r\n"
								"\r\n";

std::mutex mutex;
std::vector<socket_wrapper> clients;


void client_connections() {
	const socket_wrapper master_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);
	master_socket.bind(address_family::IPV4, CLIENT_PORT, SERVER_IP);
	master_socket.listen(SOMAXCONN);

	while (true) {
		socket_wrapper client_socket = master_socket.accept();
		std::cout << "New client connected" << std::endl;

		mutex.lock();
		client_socket.send((HTTP_HEADER + HTTP_PARAMS).c_str(), (HTTP_HEADER + HTTP_PARAMS).size());
		clients.push_back(std::move(client_socket));
		mutex.unlock();
	}
}

int main(int argc, char* argv[]) try {
	if (argc != 2) {
		throw std::invalid_argument("Enter 1 argument: SERVER_IP");
	}
	SERVER_IP = argv[1];

	const socket_wrapper master_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	master_socket.bind(address_family::IPV4, REPEATER_PORT, SERVER_IP);
	master_socket.listen(SOMAXCONN);

	const socket_wrapper repeater_socket = master_socket.accept();
	std::cout << "Repeater connected" << std::endl;

	std::thread clients_connect(client_connections);
	clients_connect.detach();

	while (true) {
		int frame_size = 0;
		repeater_socket.recv(reinterpret_cast<char*>(&frame_size), sizeof(int));  // size

		char* received = new char[frame_size];
		try {
			const int received_bytes = repeater_socket.recv(received, frame_size);  // frame

			if (received_bytes < frame_size) {  // receive the remaining data from the buffer and skip frame
				char* lost_data = new char[frame_size - received_bytes];
				try {
					repeater_socket.recv(lost_data, frame_size - received_bytes);  // lost frame
				}
				catch (std::exception&) {
					delete[] lost_data;
					throw;
				}
				delete[] lost_data;
				delete[] received;
				continue;
			}

			mutex.lock();
			for (auto it = clients.begin(); it < clients.end(); ++it) {
				int result = it->select(0, 1000);  // check if socket is nonactive
				if (result == -1) {
					clients.erase(it);
					break;
				}
				try {
					std::string body = "--mjpegstream\r\nContent - Type: image / jpeg\r\nContent - Length: " + 
						std::to_string(frame_size) + "\r\n\r\n";
					it->send(body.c_str(), body.size());
					it->send(received, frame_size);
				}
				catch (std::exception&) {  // if client stop responding
					std::cout << "Client disconnected" << std::endl;
					clients.erase(it);
					break;
				}
			}
		}
		catch (std::exception&) {
			delete[] received;
			throw;
		}
		mutex.unlock();
		delete[] received;
	}
}
catch (std::exception& e) {
	std::cout << e.what() << std::endl;
}