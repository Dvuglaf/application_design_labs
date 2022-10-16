#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <map>
#include "bencode.h"
#include <fstream>
#include <random>
#include "SHA1.h"
#include "utils.h"
#include <format>
#include <string_view>
#include <ostream>

inline constexpr unsigned char operator "" _u(char arg) noexcept {
	return static_cast<unsigned char>(arg);
}

ustring_type get_sha1_hash(const std::string& path) {
	CSHA1 sha1;
	sha1.HashFile(path.c_str());
	sha1.Final();
	unsigned char buff[20];
	sha1.GetHash(buff);

	return ustring_type(buff);
}

ustring_type read_file(const std::string& path) {
	std::ifstream input(path, std::ios::binary);
	ustring_type buffer(std::istreambuf_iterator<char>(input), {});
	input.close();

	return buffer;
}

ustring_type get_random_string_of_digits() {
	ustring_type ret;

	std::random_device r;
	std::default_random_engine engine(r());
	std::uniform_int_distribution<int> uniform_dist(49, 57);

	for (size_t i = 0; i < 20; ++i) {
		ret.push_back(uniform_dist(engine));
	}

	return ret;
}

std::string ustring_to_string(const std::basic_string<unsigned char>& str) {
	return std::string(str.begin(), str.end());
}

void parsing_url(const ustring_type& url,
	std::string& protocol, std::string& address, int& port, std::string& announce)
{
	auto split_slash = split(url, '/'_u);
	protocol = ustring_to_string(split(split_slash[0], ':'_u)[0]);

	auto address_and_port = split(split_slash[1], ':'_u);
	address = ustring_to_string(address_and_port[0]);
	port = std::stoi(ustring_to_string(address_and_port[1]));

	if (split_slash.size() != 3) {
		return;
	}
	announce = ustring_to_string(split_slash[2]);
}

std::string to_hex(const ustring_type& input) {
	std::string result;
	for (const auto s : input) {
		char buff[3];
		std::string tmp = itoa(static_cast<int>(s), buff, 16);
		if (tmp.size() == 1) {
			tmp.insert(tmp.begin(), '0');
		}
		result += "%" + tmp;
	}
	return result;
}

void get_request(const std::string& path) { // TODO
	const auto file_content = read_file(path);
	const auto data = read_bencode(file_content);
	const auto sha1_hash = get_sha1_hash(path);
	const auto peer_id = get_random_string_of_digits();

	unsigned char key_announce[] = "announce";
	const bencode_element elem = (std::get<dict_type>(data[0].get_value()).find(key_announce))->second;
	if (elem.get_type() != 1) {  // not ustring_type
		throw std::runtime_error("Wrong structure of .torrent file!");
	}
	const ustring_type url = std::get<ustring_type>(elem.get_value());

	std::string protocol, address, announce;
	int port;

	parsing_url(url, protocol, address, port, announce);

	std::string header = "";
	header += "GET ";
	header += protocol + "://" + address;
	if (protocol != "udp") {
		header += "/" + announce;
	}
	header += "?info_hash=" + to_hex(sha1_hash);
	header += "&peer_id=" + to_hex(peer_id);
	header += "&ip=0.0.0.0";
	header += "&port=6889";
	header += "&uploaded=0";
	header += "&downloaded=0";
	header += "&left=";
	header += " HTTP/1.1";
	header += "\r\n";
}

int main(int argc, char* argv[]) {
	/*
	if (argc != 2) {
		std::cerr << "Enter only path to torrent file!\n";
		return 1;
	}
	const std::string path = argv[1];
	*/
	const std::string path = "\\files\\7-zip_21.01_alpha.torrent";
	const auto file_content = read_file(path);
	const auto data = read_bencode(file_content);
	std::cout << data;
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