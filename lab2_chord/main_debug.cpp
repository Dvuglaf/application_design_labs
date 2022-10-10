#include "chord.h"
#include <iostream>
#include <string>

int main() {
	std::cout << "*************************** CHORD **************************" << std::endl;

	std::string cmd;

	chord_node node(2, std::string("127.0.0.1"), 5002);

	try {
		while (true) {
			std::cout << "chord_prompt$ ";
			std::cin >> cmd;

			if (cmd == "join")
				node.cli(0);
			else if (cmd == "display")
				node.cli(1);
		}
	}
	catch (std::exception& e) {
		std::cout << e.what();
	}
	return 0;
}