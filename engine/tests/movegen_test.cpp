#include "movegen.h"
#include "test_util.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace zugzwang;

namespace {

using Moves = std::vector<std::string>;

Moves movesFrom(std::string_view fen, Square from) {
    Moves out;
    for (Move m : generateMoves(parse(fen)))
        if (m.from() == from) out.push_back(toUci(m));
    std::sort(out.begin(), out.end());
    return out;
}

bool generates(std::string_view fen, Move move) {
    MoveList moves = generateMoves(parse(fen));
    return std::find(moves.begin(), moves.end(), move) != moves.end();
}

} // namespace

TEST_CASE("Move packs squares and flag without overlap", "[movegen]") {
    constexpr Move promotion(kH7, kG8, kPromoteQueen);
    STATIC_REQUIRE(promotion.from() == kH7);
    STATIC_REQUIRE(promotion.to() == kG8);
    STATIC_REQUIRE(promotion.flag() == kPromoteQueen);
    STATIC_REQUIRE(promotion.isPromotion());
    STATIC_REQUIRE(promotion.promotionType() == kQueen);
    STATIC_REQUIRE(Move(kB7, kA8, kPromoteKnight).promotionType() == kKnight);
    STATIC_REQUIRE_FALSE(Move(kE1, kG1, kCastling).isPromotion());
    STATIC_REQUIRE(sizeof(Move) == 2);
    STATIC_REQUIRE(std::is_trivially_default_constructible_v<Move>);

    CHECK(toUci(Move(kE2, kE4)) == "e2e4");
    CHECK(toUci(Move(kE1, kG1, kCastling)) == "e1g1");
    CHECK(toUci(Move(kE5, kD6, kEnPassant)) == "e5d6");
    CHECK(toUci(Move(kB7, kA8, kPromoteKnight)) == "b7a8n");
    CHECK(toUci(Move(kH2, kH1, kPromoteQueen)) == "h2h1q");
}

// Counted by hand. Where pseudo-legal differs from the published legal count, the note says why.
TEST_CASE("pseudo-legal move counts on the perft suite", "[movegen]") {
    for (auto [fen, count] : std::vector<std::pair<std::string_view, int>>{
             {Position::kStartFen, 20},
             {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", 20},
             {kKiwipete, 48},
             // 14 legal: the b5 pawn is pinned and b6 is attacked.
             {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 16},
             // 6 legal: white is in check from the b6 bishop.
             {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 38},
             {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 44},
             {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 46}}) {
        CAPTURE(fen);
        CHECK(generateMoves(parse(fen)).size() == count);
    }
}

TEST_CASE("pawn pushes stop at blockers", "[movegen]") {
    CHECK(movesFrom("4k3/8/8/8/8/8/4P3/4K3 w - - 0 1", kE2) == Moves{"e2e3", "e2e4"});
    CHECK(movesFrom("4k3/8/8/8/4p3/8/4P3/4K3 w - - 0 1", kE2) == Moves{"e2e3"});
    CHECK(movesFrom("4k3/8/8/8/8/4p3/4P3/4K3 w - - 0 1", kE2).empty());
    CHECK(movesFrom("4k3/8/8/8/8/4P3/8/4K3 w - - 0 1", kE3) == Moves{"e3e4"});
    CHECK(movesFrom("4k3/4p3/8/8/8/8/8/4K3 b - - 0 1", kE7) == Moves{"e7e5", "e7e6"});
    CHECK(movesFrom("4k3/4p3/8/4P3/8/8/8/4K3 b - - 0 1", kE7) == Moves{"e7e6"});
}

TEST_CASE("pawn captures take enemies only and do not wrap around the board", "[movegen]") {
    CHECK(movesFrom("4k3/8/8/8/8/3p1P2/4P3/4K3 w - - 0 1", kE2) == Moves{"e2d3", "e2e3", "e2e4"});
    CHECK(movesFrom("4k3/8/8/4p3/3P1P2/8/8/4K3 b - - 0 1", kE5) == Moves{"e5d4", "e5e4", "e5f4"});
    // Without the file masks in shift(), h2 would see a4 and a2 would see h2.
    CHECK(movesFrom("4k3/8/8/8/p7/8/7P/4K3 w - - 0 1", kH2) == Moves{"h2h3", "h2h4"});
    CHECK(movesFrom("4k3/8/8/8/8/8/P6p/4K3 w - - 0 1", kA2) == Moves{"a2a3", "a2a4"});
}

TEST_CASE("promotions generate all four pieces", "[movegen]") {
    CHECK(movesFrom("1n2k3/P7/8/8/8/8/8/4K3 w - - 0 1", kA7) ==
          Moves{"a7a8b", "a7a8n", "a7a8q", "a7a8r", "a7b8b", "a7b8n", "a7b8q", "a7b8r"});
    CHECK(movesFrom("4k3/8/8/8/8/8/7p/4K1N1 b - - 0 1", kH2) ==
          Moves{"h2g1b", "h2g1n", "h2g1q", "h2g1r", "h2h1b", "h2h1n", "h2h1q", "h2h1r"});
}

TEST_CASE("en passant comes only from the stored square", "[movegen]") {
    CHECK(movesFrom("4k3/8/8/3pP3/8/8/8/4K3 w - - 0 1", kE5) == Moves{"e5e6"});
    CHECK(movesFrom("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", kE5) == Moves{"e5d6", "e5e6"});
    CHECK(generates("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", Move(kE5, kD6, kEnPassant)));
    CHECK(generates("4k3/8/8/2PpP3/8/8/8/4K3 w - d6 0 1", Move(kC5, kD6, kEnPassant)));
    CHECK(generates("4k3/8/8/2PpP3/8/8/8/4K3 w - d6 0 1", Move(kE5, kD6, kEnPassant)));
    CHECK(generates("4k3/8/8/8/3Pp3/8/8/4K3 b - d3 0 1", Move(kE4, kD3, kEnPassant)));
}

TEST_CASE("sliders stop at their own pieces and capture enemies", "[movegen]") {
    CHECK(movesFrom("4k3/8/8/8/R2p4/8/P7/4K3 w - - 0 1", kA4) ==
          Moves{"a4a3", "a4a5", "a4a6", "a4a7", "a4a8", "a4b4", "a4c4", "a4d4"});
    CHECK(movesFrom("4k3/8/8/8/5p2/8/1P6/2B1K3 w - - 0 1", kC1) == Moves{"c1d2", "c1e3", "c1f4"});
    CHECK(movesFrom("4k3/8/8/8/3Q4/8/8/4K3 w - - 0 1", kD4).size() == 27);
}

TEST_CASE("leapers ignore blockers but not their own pieces", "[movegen]") {
    CHECK(movesFrom("4k3/8/8/8/8/8/PPP5/N3K3 w - - 0 1", kA1) == Moves{"a1b3"});
    CHECK(movesFrom("4k3/8/8/8/8/8/3PPP2/3QKB2 w - - 0 1", kE1).empty());
}

TEST_CASE("castling needs the right and an empty path", "[movegen]") {
    constexpr std::string_view kOpen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    CHECK(generates(kOpen, Move(kE1, kG1, kCastling)));
    CHECK(generates(kOpen, Move(kE1, kC1, kCastling)));
    CHECK(generates("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", Move(kE8, kG8, kCastling)));
    CHECK(generates("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", Move(kE8, kC8, kCastling)));

    CHECK_FALSE(generates("r3k2r/8/8/8/8/8/8/R3K2R w Qkq - 0 1", Move(kE1, kG1, kCastling)));
    CHECK_FALSE(generates("r3k2r/8/8/8/8/8/8/R3K2R w Kkq - 0 1", Move(kE1, kC1, kCastling)));
    CHECK_FALSE(generates("r3k2r/8/8/8/8/8/8/R3K1NR w KQkq - 0 1", Move(kE1, kG1, kCastling)));
    // The king never crosses b1, but the rook does.
    CHECK_FALSE(generates("r3k2r/8/8/8/8/8/8/RN2K2R w KQkq - 0 1", Move(kE1, kC1, kCastling)));

    // Castling through an attacked square is rejected at make time, not here.
    CHECK(generates("r3k2r/8/8/8/5r2/8/8/R3K2R w KQkq - 0 1", Move(kE1, kG1, kCastling)));
}
