#include "chord.h"
#include "utils.h"
#include <iostream>
#include <string>


int main(int argc, char* argv[]) {
	try {
		if (argc != 4) {
			throw std::invalid_argument("Enter 3 argument: ID, IP, PORT");
		}
		check_users_params(std::atoi(argv[1]), std::string(argv[2]), std::atoi(argv[3]));
	}
	catch (const std::invalid_argument& e) {
		std::cout << e.what() << std::endl;
		exit(1);
	}

	std::cout << "*************************** CHORD **************************" << std::endl;

	chord_node node(std::atoi(argv[1]), std::string(argv[2]), std::atoi(argv[3]));
	std::string enter;

	while (true) {
		try {
			std::cout << "chord_prompt$ ";
			std::getline(std::cin, enter);

			// Check if user enter contains space and tabulations
			auto splitted_space = split(enter, " ");
			auto splitted_tab = split(enter, "\t");

			if (splitted_space.size() != 1 || splitted_tab.size() != 1) {
				throw std::invalid_argument("Wrong input.");
			}

			if (enter == "join") {
				node.cli(0);
			}
			else if (enter == "display") {
				node.cli(1);
			}
			else if (enter == "leave") {
				node.cli(2);
				break;
			}
			else if (enter == "cls") {
				system("cls");
				std::cout << "*************************** CHORD **************************" << std::endl;
			}
			else if (enter == "put") {
				node.cli(3);
			}
			else if (enter == "find") {
				node.cli(4);
			}
			else {
				throw std::invalid_argument("Invalid command.");
			}
		}
		catch (const std::invalid_argument& e) {
			std::cout << e.what() << std::endl;
		}
		catch (const std::runtime_error& e) {  // socket problem
			std::cout << e.what() << std::endl;
			return 1;
		}
	}

	return 0;
}