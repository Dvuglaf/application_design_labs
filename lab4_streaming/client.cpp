#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include "utils/socket/socket.h"
#include <opencv2/opencv.hpp>

const int16_t CLIENT_PORT = 7777;
const std::string LOCAL_IP = "127.0.0.1";

std::vector<uchar> frame_encoded;
std::mutex mutex;


void show_frame() {
	while (true) {
		cv::Mat frame;
		mutex.lock();
		if (!frame_encoded.empty()) {
			frame = cv::imdecode(frame_encoded, cv::IMREAD_COLOR); // decode jpeg encoded frame
			frame_encoded.clear();
		}
		else {
			mutex.unlock();
			continue;
		}
		mutex.unlock();
		std::cout << "Decoded fram size: " << frame.size() << std::endl;
		cv::imshow("Display window", frame);
		cv::waitKey(1);
	}
}


int main() {
	const socket_wrapper server_socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	server_socket.connect(address_family::IPV4, CLIENT_PORT, LOCAL_IP);
	std::cout << "Connect successful" << std::endl;

	std::thread show(show_frame);
	show.detach();

	while (true) {
		int frame_size = 0;
		int received_bytes = server_socket.recv((char*)&frame_size, sizeof(int)); // size
		std::cout << "Received frame size: " << received_bytes << " bytes\n";
		char* received = new char[frame_size];
		received_bytes = server_socket.recv(received, frame_size); // frame
		std::cout << "Received frame: " << received_bytes << " bytes\n";
		if (received_bytes < frame_size) {
			std::cout << "\n\n\n\n" << "Failed to receive full frame. Skipping this frame ..." << "\n\n\n\n";
			char* lost_data = new char[frame_size - received_bytes];
			received_bytes = server_socket.recv(lost_data, frame_size - received_bytes); // lost frame
			delete[] lost_data;
			delete[] received;
			continue;
		}

		mutex.lock();
		frame_encoded.reserve(received_bytes);
		for (int i = 0; i < received_bytes; ++i) {
			frame_encoded.push_back(received[i]);
		}
		mutex.unlock();

		delete[] received;
		
	}
}