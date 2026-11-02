#include "attacks.h"
#include "uci.h"

#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "Zugzwang " ZUGZWANG_VERSION << '\n';
    zugzwang::attacks::init();

    // Arguments run as one command and exit, as in Stockfish, so tools can call `zugzwang bench`.
    if (argc > 1) {
        std::string command;
        for (int i = 1; i < argc; ++i)
            command += std::string(argv[i]) + ' ';
        std::istringstream in(command);
        zugzwang::uciLoop(in, std::cout);
    } else {
        zugzwang::uciLoop(std::cin, std::cout);
    }
    return 0;
}
