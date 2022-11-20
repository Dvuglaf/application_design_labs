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
		std::cout << "Begin\n";
		mutex.lock();
		const std::string send_buffer = std::to_string(buffer.size()) + ";" + buffer;
		mutex.unlock();
		const int bytes_sent = client_socket.send(send_buffer.c_str(), send_buffer.size());
		std::cout << "Sent: " << bytes_sent << " bytes\n";


		std::string check;
		while (check != "requested") {
			check.clear();
			char temp[16];
			int rec = client_socket.recv(temp, 16);
			for (int i = 0; i < rec; ++i) {
				check.push_back(temp[i]);
			}
			std::cout << check << "\n";
		}

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
		std::cout << "repeater\n";
		int total_received_bytes = 0;

		char* received = new char[65000];
		const int received_bytes = repeater_socket.recv(received, 65000); // size + frame
		std::string size;

		int i = 0;
		for (; i < received_bytes; ++i) {
			if (received[i] != ';') {
				size += received[i];
			}
			else { 
				++i; 
				break; 
			}
		}

		mutex.lock();
		buffer.clear();
		buffer.reserve(received_bytes);
		for (; i < received_bytes; ++i) {
			buffer += received[i];
		}
		delete[] received;
		//std::cout << buffer.size() << "\t" << size + "\n";

		if (received_bytes < std::stoi(size)) {
			int bytes_left = std::stoi(size) - received_bytes - size.size() - 1;
			while (bytes_left > 0) {
				int received_size = 65000 > bytes_left ? 65000 : bytes_left;
				char* received = new char[received_size];
				const int received_bytes = repeater_socket.recv(received, received_size); // frame
				buffer.reserve(buffer.capacity() + received_bytes);
				for (int i = 0; i < received_bytes; ++i) {
					buffer.push_back(received[i]);
				}
				delete[] received;
				bytes_left -= received_bytes;
				//std::cout << buffer.size() << "\t" << size + "\n";
			}
		}
		mutex.unlock();
		std::string temp = "requested";
		repeater_socket.send(temp.c_str(), temp.size());
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