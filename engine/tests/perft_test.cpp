#include "perft.h"
#include "test_util.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <initializer_list>
#include <string_view>

using namespace zugzwang;

namespace {

constexpr std::string_view kPosition4Mirrored =
    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1";

// Expected counts are the published values from the Chess Programming Wiki's "Perft Results" page.
struct Case {
    std::string_view fen;
    int depth;
    std::uint64_t nodes;
};

void checkPerft(std::initializer_list<Case> cases) {
    for (const Case& c : cases) {
        CAPTURE(c.fen, c.depth);
        Position pos = parse(c.fen);
        CHECK(perft(pos, c.depth) == c.nodes);
        CHECK(pos.fen() == c.fen);
    }
}

} // namespace

TEST_CASE("perft matches published counts at shallow depth", "[perft]") {
    checkPerft({
        {Position::kStartFen, 1, 20},
        {Position::kStartFen, 2, 400},
        {Position::kStartFen, 3, 8902},
        {Position::kStartFen, 4, 197281},
        {kKiwipete, 1, 48},
        {kKiwipete, 2, 2039},
        {kKiwipete, 3, 97862},
        {kPosition3, 1, 14},
        {kPosition3, 2, 191},
        {kPosition3, 3, 2812},
        {kPosition3, 4, 43238},
        {kPosition3, 5, 674624},
        {kPosition4, 1, 6},
        {kPosition4, 2, 264},
        {kPosition4, 3, 9467},
        {kPosition4, 4, 422333},
        {kPosition4Mirrored, 1, 6},
        {kPosition4Mirrored, 2, 264},
        {kPosition4Mirrored, 3, 9467},
        {kPosition4Mirrored, 4, 422333},
        {kPosition5, 1, 44},
        {kPosition5, 2, 1486},
        {kPosition5, 3, 62379},
        {kPosition6, 1, 46},
        {kPosition6, 2, 2079},
        {kPosition6, 3, 89890},
    });
}

TEST_CASE("perft matches published counts at full depth", "[perft][slow]") {
    checkPerft({
        {Position::kStartFen, 5, 4865609},
        {Position::kStartFen, 6, 119060324},
        {kKiwipete, 4, 4085603},
        {kKiwipete, 5, 193690690},
        {kPosition3, 6, 11030083},
        {kPosition4, 5, 15833292},
        {kPosition4Mirrored, 5, 15833292},
        {kPosition5, 4, 2103487},
        {kPosition5, 5, 89941194},
        {kPosition6, 4, 3894594},
        {kPosition6, 5, 164075551},
    });
}
