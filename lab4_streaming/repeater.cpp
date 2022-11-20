#include <iostream>
#include <string>
#include <vector>
#include "utils/socket/socket.h"
#include <opencv2\opencv.hpp>

const int16_t REPEATER_PORT = 7778;
const std::string LOCAL_IP = "127.0.0.1";

int main() {
	cv::Mat frame;
	cv::VideoCapture webcam(0);
	
	const socket_wrapper server_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	server_socket.connect(address_family::IPV4, REPEATER_PORT, LOCAL_IP);
	std::cout << "Connect successful" << std::endl;

	while (true) {
		webcam >> frame;
		std::vector<uchar> encoded_image;
		cv::imencode(".jpg", frame, encoded_image); // jpeg frame compression
		int frame_size = encoded_image.size();
		int bytes_sent = server_socket.send((char*)&frame_size, sizeof(int));
		std::cout << "Sent frame size: " << bytes_sent << " bytes\n";
		bytes_sent = server_socket.send((char*)&encoded_image[0], frame_size);
		std::cout << "Sent frame: " << bytes_sent << " bytes\n";
	}
}