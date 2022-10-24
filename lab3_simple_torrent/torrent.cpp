#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <random>
#include <format>
#include <thread>
#include <cpr/cpr.h>
#include <iomanip>
#include <bencoding.h>
//include "bencode/BDictionary.h"
//#include "torrent_file_parser.h"

inline constexpr unsigned char operator "" _u(char arg) noexcept {
	return static_cast<unsigned char>(arg);
}

std::string get_random_string_of_digits() {
	std::string ret;

	std::random_device r;
	std::default_random_engine engine(r());
	std::uniform_int_distribution<int> uniform_dist(49, 57);

	for (size_t i = 0; i < 20; ++i) {
		ret.push_back(uniform_dist(engine));
	}

	return ret;
}

std::string to_hex(unsigned char* input) {
	std::string result;
	for (size_t i = 0; i < 20; ++i) {
		char buff[3];
		std::string tmp = itoa(static_cast<int>(input[i]), buff, 16);
		if (tmp.size() == 1) {
			tmp.insert(tmp.begin(), '0');
		}
		result += tmp;
	}
	return result;
}

void decode_peer(const std::string& peer, std::string& peer_ip, std::string& peer_port) {
	auto it = peer.begin();
	for (; it < peer.begin() + 4; ++it) {
		char buff[6];
		std::cout << static_cast<int>(*it) << std::endl;
		peer_ip += itoa(static_cast<int>(*it), buff, 16);
		peer_ip += ".";
	}
	peer_ip.pop_back();

	for (; it != peer.end(); ++it) {
		char buff[6];
		peer_port += itoa(static_cast<int>(*it), buff, 16);
	}
}

std::string generate_peer_id() {
	std::string peer_id = "-UT2021-";
	// Generate 12 random numbers
	std::random_device rd;  // Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<> distrib(1, 9);
	for (int i = 0; i < 12; i++)
		peer_id += std::to_string(distrib(gen));
	return peer_id;
}

struct request_params {
	std::string url;
	std::string peer_id;
	unsigned char* info_hash;
	unsigned long long uploaded;
	unsigned long long downloaded;
	unsigned long long left;
	u_short port;
};

std::string get_peers(const request_params& params) {
	cpr::Response res = cpr::Get(cpr::Url{ params.url }, cpr::Parameters{
				{ "info_hash", to_hex(params.info_hash) },
				{ "peer_id", std::string(params.peer_id) },
				{ "port", std::to_string(params.port) },
				{ "uploaded", std::to_string(params.uploaded) },
				{ "downloaded", std::to_string(params.downloaded) },
				{ "left", std::to_string(params.left) },
				{ "compact", std::to_string(1) }
		}, cpr::Timeout{ cpr::Timeout(15000)}
	);
	std::cout << res.text;
	return res.text;

}

void get_peers_1(const std::string& path_to_file) {
	try {
		//torrent_file_parser file(path_to_file);
		//const auto announce_url = file.get_announce();
		//std::cout << announce_url;
		//const auto response =  get_peers({ "", peer_id, info_hash, 0ull, 0ull, 2918443ull, 6881});
		
	}
	catch (std::exception& e) {
		std::cout << e.what();
	}
}

int main(int argc, char* argv[]) {
	const std::string path = "C:\\Users\\aizee\\Downloads\\ComputerNetworks.torrent";
	get_peers_1(path);
}

// Test own data
// unsigned char tmp[] = "d0:i777ee";
// std::basic_string<unsigned char> buffer(tmp);

// Test url parsing
/*const std::string url = "udp://opentor.org:2710/announce";

	std::string protocol, address, announce;
	int port;
	parsing_url(url, protocol, address, port, announce);
	std::cout << "URL: \t\t" << url << std::endl;
	std::cout << "Protocol: \t" << protocol << std::endl;
	std::cout << "Address: \t" << address << std::endl;
	std::cout << "Port: \t" << port << std::endl;
	std::cout << "Announce: \t" << announce << std::endl;*/


/*auto announce_list = std::get<list_type>(elem.get_value());
	std::cout << announce_list;
	for (auto& item : announce_list) {
		std::string protocol, address, announce, port;

		parsing_url(ustring_to_string(std::get<ustring_type>(std::get<list_type>(item.get_value())[0].get_value())), 
			protocol, address, port, announce);
		auto ip = host_to_ip(address);
		std::cout << ip << std::endl;
	}*/
// TODO: проход по announce-list и попытка подключения к каждому (если не активен то ловить 10049 в connect)