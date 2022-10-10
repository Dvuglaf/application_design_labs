#ifndef SOCKET_WRAPPER
#define SOCKET_WRAPPER

#include <WinSock2.h>
#include <string>


enum socket_type {
	TCP_SOCKET = SOCK_STREAM,
	UDP_SOCKET = SOCK_DGRAM,
};

enum address_family {
	IPV4 = AF_INET,
	IPV6 = AF_INET6
};

enum protocol {
	ICMP = IPPROTO_ICMP,
	IGMP = IPPROTO_IGMP,
	TCP = IPPROTO_TCP,
	UDP = IPPROTO_UDP
};

enum shutdown_type {
	RECV = SD_RECEIVE,
	SEND = SD_SEND,
	BOTH = SD_BOTH
};

class socket_wrapper {
public:
	socket_wrapper() = delete;

	explicit socket_wrapper(SOCKET);

	/**
	* The socket function creates a socket that is bound to a specific transport service provider.
	*
	* @param adress_family: address family specification (values in WinSock2.h header)
	* @param type: the type specification for the new socket (values in WinSock2.h header)
	* @param protocol: the protocol to be used (values in WinSock2.h header)
	*/
	explicit socket_wrapper(int adress_family, int type, int protocol);

	socket_wrapper(const socket_wrapper&) = delete;
	socket_wrapper& operator=(const socket_wrapper&) = delete;

	socket_wrapper(socket_wrapper&&) noexcept;
	socket_wrapper& operator=(socket_wrapper&&) noexcept;

	~socket_wrapper() noexcept;

	/**
	 * The bind function associates a local address with a socket.
	 *
	 * @param internet_protocol: address family specification (values in WinSock2.h header)
	 * @param port: port
	 * @param ip: ip address
	 */
	void bind(u_short internet_protocol, u_short port, const std::string& ip) const;

	/**
	* The connect function establishes a connection to a specified socket.
	*
	* @param internet_protocol: address family specification (values in WinSock2.h header)
	* @param port: port
	* @param ip: ip address
	*/
	void connect(u_short internet_protocol, u_short port, const std::string& ip) const;

	/**
	* The listen function places a socket in a state in which it is listening for an incoming connection.
	*
	* @param max_connections: the maximum length of the queue of pending connections
	*/
	void listen(int max_connections) const;

	/**
	* The accept function permits an incoming connection attempt on a socket.
	*
	* @return a handle for the socket on which the actual connection is made
	*/
	socket_wrapper accept() const;

	/**
	* The send function sends data on a connected socket.
	*
	* @param buffer: data to send
	* @param size: bytes of sending data
	*
	* @return the total number of bytes sent
	*/
	int send(const char* buffer, int size) const;

	/**
	* The recv function receives data from a connected socket or a bound connectionless socket
	*
	* @param buffer: a pointer to the buffer to receive the incoming data
	* @param size: bytes to receive the incoming data
	*
	* @return the number of bytes received and the buffer pointed to by the buffer parameter will contain this data received
	*/
	int recv(char* buffer, int size) const;

	/**
	* The shutdown function disables both send and receive operations.
	*
	* @param type: type of shutdown (only for receive, only for send, both)
	*/
	void shutdown(u_short type) const;

public:
	SOCKET _socket;
	static WSADATA _wsa;
	static size_t _socket_count;
};
#endif