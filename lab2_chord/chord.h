#ifndef CHORD
#define CHORD

#include <vector>
#include <utility>
#include <string>
#include "socket.h"

class chord_node {
public:
	using size_type = unsigned long;

	const static size_type _m = 4;

public:
	chord_node() = delete;

	/*
	* Constructor: create node.
	*
	* @param id: node's id
	* @param ip: node's ip
	* @param port: node's port
	*/
	chord_node(size_type id, const std::string& ip, size_type port);

	/*
	* Handle user's enter.
	*
	* @param command_id: identifier of required command
	*/
	void cli(size_type command_id);

	chord_node(const chord_node&) = delete;
	chord_node& operator=(const chord_node&) = delete;

private:  // This section presents the methods that are executed in the node when it acts as server.
	/*
	* Launch node as a server in thread to handle requests in infinite loop.
	*/
	void start();

	/*
	* Handle incoming connection from another node.
	*
	* @param in: incoming connection's socket
	*/
	void handle_connection(const socket_wrapper& in);

	/*
	* Execute requested named procedure (execute at the remote node, regarding the one that asks to call).
	*
	* @param procedure_name: name of callable procedure
	* @param procedure_args: passed to procedure сomma-separated arguments
	*
	* @return return value of procedure as std::string
	*/
	std::string execute(const std::string& procedure_name, const std::string& procedure_args);

	/*
	* Update current finger table if s is i_th finger of this node, when new node is joining (execute at the remote node,
	* regarding the one that asks to call (s)).
	*
	* @param s_id: id of new inserted node (finger table entries update to value of s)
	* @param i: index of row in finger table need to update
	* @param s_ip: ip of node s
	* @param s_port: port of node s
	*/
	void update_finger_table_join(size_type s_id, size_type i, const std::string& s_ip, size_type s_port);

	/*
	* Update current finger table if s is i_th finger of this when node is leaving (execute at the remote node,
	* regarding the one that asks to call (s)).
	*
	* @param s_id: id of successor leaving node
	* @param i: index of row in finger table need to update
	* @param s_ip: ip of node s
	* @param s_port: port of node s
	* @param leave_id: id of leaving node
	*/
	void update_finger_table_leave(size_type s_id, size_type i,
		const std::string& s_ip, size_type s_port, size_type leave_id);

private:  // This section presents the methods that are executed in the node when it acts as client.
	/*
	* Calculate i_th interval's start (n + 2^(i-1) % 2^m.
	*
	* @param i: index of finger
	* 
	* @return i_th interval start
	*/
	size_type get_start(size_type i) noexcept;

	/*
	* Ask remote note to call procedure at itself.
	*
	* @param remote_id: remote node id
	* @param remote_ip: remote node ip
	* @param remote_port: remote node port
	* @param procedure_name: name of callable procedure
	* @param procedure_args: passed to procedure сomma-separated arguments
	* 
	* @return answer (return value) from remote node as std::string
	*/
	std::string remote_procedure_call(size_type remote_id, const std::string& remote_ip, size_type remote_port, 
		const std::string& procedure_name, const std::string& procedure_args);

	/*
	* Join node in the network.
	*
	* @param id: an arbitrary node's id in the network or own id if join first node
	* @param ip: an arbitrary node's ip in the network or own ip if join first node
	* @param port: an arbitrary node's port in the network or own port if join first node
	*/
	void join(size_type id, const std::string& ip, size_type port);

	/*
	* Initialize local finger table (of new inserted).
	*
	* @param id: an arbitrary node's id already in the network
	* @param ip: an arbitrary node's ip already in the network
	* @param port: an arbitrary node's port already in the network
	*/
	void init_finger_table(size_type id, const std::string& ip, size_type port);

	/*
	* Update all nodes whose finger table should refer to this node (when new node joins the network).
	*/
	void update_others_join();

	/*
	* Leave node from the network.
	*/
	void leave();

	/*
	* Update all nodes whose finger table should refer to this node (when node leaves the network).
	*/
	void update_others_leave();

	/*
	* Insert key. The node stores the key if it is the successor of the node with the index 'key'.
	* 
	* @param key: inserted key
	*/
	void put_key(size_type key);

private:  // This section presents the methods that are executed in the node when it acts both as client and server.
	/*
	* Find successor of target_id node.
	*
	* @param target_id: node's id which successor will be found
	*
	* @return three values: successor_id, successor_ip, successor_port separated by ';' as std::string
	*/
	std::string find_successor(size_type target_id);

	/*
	* Find predecessor of target_id node.
	*
	* @param target_id: node's id which predecessor will be found
	*
	* @return three values: predecessor_id, predecessor_ip, predecessor_port separated by ';' as std::string
	*/
	std::string find_predecessor(size_type target_id);

	/*
	* Find closest finger preceding target_id node.
	*
	* @param target_id: node's which closest preceding finger will be found
	*
	* @return three values: closest_preceding_finger_id, closest_preceding_finger_ip,
	*		closest_preceding_finger_port separated by ';' as std::string
	*/
	std::string closest_preceding_finger(size_type target_id);

	/*
	* Update keys. Key is placed to node_id >= key.
	*/
	void update_keys();


private:
	struct finger {
		size_type _start;
		std::pair<size_type, size_type> _interval;
		size_type _id;
		std::string _ip;
		size_type _port;
	};

	struct predecessor_info {
		size_type _id;
		std::string _ip;
		size_type _port;
	};

private:
	std::vector<finger> _finger_table;

	predecessor_info _predecessor;

	size_type _id;
	std::string _ip;
	size_type _port;

	bool _joined;  // true if node correctly joined to the network

	std::vector<size_type> _keys;
};

#endif