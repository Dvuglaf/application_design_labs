#include "chord.h"
#include <string>
#include "socket.h"
#include <thread>
#include <iostream>

using size_type = chord_node::size_type;

std::vector<std::string> split(const std::string& string, const std::string& delimiter) {
	std::vector<std::string> tokens;
	size_t previous = 0, position = 0;
	do {
		position = string.find(delimiter, previous);
		if (position == std::string::npos) position = string.length();
		std::string token = string.substr(previous, position - previous);
		if (!token.empty()) tokens.push_back(token);
		previous = position + delimiter.length();
	} while (position < string.length() && previous < string.length());
	return tokens;
}

bool does_belong(size_type idx, size_type start, size_type end, bool left_included, bool right_included) {
	if (left_included && right_included) {  // [..., ...]
		if (start < end) {
			return (idx >= start && idx <= end);
		}
		else if (end < start) {
			return !(idx > end && idx < start);
		}
		else if (end == start) {
			return true;
		}
	}
	else if (left_included && !right_included) {  // [..., ...)
		if (start < end) {
			return (idx >= start && idx < end);
		}
		else if (end < start) {
			return !(idx >= end && idx < start);
		}
		else if (end == start) {
			return true;
		}
	}
	else if (!left_included && right_included) {  // (..., ...]
		if (start < end) {
			return (idx > start && idx <= end);
		}
		else if (end < start) {
			return !(idx > end && idx <= start);
		}
		else if (end == start) {
			return true;
		}
	}
	else if (!(left_included && right_included)) {  // (..., ...)
		if (start < end) {
			return (idx > start && idx < end);
		}
		else if (end < start) {
			return !(idx >= end && idx <= start);
		}
		else if (end == start) {
			return !(idx == start);
		}
	}
}

std::string handle(const socket_wrapper& slave_socket) {
	int received_bytes = 0;
	std::string received_buffer;
	try {
		do {
			char received[8192];
			received_bytes = slave_socket.recv(received, 8192);

			//if (received_bytes > 0)
				//std::cout << "\tReceived " << received_bytes << " bytes\n";

			//else if (received_bytes == 0)
				//std::cout << "Connection closed" << std::endl;

			for (size_t i = 0; i < received_bytes; ++i)
				received_buffer.push_back(received[i]);
			//std::cout << "RECEIVED: " << received_buffer << std::endl;

		} while (received_bytes > 0);
	}
	catch (std::exception& e) {
		std::cout << e.what();
	}
	return received_buffer;
}

chord_node::chord_node(size_type id, const std::string& ip, size_type port) {
	_n = id;
	_ip = ip;
	_port = port;

	std::thread thread(&chord_node::start, this);
	thread.detach();
}

void chord_node::start() {
	std::cout << _ip << ":" << _port << std::endl;
	while (true) {
		const socket_wrapper socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);
		socket.bind(address_family::IPV4, _port, _ip);
		socket.listen(SOMAXCONN);
		socket_wrapper slave_socket = socket.accept();
		std::thread thread(&chord_node::handle_connection, this, (std::move(slave_socket)));
		thread.detach();
	}

}

void chord_node::handle_connection(const socket_wrapper& slave_socket) {
	std::string received = handle(slave_socket);

	std::vector<std::string> splitted = split(received, std::string(";"));

	std::string answer;

	if (splitted.size() == 1) // no args
		answer = execute(splitted[0], "");
	else
		answer = execute(splitted[0], splitted[1]);

	const int bytes_sent = slave_socket.send(reinterpret_cast<const char*>(&answer[0]),
		static_cast<int>(answer.size()));
}

// Отправление запроса на выполнение процедуры на удаленной Node. Получение ответа.
std::string chord_node::remote_procedure_call(const std::string& ip, size_type port,
	const std::string& procedure_name, const std::string& procedure_args)
{
	// Connect with another node
	const socket_wrapper socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);
	socket.connect(address_family::IPV4, port, ip);

	// Send request
	std::string request = procedure_name + std::string(";") + procedure_args;
	const int bytes_sent = socket.send(reinterpret_cast<const char*>(&request[0]),
		static_cast<int>(request.size()));

	socket.shutdown(shutdown_type::SEND);

	// Receive answer
	return handle(socket);
}

// Исполнение требуемого запроса.
// @ returns ответ на запрос
std::string chord_node::execute(const std::string& procedure_name, const std::string& procedure_args) {
	if (procedure_name == "get_successor") {
		return std::to_string(_finger_table[0]._node) + std::string(";") +
			_finger_table[0]._ip + std::string(";") + std::to_string(_finger_table[0]._port);
	}
	else if (procedure_name == "get_predecessor") {
		return std::to_string(_predecessor) + std::string(";") +
			_predecessor_ip + std::string(";") + std::to_string(_predecessor_port);
	}
	else if (procedure_name == "set_successor") {
		auto splitted = split(procedure_args, ",");
		_finger_table[0]._node = std::stol(splitted[0]);
		_finger_table[0]._ip = splitted[1];
		_finger_table[0]._port = std::stol(splitted[2]);
	}
	else if (procedure_name == "set_predecessor") {
		auto splitted = split(procedure_args, ",");
		_predecessor = std::stol(splitted[0]);
		_predecessor_ip = splitted[1];
		_predecessor_port = std::stol(splitted[2]);
	}
	else if (procedure_name == "find_successor") {
		return find_successor(std::stol(procedure_args));
	}
	else if (procedure_name == "closest_preceding_finger") {
		return closest_precedng_finger(std::stol(procedure_args));
	}
	else if (procedure_name == "update_finger_table") {
		auto splitted = split(procedure_args, ",");
		update_finger_table(std::stol(splitted[0]), std::stol(splitted[1]),
			splitted[2], std::stol(splitted[3]));
	}
	else
		throw std::runtime_error("unknown procedure name");

	return "";
}

// Return string "id;IP;port"
std::string chord_node::find_successor(size_type id) {
	std::string get = find_predecessor(id);
	auto splitted = split(get, ";");

	auto ret = std::stol(splitted[0]);
	auto ip = splitted[1];
	auto port = std::stol(splitted[2]);
	get = remote_procedure_call(ip, port,
		std::string("get_successor"), std::string(""));
	splitted = split(get, ";");

	return splitted[0] + std::string(";") +
		splitted[1] + std::string(";") +
		splitted[2];

}

// Return string "id;IP;port"
std::string chord_node::find_predecessor(size_type id) {
	size_type ret = _n;

	/*
	std::string get = closest_precedng_finger(id);
	auto splitted = split(get, ";");
	ret = std::stol(splitted[0]);
	*/
	auto ip = _ip;
	auto port = _port;

	size_type remote_successor = _finger_table[0]._node;

	while (!does_belong(id, ret, remote_successor, false, true)) {  // id not in (ret, remote_successor]
		std::string get = remote_procedure_call(ip, port, std::string("closest_preceding_finger"), std::to_string(id));
		auto splitted = split(get, ";");
		ret = std::stol(splitted[0]);
		ip = splitted[1];
		port = std::stol(splitted[2]);

		get = remote_procedure_call(ip, port,
			std::string("get_successor"), std::string(""));
		splitted = split(get, ";");
		remote_successor = std::stol(splitted[0]);
	}
	return std::to_string(ret) + std::string(";") +
		ip + std::string(";") +
		std::to_string(port);
}

// Return string "id;IP;port"
std::string chord_node::closest_precedng_finger(size_type id) { // MUST RETURN IP AND PORT!!
	for (int i = _m - 1; i >= 0; --i) {
		if (does_belong(_finger_table[i]._node, _n, id, false, false)) {  // finger_table[i].node in (n, id)
			return std::to_string(_finger_table[i]._node) + std::string(";") +
				_finger_table[i]._ip + std::string(";") +
				std::to_string(_finger_table[i]._port);
		}
	}
	return std::to_string(_n) + std::string(";") +
		_ip + std::string(";") +
		std::to_string(_port);
}

size_type chord_node::get_start(size_type i) {
	if (i == 0) {
		return (_n + 1) % (1 << _m);
	}
	else {
		return (_n + (1 << i)) % (1 << _m);
	}
}


void chord_node::join(size_type id, const std::string& ip, size_type port) {
	if (_n == id) {
		for (size_type i = 0; i < _m; ++i) {
			_finger_table.push_back({
				get_start(i),
				std::make_pair<size_type, size_type>(
					get_start(i),
					get_start(i + 1)
				),
				_n,
				ip,
				port
				});
		}
		_finger_table[0]._node = _n;
		_finger_table[0]._ip = ip;
		_finger_table[0]._port = port;

		_predecessor = _n;
		_predecessor_ip = ip;
		_predecessor_port = port;

		_ip = ip;
		_port = port;
	}
	else {
		init_finger_table(id, ip, port);
		update_others();
	}
}

void chord_node::init_finger_table(size_type id, const std::string& ip, size_type port) {
	std::string get = remote_procedure_call(ip, port, "find_successor", std::to_string(get_start(0)));
	auto splitted = split(get, ";");

	_finger_table.push_back({
				get_start(0),
				std::make_pair<size_type, size_type>(
					get_start(0),
					get_start(1)
				),
				std::stol(splitted[0]),
				splitted[1],
				std::stol(splitted[2])
		});

	get = remote_procedure_call(_finger_table[0]._ip, _finger_table[0]._port, "get_predecessor", "");
	splitted = split(get, ";");
	_predecessor = std::stol(splitted[0]);
	_predecessor_ip = splitted[1];
	_predecessor_port = std::stol(splitted[2]);

	remote_procedure_call(_finger_table[0]._ip, _finger_table[0]._port,
		"set_predecessor", std::to_string(_n) + std::string(",") +
		_ip + std::string(",") + std::to_string(_port));

	for (size_type i = 0; i < _m - 1; ++i) {
		// get_start(i + 1) in [n, _finger_table[i]._node)
		if (does_belong(get_start(i + 1), _n, _finger_table[i]._node, true, false)) {
			_finger_table.push_back({
						get_start(i + 1),
						std::make_pair<size_type, size_type>(
							get_start(i + 1),
							get_start(i + 2)
						),
						_finger_table[i]._node,
						_finger_table[i]._ip,
						_finger_table[i]._port
				});
		}
		else {
			get = remote_procedure_call(ip, port,
				std::string("find_successor"), std::to_string(get_start(i + 1)));
			splitted = split(get, ";");

			_finger_table.push_back({
						get_start(i + 1),
						std::make_pair<size_type, size_type>(
							get_start(i + 1),
							get_start(i + 2)
						),
						std::stol(splitted[0]),
						splitted[1],
						std::stol(splitted[2])
				});
		}
	}
}

void chord_node::update_others() {
	for (size_type i = 0; i < _m; ++i) {
		std::string get;
		if (i == 0)
			get = find_predecessor((_n) % (1 << _m));
		else
			get = find_predecessor((_n + 1 - (1 << i) + (1 << _m)) % (1 << _m));
		auto splitted = split(get, ";");
		std::string args = std::to_string(_n) + std::string(",") + std::to_string(i) +
			std::string(",") + _ip + std::string(",") + std::to_string(_port);

		remote_procedure_call(splitted[1], std::stol(splitted[2]),
			std::string("update_finger_table"), args);
	}
}

void chord_node::update_finger_table(size_type s, size_type i, const std::string& s_ip, size_type s_port) {
	if (does_belong(s, _finger_table[i]._start, _finger_table[i]._node, true, false) &&
		_finger_table[i]._start != _finger_table[i]._node) { // s in [_n, _finger_table[i]._node)
		std::cout << "NODE_" << _n << " [";
		std::cout << i << "] ("
			<< _finger_table[i]._interval.first << " "
			<< _finger_table[i]._interval.second << ") "
			<< _finger_table[i]._node << " <"
			<< _finger_table[i]._ip << ":"
			<< _finger_table[i]._port << ">" << "\t\t";

		_finger_table[i]._node = s;
		_finger_table[i]._ip = s_ip;
		_finger_table[i]._port = s_port;

		std::cout << i << "] ("
			<< _finger_table[i]._interval.first << " "
			<< _finger_table[i]._interval.second << ") "
			<< _finger_table[i]._node << " <"
			<< _finger_table[i]._ip << ":"
			<< _finger_table[i]._port << ">" << std::endl;

		std::string args = std::to_string(s) + std::string(",") + std::to_string(i) +
			std::string(",") + s_ip + std::string(",") + std::to_string(s_port);

		remote_procedure_call(_predecessor_ip, _predecessor_port,
			std::string("update_finger_table"), args);
	}
	if (does_belong(s, _predecessor, _n, false, false)) {
		_predecessor = s;
		_predecessor_ip = s_ip;
		_predecessor_port = s_port;
	}
}


void chord_node::cli(size_type command_id) {
	if (command_id == 0) {  // join
		size_type id, port;
		std::string ip;
		std::string value;

		std::cout << "Enter ID: ";
		std::cin >> value;
		id = std::stol(value);

		std::cout << "Enter IP: ";
		std::cin >> value;
		ip = value;

		std::cout << "Enter PORT: ";
		std::cin >> value;
		port = std::stol(value);

		join(id, ip, port);
		std::cout << "Join successful\n";
	}
	else if (command_id == 1) {  //display
		for (size_type i = 0; i < _m; ++i) {
			std::cout << i << "] ("
				<< _finger_table[i]._interval.first << " "
				<< _finger_table[i]._interval.second << ") "
				<< _finger_table[i]._node << " <"
				<< _finger_table[i]._ip << ":"
				<< _finger_table[i]._port << ">" << std::endl;
		}
		std::cout << "Successor: " << _finger_table[0]._node;
		std::cout << "\nPredecessor: " << _predecessor;
		std::cout << "\nPredecessor_IP: " << _predecessor_ip;
		std::cout << "\nPredecessor_PORT: " << _predecessor_port;
		std::cout << std::endl;
		std::cout << "OWN: " << _ip << ":" << _port << std::endl;


	}
	else if (command_id == 2) {
		std::cout << "node_id: " << std::endl;
		std::string value;
		std::cin >> value;

		std::cout << find_successor(std::stol(value)) << std::endl;
	}
	else if (command_id == 3){
		std::cout << "node_id: " << std::endl;
		std::string value;
		std::cin >> value;

		std::cout << find_predecessor(std::stol(value)) << std::endl;
	}
}