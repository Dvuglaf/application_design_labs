#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "utils/socket/socket.h"
#include <opencv2\opencv.hpp>

using namespace std::chrono_literals;

const int16_t REPEATER_PORT = 7778;
static std::string SERVER_IP;


int main(int argc, char* argv[]) try {
	if (argc != 2) {
		throw std::invalid_argument("Enter 1 argument: SERVER_IP");
	}
	SERVER_IP = argv[1];

	cv::Mat frame;
	cv::VideoCapture webcam(0);
	
	const socket_wrapper server_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	server_socket.connect(address_family::IPV4, REPEATER_PORT, SERVER_IP);
	std::cout << "Connect successful" << std::endl;

	while (true) {
		webcam >> frame;
		std::vector<uchar> encoded_image;
		cv::imencode(".jpg", frame, encoded_image);  // jpeg frame compression
		int frame_size = encoded_image.size();
		
		server_socket.send(reinterpret_cast<char*>(&frame_size), sizeof(int));
		server_socket.send(reinterpret_cast<char*>(&encoded_image[0]), frame_size);
		std::this_thread::sleep_for(80ms);  // 15 FPS 
	}
}
catch (std::exception& e) {
	std::cout << e.what() << std::endl;
}