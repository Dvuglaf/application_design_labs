#pragma once
#include <WinSock2.h>
#include <string>

class socket_wrapper {
	SOCKET _socket;
public:
	socket_wrapper();
	
	explicit socket_wrapper(SOCKET);

	socket_wrapper(const socket_wrapper&) = delete;

	socket_wrapper& operator=(const socket_wrapper&) = delete;

	socket_wrapper(socket_wrapper&&) noexcept;

	socket_wrapper& operator=(socket_wrapper&&) noexcept;

	explicit socket_wrapper(int, int, int);

	void bind(u_short, u_short, const std::string&) const ;

	void connect(u_short, u_short, const std::string&) const;

	void listen(int) const;

	socket_wrapper accept() const;

	void send(const std::string&) const;

	std::string recv() const;

	void shutdown() const;

	~socket_wrapper() noexcept;

};