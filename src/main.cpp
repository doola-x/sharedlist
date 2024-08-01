#include <iostream>
#include "include/user.hpp"

int main(int argc, char **argv) {
	//arguments will pass information about commands and parameters

	if (argc < 2) {
        std::cerr << "Error: No command provided." << std::endl;
        return EXIT_FAILURE;
    }

    string command(argv[1]);

    if (command == "users") {
        User *u = new User();
        u->getUserName(0);
        delete u;
    }


	return EXIT_SUCCESS;
}