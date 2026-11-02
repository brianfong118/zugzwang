#include "movegen.h"
#include "search.h"
#include "test_util.h"

#include <catch2/catch_test_macros.hpp>

#include <initializer_list>
#include <string_view>

using namespace zugzwang;

namespace {

SearchInfo searchFen(std::string_view fen, int depth) {
    Position pos = parse(fen);
    SearchInfo info = search(pos, depth);
    CHECK(pos.fen() == fen);
    return info;
}

bool hasLegalMove(Position& pos) {
    for (Move m : generateMoves(pos)) {
        if (pos.make(m)) {
            pos.unmake(m);
            return true;
        }
    }
    return false;
}

} // namespace

TEST_CASE("search finds a mate in one", "[search]") {
    SearchInfo info = searchFen("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", 2);
    CHECK(info.depth == 2);
    CHECK(info.score == kMate - 1);
    REQUIRE(info.pv.length == 1);
    CHECK(info.pv.moves[0] == Move(kA1, kA8));
}

TEST_CASE("search finds a mate in two and its pv delivers it", "[search]") {
    Position pos = parse("k7/8/2K5/8/8/8/8/7R w - - 0 1");
    SearchInfo info = search(pos, 4);
    CHECK(info.score == kMate - 3);
    REQUIRE(info.pv.length == 3);
    for (int i = 0; i < info.pv.length; ++i)
        REQUIRE(pos.make(info.pv.moves[i]));
    CHECK(pos.inCheck());
    CHECK_FALSE(hasLegalMove(pos));
}

TEST_CASE("a root with no legal move scores mate or stalemate and has no pv", "[search]") {
    SearchInfo mated = searchFen("k7/1Q6/1K6/8/8/8/8/8 b - - 0 1", 3);
    CHECK(mated.score == -kMate);
    CHECK(mated.pv.length == 0);

    SearchInfo stalemate = searchFen("k7/8/1Q6/8/8/8/8/7K b - - 0 1", 3);
    CHECK(stalemate.score == 0);
    CHECK(stalemate.pv.length == 0);
}

TEST_CASE("search takes the free queen, not the pawn that is defended", "[search]") {
    SearchInfo info = searchFen("4k3/8/2p5/3p4/8/8/q2R4/4K3 w - - 0 1", 2);
    CHECK(info.score == 300);
    REQUIRE(info.pv.length > 0);
    CHECK(info.pv.moves[0] == Move(kD2, kA2));
}

TEST_CASE("the side a queen down repeats the position a third time", "[search]") {
    Position pos = parse("4k3/8/8/8/8/8/8/Q3K3 w - - 0 1");
    for (Move m : {Move(kA1, kA2), Move(kE8, kD8), Move(kA2, kA1), Move(kD8, kE8), Move(kA1, kA2),
                   Move(kE8, kD8), Move(kA2, kA1)})
        REQUIRE(pos.make(m));

    SearchInfo info = search(pos, 2);
    CHECK(info.score == 0);
    REQUIRE(info.pv.length > 0);
    CHECK(info.pv.moves[0] == Move(kD8, kE8));
}
