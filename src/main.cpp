#include <iostream>

int main(int argc, char **argv) {
	//arguments will pass information about commands and parameters

	if (argc < 2) {
        std::cerr << "Error: No command provided." << std::endl;
        showHelp();
        return EXIT_FAILURE;
    }

    string command(argv[1]);

    if ()


	return EXIT_SUCCESS;
}