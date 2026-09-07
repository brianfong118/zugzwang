#include "attacks.h"

#include <catch2/catch_session.hpp>

int main(int argc, char* argv[]) {
    zugzwang::attacks::init();
    return Catch::Session().run(argc, argv);
}
