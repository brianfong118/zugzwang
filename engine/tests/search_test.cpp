#include "movegen.h"
#include "search.h"
#include "test_util.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <initializer_list>
#include <stop_token>
#include <string_view>
#include <vector>

using namespace zugzwang;

namespace {

SearchInfo searchFen(std::string_view fen, int depth) {
    Position pos = parse(fen);
    SearchInfo info = search(pos, {.depth = depth});
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
    SearchInfo info = search(pos, {.depth = 4});
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

    SearchInfo info = search(pos, {.depth = 2});
    CHECK(info.score == 0);
    REQUIRE(info.pv.length > 0);
    CHECK(info.pv.moves[0] == Move(kD8, kE8));
}

TEST_CASE("depth 1 completes even when a limit comes first", "[search]") {
    Position pos = parse(Position::kStartFen);
    SearchInfo info = search(pos, {.nodes = 1});
    CHECK(info.depth == 1);
    CHECK(info.pv.length == 1);
    CHECK(pos.fen() == Position::kStartFen);
}

TEST_CASE("a stop request ends an otherwise unlimited search", "[search]") {
    std::stop_source source;
    source.request_stop();
    Position pos = parse(Position::kStartFen);
    SearchInfo info = search(pos, {}, source.get_token());
    CHECK(info.depth >= 1);
    CHECK(info.pv.length > 0);
}

TEST_CASE("a node limit discards the iteration it interrupts", "[search]") {
    Position pos = parse(Position::kStartFen);
    SearchInfo full = search(pos, {.depth = 3});
    SearchInfo limited = search(pos, {.nodes = full.nodes + 1});
    CHECK(limited.depth == 3);
    CHECK(limited.nodes == full.nodes);
}

TEST_CASE("a time limit ends an otherwise unlimited search", "[search]") {
    Position pos = parse(Position::kStartFen);
    SearchInfo info = search(pos, {.time = std::chrono::milliseconds(20)});
    CHECK(info.depth >= 1);
    CHECK(info.pv.length > 0);
}

TEST_CASE("each completed depth is reported once, in order", "[search]") {
    Position pos = parse(Position::kStartFen);
    std::vector<int> depths;
    SearchInfo last = search(pos, {.depth = 4}, {},
                             [&](const SearchInfo& info) { depths.push_back(info.depth); });
    CHECK(depths == std::vector<int>{1, 2, 3, 4});
    CHECK(last.depth == 4);

    depths.clear();
    Position stalemate = parse("k7/8/1Q6/8/8/8/8/7K b - - 0 1");
    search(stalemate, {}, {}, [&](const SearchInfo& info) { depths.push_back(info.depth); });
    CHECK(depths == std::vector<int>{1});
}

TEST_CASE("the time budget spreads the clock and caps it when it runs low", "[search]") {
    using std::chrono::milliseconds;
    CHECK(timeBudget(milliseconds(180'000), milliseconds(2'000)) == milliseconds(8'000));
    CHECK(timeBudget(milliseconds(60'000), milliseconds(0)) == milliseconds(2'000));
    // remaining/30 + increment would be 2333 ms, almost a quarter of what is left.
    CHECK(timeBudget(milliseconds(10'000), milliseconds(2'000)) == milliseconds(1'000));
    CHECK(timeBudget(milliseconds(0), milliseconds(0)) == milliseconds(0));
}
