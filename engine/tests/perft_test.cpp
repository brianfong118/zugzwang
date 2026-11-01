#include "perft.h"
#include "test_util.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <initializer_list>
#include <string_view>

using namespace zugzwang;

namespace {

// Expected counts are the published values from the Chess Programming Wiki's "Perft Results" page.
constexpr std::string_view kPosition3 = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
constexpr std::string_view kPosition4 =
    "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
constexpr std::string_view kPosition4Mirrored =
    "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1";
constexpr std::string_view kPosition5 = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
constexpr std::string_view kPosition6 =
    "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";

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
