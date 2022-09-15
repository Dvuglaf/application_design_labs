#include <iostream>
#include <string>
#include <WinSock2.h>
#include "socket.h"


int main() {

	WSADATA wsaData;

	const std::string local_ip = "127.0.0.1";
	const int port = 7777;

	const int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != NO_ERROR) {
		std::cout << "WSAStartup failed with error " << result << std::endl;
		return 1;
	}

	try {
		const socket_wrapper master_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		master_socket.bind(AF_INET, port, local_ip);
		master_socket.listen(SOMAXCONN);

		const socket_wrapper slave_socket = master_socket.accept();

		std::string received;
		size_t received_bytes = 0;

		do {
			received = slave_socket.recv(256);
			received_bytes = received.size();

			if (received_bytes > 0) {
				std::cout << "\nBytes received: " << received_bytes << std::endl;
				std::cout << "Received: " << received << std::endl;
			}
			else if (received_bytes == 0)
				std::cout << "\nconnection closed" << std::endl;

			received.clear();

		} while (received_bytes > 0);
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	WSACleanup();
	system("pause");

	return 0;
}