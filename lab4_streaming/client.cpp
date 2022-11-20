#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include "utils/socket/socket.h"
#include <opencv2/opencv.hpp>

using namespace std::chrono_literals;

const int16_t CLIENT_PORT = 7777;
const std::string LOCAL_IP = "127.0.0.1";
std::string buffer;
std::mutex mutex;


void server_connection() {
	const socket_wrapper server_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	server_socket.connect(address_family::IPV4, CLIENT_PORT, LOCAL_IP);
	std::cout << "Connect successful" << std::endl;

	while (true) {
		int total_received_bytes = 0;

		char* received = new char[65000];
		const int received_bytes = server_socket.recv(received, 65000); // size + frame
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
				const int received_bytes = server_socket.recv(received, received_size); // frame
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
		std::cout << "RECEIVED\n";
		std::string temp = "requested";
		server_socket.send(temp.c_str(), temp.size());
	}
}

void stream() {
	//cv::namedWindow("Display window");
	while (true) {
		mutex.lock();
		if (buffer.size() == 0) {
			mutex.unlock();
			//std::this_thread::sleep_for(10ms);
			continue;
		}
		cv::Mat frame = cv::imdecode(std::vector<uchar>(buffer.begin(), buffer.end()), cv::IMREAD_COLOR); // decode jpeg encoded frame
		mutex.unlock();
		//std::cout << "UPDATE\n";
		//cv::imshow("Display window", frame);
		//cv::waitKey(25);
		//std::this_thread::sleep_for(10ms);
	}
}

int main() {
	std::thread server(server_connection);
	server.detach();
	std::thread show(stream);
	show.detach();

	while (true) {
		;
	}
}