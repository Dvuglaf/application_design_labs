#include "chord.h"
#include <string>
#include "socket.h"
#include <thread>
#include <iostream>
#include "utils.h"

#define to_number(x) std::stoul(x)

using size_type = chord_node::size_type;

chord_node::chord_node(size_type id, const std::string& ip, size_type port) {
	_id = id;
	_ip = ip;
	_port = port;

	std::thread server(&chord_node::start, this);
	server.detach();
}

void chord_node::start() {
	while (true) {
		const socket_wrapper socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);
		socket.bind(address_family::IPV4, static_cast<unsigned short>(_port), _ip);
		socket.listen(SOMAXCONN);

		socket_wrapper slave_socket = socket.accept();
		std::thread thread(&chord_node::handle_connection, this, (std::move(slave_socket)));
		thread.detach();
	}
}

void chord_node::handle_connection(const socket_wrapper& slave_socket) {
	std::string get = receive_via_socket(slave_socket);
	auto received = split(get, std::string(";"));

	if (to_number(received[0]) != _id) {  // mismatch between id and other params
		std::string answer = "wrong_id";
		slave_socket.send(reinterpret_cast<const char*>(&answer[0]),
			static_cast<int>(answer.size()));  // send an answer "wrong_id" 
		return;
	}

	std::string answer;

	if (received.size() == 2) // no args, only id for check and procedure name
		answer = execute(received[1], "");
	else
		answer = execute(received[1], received[2]);

	slave_socket.send(reinterpret_cast<const char*>(&answer[0]),
		static_cast<int>(answer.size()));  // send an answer
}

std::string chord_node::remote_procedure_call(size_type target_id, const std::string& target_ip, size_type target_port,
	const std::string& procedure_name, const std::string& procedure_args)
{
	// Connect with another node
	const socket_wrapper socket(address_family::IPV4, socket_type::TCP_SOCKET, protocol::TCP);

	socket.connect(address_family::IPV4, static_cast<unsigned short>(target_port), target_ip);

	// Send request "target_id;procedure;args"
	std::string request = std::to_string(target_id) + std::string(";") + 
		procedure_name + std::string(";") + procedure_args;
	socket.send(reinterpret_cast<const char*>(&request[0]),
		static_cast<int>(request.size()));

	socket.shutdown(shutdown_type::SEND);  // shutdown for send, not receive

	// Receive answer
	std::string received_answer = receive_via_socket(socket);
	if (received_answer == "wrong_id") {
		throw std::invalid_argument("Wrong id. Mismatch between id and port.");
	}

	return received_answer;
}

std::string chord_node::execute(const std::string& procedure_name, const std::string& procedure_args) {
	if (procedure_name == "get_successor") {  // first row in finger table is successor of current node
		return std::to_string(_finger_table[0]._id) + std::string(";") +
			_finger_table[0]._ip + std::string(";") + std::to_string(_finger_table[0]._port);
	}
	else if (procedure_name == "get_predecessor") {
		return std::to_string(_predecessor._id) + std::string(";") +
			_predecessor._ip + std::string(";") + std::to_string(_predecessor._port);
	}
	else if (procedure_name == "set_successor") {
		auto args = split(procedure_args, ",");  // id, ip, port
		_finger_table[0]._id = to_number(args[0]);
		_finger_table[0]._ip = args[1];
		_finger_table[0]._port = to_number(args[2]);
	}
	else if (procedure_name == "set_predecessor") {
		auto args = split(procedure_args, ",");  // id, ip, port
		_predecessor._id = to_number(args[0]);
		_predecessor._ip = args[1];
		_predecessor._port = to_number(args[2]);
	}
	else if (procedure_name == "find_successor") {
		return find_successor(to_number(procedure_args));
	}
	else if (procedure_name == "closest_preceding_finger") {
		return closest_preceding_finger(to_number(procedure_args));
	}
	else if (procedure_name == "update_finger_table_join") {
		auto args = split(procedure_args, ",");  // s_id, i, s_ip, s_port
		update_finger_table_join(to_number(args[0]), to_number(args[1]),
			args[2], to_number(args[3]));
	}
	else if (procedure_name == "update_finger_table_leave") {
		auto args = split(procedure_args, ",");  // s_id, i, s_ip, s_port, leave_id
		update_finger_table_leave(to_number(args[0]), to_number(args[1]),
			args[2], to_number(args[3]), to_number(args[4]));
	}
	else
		throw std::runtime_error("unknown procedure name");

	return "";
}

std::string chord_node::find_successor(size_type target_id) {
	std::string get = find_predecessor(target_id);
	auto pred_of_target = split(get, ";");

	size_type id_pred_of_target = to_number(pred_of_target[0]);
	std::string ip_pred_of_target = pred_of_target[1];
	size_type port_pred_of_target = to_number(pred_of_target[2]);

	get = remote_procedure_call(id_pred_of_target, ip_pred_of_target, port_pred_of_target,
		std::string("get_successor"), std::string(""));
	auto succ_of_pred_of_target = split(get, ";");

	return succ_of_pred_of_target[0] + std::string(";") +
		succ_of_pred_of_target[1] + std::string(";") +
		succ_of_pred_of_target[2];

}

std::string chord_node::find_predecessor(size_type target_id) {
	size_type remote_id = _id;
	std::string remote_ip = _ip;
	size_type remote_port = _port;

	size_type remote_successor = _finger_table[0]._id;

	// id not in (remote_id, remote_successor]
	while (!does_belong(target_id, remote_id, remote_successor, false, true)) {
		std::string get = remote_procedure_call(remote_id, remote_ip, remote_port,
			std::string("closest_preceding_finger"), std::to_string(target_id));
		auto closest_pred_of_id = split(get, ";");

		remote_id = to_number(closest_pred_of_id[0]);
		remote_ip = closest_pred_of_id[1];
		remote_port = to_number(closest_pred_of_id[2]);

		get = remote_procedure_call(remote_id, remote_ip, remote_port,
			std::string("get_successor"), std::string(""));
		auto succ_of_remote = split(get, ";");

		remote_successor = to_number(succ_of_remote[0]);
	}

	return std::to_string(remote_id) + std::string(";") +
		remote_ip + std::string(";") +
		std::to_string(remote_port);
}

std::string chord_node::closest_preceding_finger(size_type target_id) {
	for (int i = _m - 1; i >= 0; --i) {  

		// finger_table[i].node in (n, target_id)
		if (does_belong(_finger_table[i]._id, _id, target_id, false, false)) {  
			return std::to_string(_finger_table[i]._id) + std::string(";") +
				_finger_table[i]._ip + std::string(";") +
				std::to_string(_finger_table[i]._port);
		}
	}

	return std::to_string(_id) + std::string(";") +
		_ip + std::string(";") +
		std::to_string(_port);
}

size_type chord_node::get_start(size_type i) {
	if (i == 0) {
		return (_id + 1) % (1 << _m);
	}
	else {
		return (_id + (1u << i)) % (1u << _m);
	}
}

void chord_node::join(size_type id, const std::string& ip, size_type port) {
	if (_id == id) {  // first joined node to the ring
		if (ip != _ip) {
			throw std::invalid_argument("Enter your ip.");
		}
		if (port != _port) {
			throw std::invalid_argument("Enter your port.");
		}
		for (size_type i = 0; i < _m; ++i) {
			_finger_table.push_back({
				get_start(i),
				std::make_pair<size_type, size_type>(
					get_start(i),
					get_start(i + 1)
				),
				_id,
				ip,
				port}
			);
		}
		// Successor = Predecessor = node
		_finger_table[0]._id = _predecessor._id = _id;  
		_finger_table[0]._ip = _predecessor._ip = ip;
		_finger_table[0]._port = _predecessor._port = port;

		_ip = ip;
		_port = port;
	}
	else {
		init_finger_table(id, ip, port);
		update_others_join();
	}
}

void chord_node::init_finger_table(size_type id, const std::string& ip, size_type port) {
	std::string get = remote_procedure_call(id, ip, port, "find_successor", std::to_string(get_start(0)));
	auto succ = split(get, ";");

	_finger_table.push_back({
				get_start(0),
				std::make_pair<size_type, size_type>(
					get_start(0),
					get_start(1)
				),
				to_number(succ[0]),
				succ[1],
				to_number(succ[2])}
	);

	get = remote_procedure_call(_finger_table[0]._id, _finger_table[0]._ip, _finger_table[0]._port, 
		"get_predecessor", "");
	auto pred_of_succ = split(get, ";");
	_predecessor._id = to_number(pred_of_succ[0]);
	_predecessor._ip = pred_of_succ[1];
	_predecessor._port = to_number(pred_of_succ[2]);

	// Set successor's predecessor to this node
	remote_procedure_call(_finger_table[0]._id, _finger_table[0]._ip, _finger_table[0]._port,
		"set_predecessor", std::to_string(_id) + std::string(",") +
		_ip + std::string(",") + std::to_string(_port));

	for (size_type i = 0; i < _m - 1; ++i) {
		// get_start(i + 1) in [n, _finger_table[i]._id)
		if (does_belong(get_start(i + 1), _id, _finger_table[i]._id, true, false)) {
			_finger_table.push_back({
						get_start(i + 1),
						std::make_pair<size_type, size_type>(
							get_start(i + 1),
							get_start(i + 2)
						),
						_finger_table[i]._id,
						_finger_table[i]._ip,
						_finger_table[i]._port}
			);
		}
		else {
			get = remote_procedure_call(id, ip, port,
				std::string("find_successor"), std::to_string(get_start(i + 1)));
			auto succ_of_start_inc_i = split(get, ";");

			_finger_table.push_back({
						get_start(i + 1),
						std::make_pair<size_type, size_type>(
							get_start(i + 1),
							get_start(i + 2)
						),
						to_number(succ_of_start_inc_i[0]),
						succ_of_start_inc_i[1],
						to_number(succ_of_start_inc_i[2])}
			);
		}
	}
}

void chord_node::update_others_join() {
	for (size_type i = 0; i < _m; ++i) {
		std::string get;
		if (i == 0)  // need to update predecessor's successor to this node
			get = find_predecessor((_id) % (1 << _m));
		else
			get = find_predecessor((_id + 1 - (1 << i) + (1 << _m)) % (1 << _m));
		
		auto found_pred = split(get, ";");

		std::string args = std::to_string(_id) + std::string(",") + std::to_string(i) +
			std::string(",") + _ip + std::string(",") + std::to_string(_port);  // args = this node's params

		// Let other nodes know that a new one has been inserted
		remote_procedure_call(to_number(found_pred[0]), found_pred[1], to_number(found_pred[2]),
			std::string("update_finger_table_join"), args);
	}
}

void chord_node::update_finger_table_join(size_type s_id, size_type i, const std::string& s_ip, size_type s_port) {
	// If s_id in [ _finger_table[i]._start, _finger_table[i]._id) -> update
	// If _finger_table[i]._start == _finger_table[i]._id no need to update
	if (does_belong(s_id, _finger_table[i]._start, _finger_table[i]._id, true, false) &&
		_finger_table[i]._start != _finger_table[i]._id) { 

		_finger_table[i]._id = s_id;
		_finger_table[i]._ip = s_ip;
		_finger_table[i]._port = s_port;

		std::string args = std::to_string(s_id) + std::string(",") + std::to_string(i) +
			std::string(",") + s_ip + std::string(",") + std::to_string(s_port);

		// Walk through predecessors and update table if needed
		remote_procedure_call(_predecessor._id, _predecessor._ip, _predecessor._port,
			std::string("update_finger_table_join"), args);
	}
	// If s in (_pred_id, _id) we need to update predecessor of this node to new inserted node
	if (does_belong(s_id, _predecessor._id, _id, false, false)) {
		_predecessor._id = s_id;
		_predecessor._ip = s_ip;
		_predecessor._port = s_port;
	}
}

void chord_node::leave() {
	if (_id == _predecessor._id) {  // if one node exist in the network
		return;
	}

	// Change predecessor in the successor to this node predecessor
	std::string args = std::to_string(_predecessor._id) + std::string(",") + _predecessor._ip +
		std::string(",") + std::to_string(_predecessor._port);
	remote_procedure_call(_finger_table[0]._id, _finger_table[0]._ip, _finger_table[0]._port,
		std::string("set_predecessor"), args);

	update_others_leave();
}

void chord_node::update_others_leave() {
	for (size_type i = 0; i < _m; ++i) {
		std::string get;
		if (i == 0)  // need to update predecessor's successor to this node
			get = find_predecessor((_id) % (1 << _m));
		else
			get = find_predecessor((_id + 1 - (1 << i) + (1 << _m)) % (1 << _m));

		auto found_pred = split(get, ";");

		std::string args = std::to_string(_finger_table[0]._id) + std::string(",") + std::to_string(i) +
			std::string(",") + _finger_table[0]._ip + 
			std::string(",") + std::to_string(_finger_table[0]._port) +
			std::string(",") + std::to_string(_id);						// args = successor node's params

		// Let other nodes know that a new one has been inserted
		remote_procedure_call(to_number(found_pred[0]), found_pred[1], to_number(found_pred[2]),
			std::string("update_finger_table_leave"), args);
	}
}

void chord_node::update_finger_table_leave(size_type s_id, size_type i, 
	const std::string& s_ip, size_type s_port, size_type leaved_id) 
{
	if (_finger_table[i]._id == leaved_id) {
		_finger_table[i]._id = s_id;
		_finger_table[i]._ip = s_ip;
		_finger_table[i]._port = s_port;

		std::string args = std::to_string(s_id) + std::string(",") + std::to_string(i) +
			std::string(",") + s_ip +
			std::string(",") + std::to_string(s_port) +
			std::string(",") + std::to_string(leaved_id);		// args = successor node's params

		// Let other nodes know that a new one has been inserted
		remote_procedure_call(_predecessor._id, _predecessor._ip, _predecessor._port,
			std::string("update_finger_table_leave"), args);
	}

}

void chord_node::cli(size_type command_id) {
	if (command_id == 0) {  // join
		size_type id, port;
		std::string ip;
		std::string value;

		std::cout << "Enter ID: ";
		std::cin >> value;
		id = to_number(value);

		std::cout << "Enter IP: ";
		std::cin >> value;
		ip = value;

		std::cout << "Enter PORT: ";
		std::cin >> value;
		port = to_number(value);

		check_users_params(id, ip, port);

		join(id, ip, port);
		std::cout << "Join successful\n";
	}
	else if (command_id == 1) {  //display info about node
		for (size_type i = 0; i < _m; ++i) {
			std::cout << i << "] ("
				<< _finger_table[i]._interval.first << " "
				<< _finger_table[i]._interval.second << ") "
				<< _finger_table[i]._id << " <"
				<< _finger_table[i]._ip << ":"
				<< _finger_table[i]._port << ">" << std::endl;
		}
		std::cout << "Successor_ID: " << _finger_table[0]._id;
		std::cout << "\nPredecessor_ID: " << _predecessor._id;
		std::cout << std::endl;
	}
	else if (command_id == 2) {
		leave();
		std::cout << "Leave successful\n";
	}
}