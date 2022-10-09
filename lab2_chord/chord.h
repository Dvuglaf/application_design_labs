#ifndef CHORD
#define CHORD

#include <vector>
#include <utility>
#include <string>
#include "socket.h"

class chord_node {
public:
	using size_type = long;

public:
	chord_node() = delete;
	chord_node(size_type id, const std::string& ip, size_type port);

	size_type get_start(size_type i);

	void start();
	void handle_connection(const socket_wrapper&);

	std::string remote_procedure_call(const std::string& ip, size_type port, const std::string& procedure_name, const std::string& procedure_args);

	std::string execute(const std::string& procedure_name, const std::string& procedure_args);

	std::string find_successor(size_type id);
	std::string find_predecessor(size_type id);
	std::string closest_precedng_finger(size_type id);

	void join(size_type id, const std::string& ip, size_type port);
	void init_finger_table(size_type id, const std::string& ip, size_type port);
	void update_others();
	void update_finger_table(size_type s, size_type i, const std::string& s_ip, size_type s_port);

	void cli(size_type command_id);

	chord_node(const chord_node&) = delete;
	chord_node& operator=(const chord_node&) = delete;

private:
	struct finger {
		size_type _start;
		std::pair<size_type, size_type> _interval;
		size_type _node;
		std::string _ip;
		size_type _port;
	};

private:
	const static size_type _m = 3;
	size_type _n;  // current node identifier
	std::vector<finger> _finger_table;

	size_type _predecessor;
	std::string _predecessor_ip;
	size_type _predecessor_port;

	std::string _ip;
	size_type _port;
};

#endif