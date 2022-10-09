#include "chord.h"
#include <iostream>
#include <string>


int main(int argc, char* argv[]) {
	std::cout << "*************************** CHORD **************************" << std::endl;

	std::string cmd;

	chord_node node(std::atoi(argv[1]), std::string(argv[2]), std::atoi(argv[3]));
	//chord_node node(1, std::string("127.0.0.1"), 5001);

	try {
		while (true) {
			std::cout << "chord_prompt$ ";
			std::cin >> cmd;

			if (cmd == "join")
				node.cli(0);
			else if (cmd == "display")
				node.cli(1);
			else if (cmd == "find_successor")
				node.cli(2);
			else if (cmd == "find_predecessor")
				node.cli(3);
		}
	}
	catch(std::exception & e) {
		std::cout << e.what();
	}
	return 0;
}