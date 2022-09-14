#include <iostream>

#include <WinSock2.h>


int main() {

	WSADATA wsaData;
	int result = 0;

	SOCKET master_socket = INVALID_SOCKET;
	sockaddr_in socket_addr;

	char* local_ip = "127.0.0.1";
	int port = 7777;

	const int buffer_size = 128;
	char buffer[buffer_size];

	// инициализация winsock
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != NO_ERROR) {
		std::cout << "WSAStartup failed with error " << result << std::endl;
		return 1;
	}

	// создание сокета сервера
	master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (master_socket == INVALID_SOCKET) {
		std::cout << "socket function failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// задание параметров сокета
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = htons(port);
	socket_addr.sin_addr.s_addr = inet_addr(local_ip);

	// привязываем сокет
	result = bind(master_socket, (SOCKADDR*)&socket_addr, sizeof(socket_addr));
	if (result == SOCKET_ERROR) {
		std::cout << "bind failed with error " << WSAGetLastError() << std::endl;
		result = closesocket(master_socket);
		if (result == SOCKET_ERROR)
			std::cout << "closesocket function failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
	else
		std::cout << "bind successful" << std::endl;

	// прослушиваем входящие запросы на сокет
	result = listen(master_socket, SOMAXCONN);
	if (result == SOCKET_ERROR) {
		std::cout << "listen function failed with error " << WSAGetLastError() << std::endl;
		result = closesocket(master_socket);
		if (result == SOCKET_ERROR)
			std::cout << "closesocket function failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
	else
		std::cout << "listening on socket..." << std::endl;

	// сокет для обработки входящих соединений
	auto slave_socket = INVALID_SOCKET;
	std::cout << "waiting for client to connect..." << std::endl;

	// принять соединение
	slave_socket = accept(master_socket, NULL, NULL);
	if (slave_socket == INVALID_SOCKET) {
		std::cout << "accept function failed with error " << WSAGetLastError() << std::endl;
		result = closesocket(master_socket);
		if (result == SOCKET_ERROR)
			std::cout << "closesocket function failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
	else
		std::cout << "client connected"<< std::endl;

	// получаем данные пока клиент не закроет соединение
	do {

		result = recv(slave_socket, buffer, buffer_size, 0);
		if (result > 0)
			std::cout << "Bytes received: " << result << std::endl;
		else if (result == 0)
			std::cout << "Connection closed" << std::endl;
		else
			std::cout << "recv failed with error " << WSAGetLastError() << std::endl;

		std::cout << std::string(buffer, result) << std::endl;
		for (int i = 0; i < buffer_size; ++i)
			buffer[i] = '\0';

	} while (result > 0);

	//shutdown(slave_socket, SD_BOTH);
	closesocket(slave_socket);
	
	closesocket(master_socket);
	WSACleanup();

	system("pause");

	return 0;
}