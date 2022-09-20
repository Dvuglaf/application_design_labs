#include <iostream>
#include <WinSock2.h>
#include "socket.h"
#include "utils.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>


int main(int argc, char* argv[]) {
	try {
		using namespace cv;
		using namespace my_utils;

		// Check number of arguments: first - programm name, second - path
		if (argc != 2) {
			throw std::invalid_argument("Error: enter only a path to image");
		}

		const std::string path = argv[1];

		auto image = read_image(path);

		WSADATA wsaData;
		const int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

		if (result != NO_ERROR)
			throw std::runtime_error(std::string("WSAStartup failed with error ") + std::to_string(WSAGetLastError()));

		const std::string local_ip = "127.0.0.1";
		const int port = 7777;

		const socket_wrapper connect_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		connect_socket.connect(AF_INET, port, local_ip);
		std::cout << "Connect successful" << std::endl;

		// Firstly send image size ("SIZE_1 SIZE_2")

		std::string image_size = std::to_string(image.rows) + " " + std::to_string(image.cols);

		const int bytes_sent_size = connect_socket.send(image_size.c_str(), static_cast<int>(image_size.size()));
		std::cout << "\tSend " << bytes_sent_size << " bytes\n";

		// Secondly send jpeg compressed noised image with additive noise (white)

		const std::vector<float> m = { 10, 12, 11 }; // noise means of each channel
		const std::vector<float> sigma = { 20, 20, 20 }; // noise mse f each channel

		cv::Mat noised_image(image.size(), image.type());

		cv::randn(noised_image, m, sigma); // generating noise
		noised_image += image; // additive

		std::vector<uchar> image_buffer;
		cv::imencode(".jpg", noised_image, image_buffer); // jpeg image compression

		const int bytes_sent_image = connect_socket.send(reinterpret_cast<const char*>(&image_buffer[0]),
			static_cast<int>(image_buffer.size()));

		std::cout << "\tSend " << bytes_sent_image << " bytes\n";

		connect_socket.shutdown();
		std::cout << "Shutdown connection\n\n";

		cv::imshow("Noised Image", noised_image);
		waitKey(0);

	}
	catch (const std::exception& e) {
		std::cout << e.what();
		WSACleanup();
	}
	
	WSACleanup();
}

// TODO: impulse noise