#include "eval.h"
#include "test_util.h"

#include <catch2/catch_test_macros.hpp>

using namespace zugzwang;

TEST_CASE("material is counted for the side to move", "[eval]") {
    CHECK(evaluate(parse(Position::kStartFen)) == 0);
    CHECK(evaluate(parse("4k3/8/8/8/8/8/8/1N2K3 w - - 0 1")) == 320);
    CHECK(evaluate(parse("4k3/8/8/8/8/8/8/1N2K3 b - - 0 1")) == -320);
    CHECK(evaluate(parse("r3k3/8/8/8/8/8/PPP5/4K3 w - - 0 1")) == -200);
}
