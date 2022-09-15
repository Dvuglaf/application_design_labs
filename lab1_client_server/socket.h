#pragma once
#include <WinSock2.h>
#include <string>


class socket_wrapper {
public:
	socket_wrapper() = delete;
	
	explicit socket_wrapper(SOCKET);
	explicit socket_wrapper(int, int, int);

	socket_wrapper(const socket_wrapper&) = delete;
	socket_wrapper& operator=(const socket_wrapper&) = delete;

	socket_wrapper(socket_wrapper&&) noexcept;
	socket_wrapper& operator=(socket_wrapper&&) noexcept;

	~socket_wrapper() noexcept;

	void bind(u_short, u_short, const std::string&) const ;

	void connect(u_short, u_short, const std::string&) const;

	void listen(int) const;

	socket_wrapper accept() const;

	void send(const std::string&) const;

	std::string recv(size_t) const;

	void shutdown() const;

private:
	SOCKET _socket;
};