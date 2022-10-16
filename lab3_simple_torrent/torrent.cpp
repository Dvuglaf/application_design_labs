#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <map>
#include "bencode.h"


int main() {
	std::string input = "i666e3:aaad3:bard6:foobari23e6:sketch6:parrote3:fooi42ee3:aaai-777e";
	std::vector<bencode_element> res = read_bencode(input);
	for (const auto& it : res) {
		std::cout << it << std::endl;
	}
}
