
#include "CommandParser.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    bool multiLevel = false;

    if (argc == 2 && std::string(argv[1]) == "--multi") {
        multiLevel = true;
    } else if (argc != 1) {
        std::cerr << "Usage: ./vm_simulator [--multi]\n";
        return 1;
    }

    std::cout << "Virtual Memory Paging Simulator ("
              << (multiLevel ? "Multi-Level" : "Single-Level")
              << ")\n";

    CommandParser parser(multiLevel);
    parser.run(std::cin);

    return 0;
}
