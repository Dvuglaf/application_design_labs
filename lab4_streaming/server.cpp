#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include "utils/socket/socket.h"
#include <chrono>

using namespace std::chrono_literals;

const int16_t CLIENT_PORT = 7777;
const int16_t REPEATER_PORT = 7778;
const std::string LOCAL_IP = "192.168.1.2";
std::mutex mutex;
std::vector<socket_wrapper> clients;

std::string header = "HTTP/1.0 200 OK\r\n";
std::string params = "Server: Mozarella/2.2\r\n"
"Accept-Range: bytes\r\n"
"Connection: close\r\n"
"Content-Type: multipart/x-mixed-replace; boundary=mjpegstream\r\n"
"\r\n";

void client_connections() {

	const socket_wrapper master_socket_2(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);
	master_socket_2.bind(address_family::IPV4, CLIENT_PORT, LOCAL_IP);
	master_socket_2.listen(SOMAXCONN);

	while (true) {
		socket_wrapper client_socket = master_socket_2.accept();
		std::cout << "New client connected" << std::endl;
		mutex.lock();
		client_socket.send((header + params).c_str(), (header + params).size());
		clients.push_back(std::move(client_socket));
		mutex.unlock();
	}
}

int main() try {
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
		/*std::cout << "Received frame size: " << received_bytes << " bytes\n";
		std::cout << "Received frame size: " << frame_size << "\n";*/
		char* received = new char[frame_size];
		received_bytes = repeater_socket.recv(received, frame_size); // frame
		/*std::cout << "Received frame: " << received_bytes << " bytes\n";*/
		if (received_bytes < frame_size) {
			/*std::cout << "\n\n\n\n" << "Failed to receive full frame. Skipping this frame ..." << "\n\n\n\n";*/
			char* lost_data = new char[frame_size - received_bytes];
			received_bytes = repeater_socket.recv(lost_data, frame_size - received_bytes); // lost frame
			delete[] lost_data;
			delete[] received;
			continue;
		}

		mutex.lock();
		for (auto it = clients.begin(); it < clients.end(); ++it) {
			int result = it->select(0, 1000);
			if (result == -1) {
				clients.erase(it);
				break;
			}
			try {
				std::string body = "--mjpegstream\r\nContent - Type: image / jpeg\r\nContent - Length: " + std::to_string(frame_size) + "\r\n\r\n";
				it->send(body.c_str(), body.size());
				it->send(received, frame_size);
			}
			catch (std::exception& e) {
				//std::cout << e.what() << std::endl;
				std::cout << "Client disconnected" << std::endl;
				clients.erase(it);
				break;
			}
		}
		mutex.unlock();
		delete[] received;
	}
}
catch (std::exception& e) {
	std::cout << e.what() << std::endl;
}