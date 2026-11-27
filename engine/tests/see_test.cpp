#include "see.h"
#include "test_util.h"

#include <catch2/catch_test_macros.hpp>

#include <string_view>

using namespace zugzwang;

namespace {

constexpr int kP = kSeeValues[kPawn];
constexpr int kN = kSeeValues[kKnight];
constexpr int kR = kSeeValues[kRook];

// The exchange is worth exactly value: seeGe holds there and fails one above.
void checkSee(std::string_view fen, Move m, int value) {
    CAPTURE(fen, value);
    Position pos = parse(fen);
    CHECK(seeGe(pos, m, value));
    CHECK_FALSE(seeGe(pos, m, value + 1));
}

} // namespace

TEST_CASE("a capture nothing can answer wins the victim", "[see]") {
    checkSee("n5k1/8/8/8/8/8/8/R5K1 w - - 0 1", Move(kA1, kA8), kN);
}

TEST_CASE("a rook that takes a pawn defended by a pawn loses the difference", "[see]") {
    checkSee("6k1/8/2p5/3p4/8/8/8/3R2K1 w - - 0 1", Move(kD1, kD5), kP - kR);
}

TEST_CASE("minor pieces trade evenly", "[see]") {
    checkSee("6k1/8/4p3/3n4/8/4N3/8/6K1 w - - 0 1", Move(kE3, kD5), 0);
    checkSee("6k1/8/4p3/3n4/8/1B6/8/6K1 w - - 0 1", Move(kB3, kD5), 0);
}

TEST_CASE("a rook behind the capturing rook joins the exchange", "[see]") {
    checkSee("3r2k1/8/8/3p4/8/8/3R4/3R2K1 w - - 0 1", Move(kD2, kD5), kP);
}

TEST_CASE("a rook behind the recapturing rook joins once it has taken", "[see]") {
    checkSee("3r2k1/3r4/8/3p4/8/8/3R4/3R2K1 w - - 0 1", Move(kD2, kD5), kP - kR);
}

TEST_CASE("a pawn's capture uncovers the bishop behind it", "[see]") {
    // dxe5 fxe5 Bxe5 keeps the knight, where stopping after fxe5 would net a pawn less.
    checkSee("6k1/8/5p2/4n3/3P4/2B5/8/6K1 w - - 0 1", Move(kD4, kE5), kN);
}

TEST_CASE("the king recaptures only on a square the other side no longer attacks", "[see]") {
    checkSee("4k3/4p3/8/8/8/8/8/4R1K1 w - - 0 1", Move(kE1, kE7), kP - kR);
    checkSee("4k3/4p3/8/8/1B6/8/8/4R1K1 w - - 0 1", Move(kE1, kE7), kP);
}

TEST_CASE("a quiet move onto an attacked square loses the piece", "[see]") {
    constexpr std::string_view kFen = "6k1/8/8/4p3/8/8/4N3/6K1 w - - 0 1";
    checkSee(kFen, Move(kE2, kD4), -kN);
    checkSee(kFen, Move(kE2, kC3), 0);
}

TEST_CASE("en passant and promotions count as even", "[see]") {
    checkSee("6k1/8/8/3pP3/8/8/8/6K1 w - d6 0 1", Move(kE5, kD6, kEnPassant), 0);
    checkSee("6k1/P7/8/8/8/8/8/6K1 w - - 0 1", Move(kA7, kA8, kPromoteQueen), 0);
}
