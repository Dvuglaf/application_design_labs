#include <iostream>
#include <string>
#include <thread>
#include <regex>
#include <mutex>
#include "utils/socket/socket.h"
#include <opencv2/opencv.hpp>

const int16_t CLIENT_PORT = 7777;
static std::string SERVER_IP;

std::vector<uchar> frame_encoded;
std::mutex mutex;


void show_frame() {
	while (true) {
		cv::Mat frame;
		mutex.lock();
		if (frame_encoded.empty()) {
			mutex.unlock();
			continue;
		}
		frame = cv::imdecode(frame_encoded, cv::IMREAD_COLOR); // decode jpeg encoded frame
		frame_encoded.clear();
		mutex.unlock();

		cv::imshow("Camera", frame);
		cv::waitKey(1);
	}
}

int main(int argc, char* argv[]) try {
	if (argc != 2) {
		throw std::invalid_argument("Enter 1 argument: SERVER_IP");
	}
	SERVER_IP = argv[1];

	const socket_wrapper server_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	server_socket.connect(address_family::IPV4, CLIENT_PORT, SERVER_IP);
	std::cout << "Connect to server successful" << std::endl;

	std::thread show(show_frame);
	show.detach();

	server_socket.send("HTTP/1.0 GET", 13);

	char received_header[150];
	server_socket.recv(received_header, 150);  // header

	while (true) {
		char received_length[100];
		int received_bytes = server_socket.recv(received_length, 100);  // length
		std::string length_str(received_length, received_bytes);
		std::smatch match;
		std::regex_search(length_str, match, std::regex("\\d+"));

		int length = std::stoi(match.str(0));

		char* received = new char[length];
		try {
			received_bytes = server_socket.recv(received, length);  // frame
			std::cout << "Received frame: " << received_bytes << " bytes" << std::endl;
			if (received_bytes < length) {  // skip frame
				delete[] received;
				continue;
			}

			mutex.lock();
			frame_encoded.reserve(received_bytes);
			for (int i = 0; i < received_bytes; ++i) {
				frame_encoded.push_back(received[i]);
			}
			mutex.unlock();
		} 
		catch (std::exception&) {
			delete[] received;
			throw;
		}

		delete[] received;
	}
}
catch (std::exception& e) {
	std::cout << e.what() << std::endl;
}