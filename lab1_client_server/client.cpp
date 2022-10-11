#include <iostream>
#include <WinSock2.h>
#include "socket.h"
#include "utils.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>


cv::Mat add_impulse_noise(const cv::Mat& image, const float intensity) {
	cv::Mat saltpepper_noise = cv::Mat::zeros(image.rows, image.cols, CV_8U);
	cv::randu(saltpepper_noise, 0, 255);

	const int intensities[2] = {
		static_cast<int>(0. + std::round(255 * intensity / 2)),
		static_cast<int>(255. - std::round(255 * intensity / 2))
	};

	cv::Mat black = saltpepper_noise < intensities[0];
	cv::Mat white = saltpepper_noise > intensities[1];

	cv::Mat noised_image = image.clone();
	noised_image.setTo(255, white);
	noised_image.setTo(0, black);

	return noised_image;
}

cv::Mat add_gaussian_noise(const cv::Mat& image) {
	cv::Mat noised_image(image.size(), image.type());

	const std::vector<float> m = { 10, 12, 11 }; // noise means of each channel
	const std::vector<float> sigma = { 20, 20, 20 }; // noise mse f each channel

	cv::randn(noised_image, m, sigma); // generating noise
	noised_image += image; // additive

	return noised_image;
}

int main(int argc, char* argv[]) {
	try {
		// Check number of arguments: first - programm name, second - path, third - intensivity of s&p noise
		if (argc != 3)
			throw std::invalid_argument("Error: enter only a path to image");

		const std::string path = argv[1];

		auto image = read_image(path);

		const std::string local_ip = "127.0.0.1";
		const int port = 7777;

		const socket_wrapper connect_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

		connect_socket.connect(address_family::IPV4, port, local_ip);
		std::cout << "Connect successful" << std::endl;

		// Firstly send image size ("SIZE_1 SIZE_2")
		std::string image_size = std::to_string(image.rows) + " " + std::to_string(image.cols);

		const int bytes_sent_size = connect_socket.send(image_size.c_str(), static_cast<int>(image_size.size()));
		std::cout << "\tSend " << bytes_sent_size << " bytes\n";

		// Secondly send jpeg compressed noised image with impulse noise (s&p)
		const float intensity = std::stof(argv[2]);

		cv::Mat noised_image = add_impulse_noise(image, intensity);

		std::vector<uchar> image_buffer;
		cv::imencode(".jpg", noised_image, image_buffer); // jpeg image compression

		const int bytes_sent_image = connect_socket.send(reinterpret_cast<const char*>(&image_buffer[0]),
			static_cast<int>(image_buffer.size()));

		std::cout << "\tSend " << bytes_sent_image << " bytes\n";

		connect_socket.shutdown(shutdown_type::BOTH);
		std::cout << "Shutdown connection\n\n";

		cv::namedWindow("Noised Image", cv::WINDOW_NORMAL);
		cv::imshow("Noised Image", noised_image);
		cv::waitKey(0);

	}
	catch (const std::exception& e) {
		std::cout << e.what();
	}

	return 0;
}
