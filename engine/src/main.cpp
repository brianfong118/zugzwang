#include "attacks.h"
#include "uci.h"

#include <iostream>

int main() {
    std::cout << "Zugzwang " ZUGZWANG_VERSION << '\n';
    zugzwang::attacks::init();
    zugzwang::uciLoop(std::cin, std::cout);
    return 0;
}
