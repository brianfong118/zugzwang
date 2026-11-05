#include "bench.h"
#include "eval.h"
#include "test_util.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>

using namespace zugzwang;

namespace {

// Flips the board top to bottom and swaps the colours, which hands the same game to the other side.
std::string mirror(std::string_view fen) {
    std::istringstream in{std::string(fen)};
    std::string placement, side, castling, ep, clocks;
    in >> placement >> side >> castling >> ep;
    std::getline(in, clocks);

    std::string flipped;
    std::istringstream ranks(placement);
    for (std::string rank; std::getline(ranks, rank, '/');)
        flipped = flipped.empty() ? rank : rank + '/' + flipped;
    auto swapCase = [](char c) {
        return char(std::isupper(c) ? std::tolower(c) : std::toupper(c));
    };
    std::transform(flipped.begin(), flipped.end(), flipped.begin(), swapCase);
    std::transform(castling.begin(), castling.end(), castling.begin(), swapCase);
    if (ep != "-") ep[1] = ep[1] == '3' ? '6' : '3';
    return flipped + (side == "w" ? " b " : " w ") + castling + ' ' + ep + clocks;
}

} // namespace

TEST_CASE("the mirror helper flips the board and swaps the colours", "[eval]") {
    CHECK(mirror("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1") ==
          "rnbqkbnr/pppp1ppp/8/4p3/8/8/PPPPPPPP/RNBQKBNR w kqKQ e6 0 1");
}

TEST_CASE("the start position is level", "[eval]") {
    CHECK(evaluate(parse(Position::kStartFen)) == 0);
}

TEST_CASE("a knight on b1 is worth its PeSTO value, blended by phase", "[eval]") {
    // The kings cancel. Phase 1 of 24 blends mg 337 - 21 and eg 281 - 51: (316 + 23 * 230) / 24.
    CHECK(evaluate(parse("4k3/8/8/8/8/8/8/1N2K3 w - - 0 1")) == 233);
    CHECK(evaluate(parse("4k3/8/8/8/8/8/8/1N2K3 b - - 0 1")) == -233);
}

TEST_CASE("mirroring the position leaves the side to move's score unchanged", "[eval]") {
    for (std::string_view fen : kBenchFens) {
        CAPTURE(fen);
        CHECK(evaluate(parse(fen)) == evaluate(parse(mirror(fen))));
    }
}
