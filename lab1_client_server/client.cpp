#include <iostream>
#include <WinSock2.h>
#include "socket.h"


int main() {

	WSADATA wsaData;

	const std::string local_ip = "127.0.0.1";
	const int port = 7777;

	const std::string buffer = "Hello, World!";

	const int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != NO_ERROR) {
		std::cout << "WSAStartup failed with error " << result << std::endl;
		return 1;
	}

	try {
		const socket_wrapper connect_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		connect_socket.connect(AF_INET, port, local_ip);

		connect_socket.send(buffer);
		std::cout << "Send: " << buffer << std::endl;
		
		connect_socket.shutdown();
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}

	WSACleanup();
	system("pause");

	return 0;
}