#include <iostream>

#include <WinSock2.h>


int main() {

	WSADATA wsaData;
	int result = 0;

	SOCKET connect_socket = INVALID_SOCKET;
	sockaddr_in socket_addr;

	char* local_ip = "127.0.0.1";
	int port = 7777;

	char buffer[] = "Hello, World!";

	// инициализация winsock
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != NO_ERROR) {
		std::cout << "WSAStartup failed with error " << result << std::endl;
		return 1;
	}

	// создание сокета для подключения к серверу
	connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect_socket == INVALID_SOCKET) {
		std::cout << "socket function failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// задание параметров сокета
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = htons(port);
	socket_addr.sin_addr.s_addr = inet_addr(local_ip);

	// подключение к серверу
	result = connect(connect_socket, (SOCKADDR*)&socket_addr, sizeof(socket_addr));
	if (result == SOCKET_ERROR) {
		std::cout << "connection failed with error " << WSAGetLastError() << std::endl;
		result = closesocket(connect_socket);
		if (result == SOCKET_ERROR)
			std::cout << "closesocket function failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
	else
		std::cout << "Connected!" << std::endl;

	// отправка данных на сервер
	result = send(connect_socket, buffer, (int)strlen(buffer), 0);
	if (result == SOCKET_ERROR) {
		std::cout << "send failed with error " << WSAGetLastError() << std::endl;
		result = closesocket(connect_socket);
		if (result == SOCKET_ERROR)
			std::cout << "closesocket function failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}
	else
		std::cout << "Bytes send: " << result << std::endl;

	// закрываем соединение
	result = shutdown(connect_socket, SD_BOTH);
	if (result == SOCKET_ERROR) {
		std::cout << "shutdown failed with error " << WSAGetLastError() << std::endl;
		result = closesocket(connect_socket);
		if (result == SOCKET_ERROR)
			std::cout << "closesocket function failed with error " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	closesocket(connect_socket);
	WSACleanup();

	system("pause");

	return 0;
}