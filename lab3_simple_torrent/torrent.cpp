#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <map>
#include "bencode.h"
#include <fstream>

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Enter only path to torrent file!\n";
		return 1;
	}

	const std::string path = argv[1];
	std::ifstream input(path, std::ios::binary);
	std::basic_string<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
	input.close();

	const std::vector<bencode_element> res = read_bencode(buffer);

	std::cout << res;

}

// Test own data
// unsigned char tmp[] = "d0:i777ee";
// std::basic_string<unsigned char> buffer(tmp);