#include "eval.h"
#include "movegen.h"
#include "search.h"
#include "test_util.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <initializer_list>
#include <stop_token>
#include <string_view>
#include <utility>
#include <vector>

using namespace zugzwang;

namespace {

// A new table for every search, so no test depends on what an earlier search stored.
SearchInfo searchFresh(Position& pos, const SearchLimits& limits, std::stop_token stop = {},
                       const SearchReport& report = {}) {
    TranspositionTable tt(1);
    return search(pos, limits, tt, stop, report);
}

SearchInfo searchFen(std::string_view fen, int depth) {
    Position pos = parse(fen);
    SearchInfo info = searchFresh(pos, {.depth = depth});
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

// Standing pat or the best capture, with every capture searched.
int quiescenceMinimax(Position& pos, int ply) {
    if (pos.isDraw(ply)) return 0;
    int best = evaluate(pos);
    if (ply >= kMaxPly) return best;
    for (Move m : generateNoisy(pos)) {
        if (!pos.make(m)) continue;
        best = std::max(best, -quiescenceMinimax(pos, ply + 1));
        pos.unmake(m);
    }
    return best;
}

// Searches every move, with no pruning and no ordering, for the search to agree with.
int minimax(Position& pos, int depth, int ply) {
    if (depth == 0) return quiescenceMinimax(pos, ply);
    if (ply > 0 && pos.isDraw(ply)) return 0;
    int best = -kInfinity;
    for (Move m : generateMoves(pos)) {
        if (!pos.make(m)) continue;
        best = std::max(best, -minimax(pos, depth - 1, ply + 1));
        pos.unmake(m);
    }
    if (best == -kInfinity) return pos.inCheck() ? -kMate + ply : 0;
    return best;
}

// Soft and hard limits in milliseconds; count() is long on Linux but long long on MinGW.
std::pair<long long, long long> budget(long long remaining, long long increment) {
    TimeBudget b =
        timeBudget(std::chrono::milliseconds(remaining), std::chrono::milliseconds(increment));
    return {b.soft.count(), b.hard.count()};
}

} // namespace

// Alpha-beta skips only lines that cannot change the result, whatever order it tries moves in.
// Endgames only: unpruned quiescence passes 200M nodes on Kiwipete even at depth 1.
TEST_CASE("search returns the minimax score", "[search]") {
    for (std::string_view fen : std::initializer_list<std::string_view>{
             kPosition3,
             "1K6/1P1k4/8/8/8/8/r7/2R5 w - - 0 1",
             "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1",
             "4k3/8/4p3/3p4/8/8/8/3QK3 w - - 0 1",
             "4k3/8/2p5/3p4/8/8/q2R4/4K3 w - - 0 1",
             "k7/8/2K5/8/8/8/8/7R w - - 0 1",
         }) {
        CAPTURE(fen);
        Position pos = parse(fen);
        CHECK(searchFresh(pos, {.depth = 4}).score == minimax(pos, 4, 0));
    }
}

TEST_CASE("search sees the recapture beyond its depth", "[search]") {
    // Without quiescence, depth 1 scores Qxd5 a pawn up and never sees exd5.
    SearchInfo info = searchFen("4k3/8/4p3/3p4/8/8/8/3QK3 w - - 0 1", 1);
    REQUIRE(info.pv.length > 0);
    CHECK(info.pv.moves[0] != Move(kD1, kD5));
}

TEST_CASE("search finds a mate in one", "[search]") {
    SearchInfo info = searchFen("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", 2);
    CHECK(info.depth == 2);
    CHECK(info.score == kMate - 1);
    REQUIRE(info.pv.length == 1);
    CHECK(info.pv.moves[0] == Move(kA1, kA8));
}

TEST_CASE("search finds a mate in two and its pv delivers it", "[search]") {
    Position pos = parse("k7/8/2K5/8/8/8/8/7R w - - 0 1");
    SearchInfo info = searchFresh(pos, {.depth = 4});
    CHECK(info.score == kMate - 3);
    REQUIRE(info.pv.length == 3);
    for (int i = 0; i < info.pv.length; ++i)
        REQUIRE(pos.make(info.pv.moves[i]));
    CHECK(pos.inCheck());
    CHECK_FALSE(hasLegalMove(pos));
}

TEST_CASE("the table stores mates from the node and returns them from the root", "[search]") {
    // Mate five plies from the root, stored at ply 2, is mate in three from that node.
    CHECK(scoreToTt(kMate - 5, 2) == kMate - 3);
    CHECK(scoreToTt(-kMate + 5, 2) == -kMate + 3);
    // Read at ply 4, by a transposition or the next move's search, it is mate seven plies away.
    CHECK(scoreFromTt(kMate - 3, 4) == kMate - 7);
    CHECK(scoreFromTt(-kMate + 3, 4) == -kMate + 7);
    CHECK(scoreFromTt(scoreToTt(kMateInMaxPly - 1, 9), 3) == kMateInMaxPly - 1);
}

TEST_CASE("a history entry approaches its bound and never passes it", "[search]") {
    CHECK(updatedHistory(0, 400) == 400);
    // Halfway to the bound, a bonus moves the entry half as far.
    CHECK(updatedHistory(kMaxHistory / 2, 400) == kMaxHistory / 2 + 200);
    CHECK(updatedHistory(-kMaxHistory, kMaxPly * kMaxPly) == kMaxHistory);

    int high = 0;
    int low = 0;
    for (int i = 0; i < 1000; ++i) {
        high = updatedHistory(high, 400);
        low = updatedHistory(low, -400);
        REQUIRE(high <= kMaxHistory);
        REQUIRE(low >= -kMaxHistory);
    }
    CHECK(high == kMaxHistory);
    CHECK(low == -kMaxHistory);
}

TEST_CASE("a second search reuses what the first stored", "[search]") {
    TranspositionTable tt(16);
    Position pos = parse(kKiwipete);
    SearchInfo first = search(pos, {.depth = 4}, tt);
    SearchInfo second = search(pos, {.depth = 4}, tt);
    CHECK(second.nodes < first.nodes);
    CHECK(second.pv.length > 0);
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
    CHECK(info.score > 0);
    REQUIRE(info.pv.length > 0);
    CHECK(info.pv.moves[0] == Move(kD2, kA2));
}

TEST_CASE("the side a queen down repeats the position a third time", "[search]") {
    Position pos = parse("4k3/8/8/8/8/8/8/Q3K3 w - - 0 1");
    play(pos, {Move(kA1, kA2), Move(kE8, kD8), Move(kA2, kA1), Move(kD8, kE8), Move(kA1, kA2),
               Move(kE8, kD8), Move(kA2, kA1)});

    SearchInfo info = searchFresh(pos, {.depth = 2});
    CHECK(info.score == 0);
    REQUIRE(info.pv.length > 0);
    CHECK(info.pv.moves[0] == Move(kD8, kE8));
}

TEST_CASE("depth 1 completes even when a limit comes first", "[search]") {
    Position pos = parse(Position::kStartFen);
    SearchInfo info = searchFresh(pos, {.nodes = 1});
    CHECK(info.depth == 1);
    CHECK(info.pv.length == 1);
    CHECK(pos.fen() == Position::kStartFen);
}

TEST_CASE("a stop request ends an otherwise unlimited search", "[search]") {
    std::stop_source source;
    source.request_stop();
    Position pos = parse(Position::kStartFen);
    SearchInfo info = searchFresh(pos, {}, source.get_token());
    CHECK(info.depth >= 1);
    CHECK(info.pv.length > 0);
}

TEST_CASE("a node limit discards the iteration it interrupts", "[search]") {
    Position pos = parse(Position::kStartFen);
    SearchInfo full = searchFresh(pos, {.depth = 3});
    SearchInfo limited = searchFresh(pos, {.nodes = full.nodes + 1});
    CHECK(limited.depth == 3);
    CHECK(limited.nodes == full.nodes);
}

TEST_CASE("a hard time limit ends an otherwise unlimited search", "[search]") {
    Position pos = parse(Position::kStartFen);
    SearchInfo info = searchFresh(pos, {.hardTime = std::chrono::milliseconds(20)});
    CHECK(info.depth >= 1);
    CHECK(info.pv.length > 0);
}

TEST_CASE("each completed depth is reported once, in order", "[search]") {
    Position pos = parse(Position::kStartFen);
    std::vector<int> depths;
    SearchInfo last = searchFresh(pos, {.depth = 4}, {},
                                  [&](const SearchInfo& info) { depths.push_back(info.depth); });
    CHECK(depths == std::vector<int>{1, 2, 3, 4});
    CHECK(last.depth == 4);

    depths.clear();
    Position stalemate = parse("k7/8/1Q6/8/8/8/8/7K b - - 0 1");
    searchFresh(stalemate, {}, {}, [&](const SearchInfo& info) { depths.push_back(info.depth); });
    CHECK(depths == std::vector<int>{1});
}

TEST_CASE("a soft time limit finishes the depth in progress and starts no other", "[search]") {
    Position pos = parse(Position::kStartFen);
    SearchInfo soft = searchFresh(pos, {.softTime = std::chrono::milliseconds(0)});
    CHECK(soft.depth == 1);
    CHECK(soft.nodes == searchFresh(pos, {.depth = 1}).nodes);
}

TEST_CASE("the time budget spreads the clock and caps it when it runs low", "[search]") {
    CHECK(budget(180'000, 2'000) == std::pair{4'000LL, 18'000LL});
    CHECK(budget(60'000, 0) == std::pair{1'000LL, 6'000LL});
    // remaining/30 + increment would be 2333 ms, almost a quarter of what is left.
    CHECK(budget(10'000, 2'000) == std::pair{500LL, 1'000LL});
    CHECK(budget(0, 0) == std::pair{0LL, 0LL});
}
