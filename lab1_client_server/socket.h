#ifndef SOCKET_WRAPPER
#define SOCKET_WRAPPER

#include <WinSock2.h>
#include <string>


class socket_wrapper {
public:
	socket_wrapper() = delete;
	
	explicit socket_wrapper(SOCKET);

	/**
	* The socket function creates a socket that is bound to a specific transport service provider.
	* 
	* @param internet_protocol address family specification (values in WinSock2.h header).
	* @param type the type specification for the new socket (values in WinSock2.h header).
	* @param protocol the protocol to be used (values in WinSock2.h header).
	*/
	explicit socket_wrapper(int internet_protocol, int type, int protocol);

	socket_wrapper(const socket_wrapper&) = delete;
	socket_wrapper& operator=(const socket_wrapper&) = delete;

	socket_wrapper(socket_wrapper&&) noexcept;
	socket_wrapper& operator=(socket_wrapper&&) noexcept;

	~socket_wrapper() noexcept;

	/**
	 * The bind function associates a local address with a socket.
	 *
	 * @param internet_protocol address family specification (values in WinSock2.h header).
	 * @param port.
	 * @param ip address.
	 */
	void bind(u_short internet_protocol, u_short port, const std::string& ip) const ;

	/**
	* The connect function establishes a connection to a specified socket.
	*
	* @param internet_protocol address family specification (values in WinSock2.h header).
	* @param port.
	* @param ip address.
	*/
	void connect(u_short internet_protocol, u_short port, const std::string& ip) const;

	/**
	* The listen function places a socket in a state in which it is listening for an incoming connection.
	* 
	* @param max_connections the maximum length of the queue of pending connections.
	*/
	void listen(int max_connections) const;

	/**
	* The accept function permits an incoming connection attempt on a socket.
	*
	* @return a handle for the socket on which the actual connection is made.
	*/
	socket_wrapper accept() const;

	/**
	* The send function sends data on a connected socket.
	*
	* @param buffer data to send.
	* @param size bytes of sending data
	* 
	* @return the total number of bytes sent.
	*/
	int send(const char* buffer, int size) const;

	/**
	* The recv function receives data from a connected socket or a bound connectionless socket
	*
	* @param buffer a pointer to the buffer to receive the incoming data.
	* @param size bytes to receive the incoming data
	*
	* @return the number of bytes received and the buffer pointed to by the buffer parameter will contain this data received.
	*/
	int recv(char* buffer, int size) const;

	/**
	* The shutdown function disables both send and receive operations.
	*/
	void shutdown() const;

private:
	SOCKET _socket;
};
#endif