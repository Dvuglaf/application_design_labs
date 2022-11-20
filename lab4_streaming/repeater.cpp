#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <mutex>
#include "utils/socket/socket.h"
#include <opencv2\opencv.hpp>



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
		std::cout << "server" << "\n";
		if (buffer.size() == 0) {
			mutex.unlock();
			std::this_thread::sleep_for(10ms);
			continue;
		}

		const std::string send_buffer = std::to_string(buffer.size()) + ";" + buffer;
		const int bytes_sent = server_socket.send(send_buffer.c_str(), send_buffer.size());
		std::cout << "Sent: " << bytes_sent << " bytes\n";
		std::string check;
		while (check != "requested") {
			check.clear();
			char temp[16];
			int rec = server_socket.recv(temp, 16);
			for (int i = 0; i < rec; ++i) {
				check.push_back(temp[i]);
			}
		}
		mutex.unlock();

	}
}

void get_frame() {
	cv::Mat frame;
	cv::VideoCapture webcam(0);
	while (true) {
		webcam >> frame;
		std::vector<uchar> encoded_image;
		cv::imencode(".jpg", frame, encoded_image); // jpeg frame compression
		mutex.lock();
		buffer.clear();
		for (size_t i = 0; i < encoded_image.size(); ++i) {
			buffer.push_back(encoded_image[i]);
		}
		mutex.unlock();
		//std::this_thread::sleep_for(10ms);
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