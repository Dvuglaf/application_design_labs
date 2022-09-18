#include <iostream>
#include <string>
#include <WinSock2.h>
#include "socket.h"
#include "utils.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>


// Receive image size from client ("SIZE_1 SIZE_2")
std::vector<int> get_image_size(const socket_wrapper& socket) {
	std::vector<int> image_size;

	char buffer[16];
	const int received_bytes = socket.recv(buffer, 16);

	std::cout << "\tReceived " << received_bytes << " bytes\n";

	auto values = my_utils::split(std::string(buffer), " ");

	image_size.push_back(std::stoi(values[0]));
	image_size.push_back(std::stoi(values[1]));

	return image_size;
}

int main() {
	try {
		WSADATA wsaData;

		const std::string local_ip = "127.0.0.1";
		const int port = 7777;

		const int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

		if (result != NO_ERROR)
			throw std::runtime_error(std::string("WSAStartup failed with error ") + std::to_string(WSAGetLastError()));


		const socket_wrapper master_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		master_socket.bind(AF_INET, port, local_ip);
		std::cout << "Bind successful" << std::endl;

		master_socket.listen(SOMAXCONN);
		std::cout << "Listening on socket..." << std::endl;

		const socket_wrapper slave_socket = master_socket.accept();
		std::cout << "Client connected" << std::endl;

		const auto image_size = get_image_size(slave_socket); // receive image size

		std::vector<uchar> image_buffer;
		int received_bytes = 0;

		do {
			char received[8192];
			received_bytes = slave_socket.recv(received, 8192);

			if (received_bytes > 0)
				std::cout << "\tReceived " << received_bytes << " bytes\n";

			else if (received_bytes == 0)
				std::cout << "Connection closed" << std::endl;

			// Accumulate data in image_buffer
			for (size_t i = 0; i < received_bytes; ++i)
				image_buffer.push_back(received[i]);

		} while (received_bytes > 0);

		// Decoding jpeg and denoising noised image

		cv::InputArray data(image_buffer);
		cv::Mat noised_image = cv::imdecode(data, cv::IMREAD_COLOR); // decode jpeg encoded immage

		cv::Mat denoised_image(noised_image.size(), noised_image.type());

		std::cout << "\nDenoising image...\n\n";

		cv::fastNlMeansDenoisingColored(noised_image, denoised_image); // denoising image using median filter

		std::cout << "\nDone\n\n";

		cv::imshow("Denoised Image", denoised_image);
		cv::waitKey(0);
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
		WSACleanup();
	}

	WSACleanup();

	return 0;
}
