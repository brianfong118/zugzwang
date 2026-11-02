#include "bench.h"
#include "position.h"

#include <catch2/catch_test_macros.hpp>

#include <optional>

using namespace zugzwang;

TEST_CASE("every bench position parses and round-trips", "[bench]") {
    for (std::string_view fen : kBenchFens) {
        CAPTURE(fen);
        std::optional<Position> pos = Position::fromFen(fen);
        REQUIRE(pos.has_value());
        // fromFen drops castling rights without a king and rook at home, so a typo shows up here.
        CHECK(pos->fen() == fen);
    }
}
