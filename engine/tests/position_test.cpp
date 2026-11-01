#include "position.h"
#include "test_util.h"
#include "zobrist.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <bit>
#include <initializer_list>
#include <vector>

using namespace zugzwang;

TEST_CASE("start position parses", "[position]") {
    Position pos = parse(Position::kStartFen);
    CHECK(pos.sideToMove() == kWhite);
    CHECK(pos.castlingRights() == kAnyCastling);
    CHECK(pos.epSquare() == kNoSquare);
    CHECK(pos.halfmoveClock() == 0);
    CHECK(pos.fullmoveNumber() == 1);
    CHECK(pos.pieceOn(kE1) == kWhiteKing);
    CHECK(pos.pieceOn(kD8) == kBlackQueen);
    CHECK(pos.pieceOn(kE4) == kNoPiece);
    CHECK(pos.pieces(kWhite) == (kRank1 | rankBb(1)));
    CHECK(pos.pieces(kBlack) == (kRank8 | rankBb(6)));
    CHECK(std::popcount(pos.occupied()) == 32);
    CHECK(pos.pieces(kBlack, kPawn) == rankBb(6));
    CHECK(pos.kingSquare(kBlack) == kE8);
    CHECK_FALSE(pos.inCheck());
}

TEST_CASE("FEN round-trips on the perft suite", "[position]") {
    for (std::string_view fen : std::initializer_list<std::string_view>{
             Position::kStartFen, kKiwipete, "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
             "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
             "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
             "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
             "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
             "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"}) {
        CAPTURE(fen);
        CHECK(parse(fen).fen() == fen);
    }
}

TEST_CASE("missing trailing fields take their defaults", "[position]") {
    Position pos = parse("4k3/8/8/8/8/8/8/4K3 b");
    CHECK(pos.sideToMove() == kBlack);
    CHECK(pos.castlingRights() == kNoCastling);
    CHECK(pos.epSquare() == kNoSquare);
    CHECK(pos.fen() == "4k3/8/8/8/8/8/8/4K3 b - - 0 1");
}

TEST_CASE("castling rights without king and rook at home are dropped", "[position]") {
    CHECK(parse("r3k2r/8/8/8/8/8/8/R3K3 w KQkq - 0 1").castlingRights() ==
          (kWhiteQueenSide | kBlackKingSide | kBlackQueenSide));
    CHECK(parse("r3k2r/8/8/8/8/8/8/R4K1R w KQkq - 0 1").castlingRights() ==
          (kBlackKingSide | kBlackQueenSide));
}

TEST_CASE("malformed FENs are rejected", "[position]") {
    for (std::string_view fen : {
             "",
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",        // no side to move
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x",      // bad side
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPP/RNBQKBNR w",       // short rank
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPPP/RNBQKBNR w",     // long rank
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/8 w",    // nine ranks
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w",               // seven ranks
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNX w",      // bad piece
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQ1BNR w",      // no white king
             "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkx", // bad castling
             "4k3/8/8/8/8/8/8/4K3 w - e3",                         // ep with no pawn
             "4k3/8/8/8/4P3/8/8/4K3 w - e3",                       // ep on the wrong side
             "4k3/8/8/8/8/8/8/4K3 w - e9",                         // ep off the board
             "4k3/8/8/8/8/8/8/4K3 w - - x 1",                      // bad halfmove clock
             "4k3/8/8/8/8/8/8/4K3 w - - 0 0",                      // fullmove starts at 1
             "4k3/8/8/8/8/8/8/4K3 w - - -1 1",                     // negative clock
             "P3k3/8/8/8/8/8/8/4K3 w",                             // pawn on the back rank
             "4k3/8/8/8/8/PPPPPPPP/PPPPPPPP/4K3 w",                // seventeen white pieces
             "4k3/pppppppp/pppppppp/8/8/8/8/4K3 w",                // seventeen black pieces
         }) {
        CAPTURE(fen);
        CHECK_FALSE(Position::fromFen(fen).has_value());
    }
}

TEST_CASE("Zobrist keys are compile-time and distinct", "[zobrist]") {
    STATIC_REQUIRE(zobrist::piece(kWhitePawn, kA2) != zobrist::piece(kWhitePawn, kA3));
    STATIC_REQUIRE(zobrist::side() != 0);

    std::vector<Key> all;
    for (int p = 0; p < 12; ++p)
        for (int s = 0; s < 64; ++s)
            all.push_back(zobrist::piece(Piece(p), Square(s)));
    for (int c = 0; c < 16; ++c)
        all.push_back(zobrist::castling(c));
    for (int f = 0; f < 8; ++f)
        all.push_back(zobrist::enPassant(f));
    all.push_back(zobrist::side());
    std::sort(all.begin(), all.end());
    CHECK(std::adjacent_find(all.begin(), all.end()) == all.end());
    CHECK(all.front() != 0);
}

TEST_CASE("incremental key matches full recompute", "[zobrist]") {
    for (std::string_view fen : std::initializer_list<std::string_view>{
             Position::kStartFen, kKiwipete,
             "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3"}) {
        Position pos = parse(fen);
        CAPTURE(fen);
        CHECK(pos.key() == pos.computeKey());
    }
}

TEST_CASE("key depends on side, castling and a capturable en passant square", "[zobrist]") {
    Key base = parse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1").key();
    CHECK(base != parse("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1").key());
    CHECK(base != parse("r3k2r/8/8/8/8/8/8/R3K2R w Qkq - 0 1").key());
    CHECK(base == parse("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 7 42").key());

    // No white pawn can take on e6, so the ep square is irrelevant to the position.
    CHECK(parse("rnbqkbnr/pppp1ppp/8/4p3/8/8/PPPPPPPP/RNBQKBNR w KQkq e6 0 2").key() ==
          parse("rnbqkbnr/pppp1ppp/8/4p3/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 2").key());
    CHECK(parse("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3").key() !=
          parse("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3").key());
}

TEST_CASE("attackersTo finds every attacker of both colours", "[position]") {
    Position start = parse(Position::kStartFen);
    CHECK(start.attackersTo(kE3) == (squareBb(kD2) | squareBb(kF2)));
    CHECK(start.attackersTo(kF3) == (squareBb(kE2) | squareBb(kG2) | squareBb(kG1)));
    CHECK(start.attackersTo(kE4) == 0);

    Position kiwi = parse(kKiwipete);
    CHECK(kiwi.attackersTo(kF7) == (squareBb(kE5) | squareBb(kE7) | squareBb(kE8)));
    CHECK(kiwi.isAttacked(kD5, kBlack));
    CHECK(kiwi.isAttacked(kH3, kWhite));
    CHECK_FALSE(kiwi.inCheck());
    // Lifting the a2 pawn lets the a1 rook see a6.
    CHECK((kiwi.attackersTo(kA6) & squareBb(kA1)) == 0);
    CHECK((kiwi.attackersTo(kA6, kiwi.occupied() & ~squareBb(kA2)) & squareBb(kA1)) != 0);

    CHECK(parse("4k3/8/8/8/8/8/8/4K2r w - - 0 1").inCheck());
    CHECK(parse("4k3/8/8/8/8/8/3P4/4K3 b - - 0 1").inCheck() == false);
}
