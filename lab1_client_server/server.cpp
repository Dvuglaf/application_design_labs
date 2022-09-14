#include <iostream>
#include <string>
#include <WinSock2.h>
#include "socket.h"

int wmain() {

	WSADATA wsaData;

	const std::string local_ip = "127.0.0.1";
	const int port = 7777;

	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != NO_ERROR) {
		std::cout << "WSAStartup failed with error " << result << std::endl;
		return 1;
	}

	try {
		socket_wrapper master_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		master_socket.bind(AF_INET, port, local_ip);
		master_socket.listen(SOMAXCONN);

		socket_wrapper slave_socket = std::move(master_socket.accept());

		std::string recieved;

		do {
			recieved = slave_socket.recv();

			if (recieved.size() > 0)
				std::cout << "Bytes received: " << recieved.size() << std::endl;
			else if (recieved.size() == 0)
				std::cout << "Connection closed" << std::endl;

			std::cout << std::string(recieved.c_str(), recieved.size()) << std::endl;
			recieved.clear();

		} while (recieved.size() > 0);
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	WSACleanup();
	system("pause");

	return 0;
}