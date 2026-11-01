#include "movegen.h"
#include "position.h"
#include "prng.h"
#include "test_util.h"
#include "zobrist.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <bit>
#include <initializer_list>
#include <string>
#include <vector>

using namespace zugzwang;

namespace {

// Rebuilding from the FEN catches any disagreement between the mailbox, bitboards, rights and key.
void requireMatchesFreshParse(const Position& pos) {
    Position fresh = parse(pos.fen());
    for (int p = 0; p < 12; ++p)
        REQUIRE(pos.pieces(Piece(p)) == fresh.pieces(Piece(p)));
    REQUIRE(pos.pieces(kWhite) == fresh.pieces(kWhite));
    REQUIRE(pos.pieces(kBlack) == fresh.pieces(kBlack));
    REQUIRE(pos.castlingRights() == fresh.castlingRights());
    REQUIRE(pos.key() == fresh.key());
    REQUIRE(pos.key() == pos.computeKey());
}

void checkIllegal(std::string_view fen, Move m) {
    Position pos = parse(fen);
    Key key = pos.key();
    CAPTURE(fen, toUci(m));
    CHECK_FALSE(pos.make(m));
    CHECK(pos.fen() == fen);
    CHECK(pos.key() == key);
}

void checkLegal(std::string_view fen, Move m) {
    Position pos = parse(fen);
    CAPTURE(fen, toUci(m));
    CHECK(pos.make(m));
}

} // namespace

TEST_CASE("start position parses", "[position]") {
    Position pos = parse(Position::kStartFen);
    CHECK(pos.sideToMove() == kWhite);
    CHECK(pos.castlingRights() ==
          (kWhiteKingSide | kWhiteQueenSide | kBlackKingSide | kBlackQueenSide));
    CHECK(pos.epSquare() == kNoSquare);
    CHECK(pos.halfmoveClock() == 0);
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
             Position::kStartFen, kKiwipete, kPosition3, kPosition4, kPosition5, kPosition6,
             "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
             "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
             "4k3/8/8/8/8/8/8/4K3 w - - 1000000 1000000"}) {
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
             "4k3/8/8/8/8/8/8/4K3 w - e6",                         // ep with no pawn
             "4k3/8/4n3/4p3/8/8/8/4K3 w - e6",                     // ep square occupied
             "3nk3/3n4/8/3pP3/8/8/8/4K3 w - d6",                   // ep pawn's origin occupied
             "4k3/8/8/8/4P3/8/8/4K3 w - e3",                       // ep on the wrong side
             "4k3/8/8/8/8/8/8/4K3 w - e9",                         // ep off the board
             "4k3/8/8/8/8/8/8/4K3 w - - x 1",                      // bad halfmove clock
             "4k3/8/8/8/8/8/8/4K3 w - - 0 0",                      // fullmove starts at 1
             "4k3/8/8/8/8/8/8/4K3 w - - -1 1",                     // negative clock
             "4k3/8/8/8/8/8/8/4K3 w - - 1000001 1",                // halfmove clock too large
             "4k3/8/8/8/8/8/8/4K3 w - - 0 1000001",                // fullmove number too large
             "4k3/8/8/8/8/8/8/4K3 w - - 0 1 x",                    // seventh field
             "P3k3/8/8/8/8/8/8/4K3 w",                             // pawn on the back rank
             "4k3/8/8/8/8/PPPPPPPP/PPPPPPPP/4K3 w",                // seventeen white pieces
             "4k3/pppppppp/pppppppp/8/8/8/8/4K3 w",                // seventeen black pieces
             "4k2R/8/8/8/8/8/8/4K3 w",                             // black in check, white to move
             "4K2r/8/8/8/8/8/8/4k3 b",                             // white in check, black to move
             "8/8/8/3Kk3/8/8/8/8 w",                               // adjacent kings
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

TEST_CASE("isAttacked looks only at the given colour", "[position]") {
    Position start = parse(Position::kStartFen);
    CHECK(start.isAttacked(kE3, kWhite));
    CHECK_FALSE(start.isAttacked(kE3, kBlack));
    CHECK(start.isAttacked(kF6, kBlack));
    CHECK_FALSE(start.isAttacked(kF6, kWhite));
    CHECK_FALSE(start.isAttacked(kE4, kWhite));
    CHECK_FALSE(start.isAttacked(kE4, kBlack));

    Position kiwi = parse(kKiwipete);
    CHECK(kiwi.isAttacked(kF7, kWhite));
    CHECK(kiwi.isAttacked(kF7, kBlack));
    CHECK(kiwi.isAttacked(kD5, kBlack));
    CHECK(kiwi.isAttacked(kH3, kWhite));
}

TEST_CASE("inCheck asks about the side to move", "[position]") {
    CHECK_FALSE(parse(Position::kStartFen).inCheck());
    CHECK_FALSE(parse(kKiwipete).inCheck());
    CHECK(parse("4k3/8/8/8/8/8/8/4K2r w - - 0 1").inCheck());
    CHECK_FALSE(parse("4k3/8/8/8/8/8/3P4/4K3 b - - 0 1").inCheck());
}

TEST_CASE("make updates all state and unmake restores it", "[make]") {
    struct Case {
        std::string_view fen;
        Move move;
        std::string_view after;
    };
    constexpr std::string_view kOpen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    constexpr std::string_view kOpenBlack = "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1";

    for (const Case& c : std::initializer_list<Case>{
             // Clocks: the halfmove clock resets on pawn moves and captures only.
             {Position::kStartFen, Move(kG1, kF3),
              "rnbqkbnr/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R b KQkq - 1 1"},
             {"4k3/8/8/8/8/4P3/8/4K3 w - - 5 10", Move(kE3, kE4),
              "4k3/8/8/8/4P3/8/8/4K3 b - - 0 10"},
             {"4k3/8/8/3p4/4P3/8/8/4K3 w - - 5 10", Move(kE4, kD5),
              "4k3/8/8/3P4/8/8/8/4K3 b - - 0 10"},
             // Double pushes set the en passant square, and the next move replaces it.
             {Position::kStartFen, Move(kE2, kE4),
              "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"},
             {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1", Move(kE7, kE5),
              "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq e6 0 2"},
             // En passant removes the pawn beside the capturer, not one on the target square.
             {"4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2", Move(kE5, kD6, kEnPassant),
              "4k3/8/3P4/8/8/8/8/4K3 b - - 0 2"},
             {"4k3/8/8/8/3Pp3/8/8/4K3 b - d3 0 2", Move(kE4, kD3, kEnPassant),
              "4k3/8/8/8/8/3p4/8/4K3 w - - 0 3"},
             // Promotions, quiet and capturing.
             {"4k3/P7/8/8/8/8/8/4K3 w - - 3 9", Move(kA7, kA8, kPromoteQueen),
              "Q3k3/8/8/8/8/8/8/4K3 b - - 0 9"},
             {"1n2k3/P7/8/8/8/8/8/4K3 w - - 0 1", Move(kA7, kB8, kPromoteKnight),
              "1N2k3/8/8/8/8/8/8/4K3 b - - 0 1"},
             {"4k3/8/8/8/8/8/7p/4K1N1 b - - 0 1", Move(kH2, kG1, kPromoteRook),
              "4k3/8/8/8/8/8/8/4K1r1 w - - 0 2"},
             // Castling moves the rook too and ends both of that side's rights.
             {kOpen, Move(kE1, kG1, kCastling), "r3k2r/8/8/8/8/8/8/R4RK1 b kq - 1 1"},
             {kOpen, Move(kE1, kC1, kCastling), "r3k2r/8/8/8/8/8/8/2KR3R b kq - 1 1"},
             {kOpenBlack, Move(kE8, kG8, kCastling), "r4rk1/8/8/8/8/8/8/R3K2R w KQ - 1 2"},
             {kOpenBlack, Move(kE8, kC8, kCastling), "2kr3r/8/8/8/8/8/8/R3K2R w KQ - 1 2"},
             // Rights end when the king moves, a rook moves, or a rook is captured at home.
             {kOpen, Move(kE1, kE2), "r3k2r/8/8/8/8/8/4K3/R6R b kq - 1 1"},
             {kOpen, Move(kH1, kG1), "r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 1 1"},
             {kOpen, Move(kA1, kA8), "R3k2r/8/8/8/8/8/8/4K2R b Kk - 0 1"}}) {
        CAPTURE(c.fen, toUci(c.move));
        Position pos = parse(c.fen);
        Key before = pos.key();

        REQUIRE(pos.make(c.move));
        CHECK(pos.fen() == c.after);
        requireMatchesFreshParse(pos);

        pos.unmake(c.move);
        CHECK(pos.fen() == c.fen);
        CHECK(pos.key() == before);
        requireMatchesFreshParse(pos);
    }
}

TEST_CASE("make rejects moves that leave the king attacked", "[make]") {
    checkIllegal("4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1", Move(kE2, kC3)); // pinned knight
    checkIllegal("5r1k/8/8/8/8/8/8/4K3 w - - 0 1", Move(kE1, kF1));    // king into attack
    checkIllegal("4r1k1/8/8/8/8/8/P7/4K3 w - - 0 1", Move(kA2, kA3));  // ignores a check
    // Capturing en passant lifts both pawns off the rank at once, exposing the king to the rook.
    checkIllegal("8/8/8/K2pP2r/8/8/8/7k w - d6 0 2", Move(kE5, kD6, kEnPassant));
}

TEST_CASE("make applies the castling attack rules", "[make]") {
    const Move kingSide(kE1, kG1, kCastling), queenSide(kE1, kC1, kCastling);

    // Out of check, through attack, and into attack, on both wings.
    checkIllegal("4k3/8/8/8/8/8/4r3/R3K2R w KQ - 0 1", kingSide);
    checkIllegal("4k3/8/8/8/8/8/4r3/R3K2R w KQ - 0 1", queenSide);
    checkIllegal("4k3/8/8/8/5r2/8/8/R3K2R w KQ - 0 1", kingSide);
    checkIllegal("4k3/8/8/8/3r4/8/8/R3K2R w KQ - 0 1", queenSide);
    checkIllegal("4k3/8/8/8/6r1/8/8/R3K2R w KQ - 0 1", kingSide);
    checkIllegal("4k3/8/8/8/2r5/8/8/R3K2R w KQ - 0 1", queenSide);
    checkIllegal("r3k2r/8/8/5R2/8/8/8/4K3 b kq - 0 1", Move(kE8, kG8, kCastling));

    // Attacks on the rook or on b1 do not matter, since the king never stands there.
    checkLegal("4k3/8/8/8/5r2/8/8/R3K2R w KQ - 0 1", queenSide);
    checkLegal("4k3/8/8/8/1r6/8/8/R3K2R w KQ - 0 1", queenSide);
    checkLegal("4k2r/8/8/8/8/8/8/R3K2R w KQ - 0 1", kingSide);
    checkLegal("r3k2r/8/8/5R2/8/8/8/4K3 b kq - 0 1", Move(kE8, kC8, kCastling));
}

TEST_CASE("key after make hashes en passant only when capturable", "[make]") {
    Position start = parse(Position::kStartFen);
    REQUIRE(start.make(Move(kE2, kE4)));
    CHECK(start.key() == parse("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1").key());

    Position pawnOnD4 = parse("4k3/8/8/8/3p4/8/4P3/4K3 w - - 0 1");
    REQUIRE(pawnOnD4.make(Move(kE2, kE4)));
    CHECK(pawnOnD4.key() != parse("4k3/8/8/8/3pP3/8/8/4K3 b - - 0 1").key());
    CHECK(pawnOnD4.key() == parse("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1").key());
}

TEST_CASE("returning to a position restores its key, not its clocks", "[make]") {
    Position pos = parse(Position::kStartFen);
    Key start = pos.key();
    for (Move m : {Move(kG1, kF3), Move(kG8, kF6), Move(kF3, kG1), Move(kF6, kG8)})
        REQUIRE(pos.make(m));
    CHECK(pos.key() == start);
    CHECK(pos.fen() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 4 3");
}

TEST_CASE("random games keep all state consistent and unwind to the start", "[make]") {
    Prng rng(7);
    for (std::string_view fen :
         {Position::kStartFen, kKiwipete, kPosition3, kPosition4, kPosition5, kPosition6}) {
        CAPTURE(fen);
        Position pos = parse(fen);
        Key rootKey = pos.key();
        std::vector<Move> played;

        for (int ply = 0; ply < 200; ++ply) {
            std::string before = pos.fen();
            Key beforeKey = pos.key();
            std::vector<Move> legal;
            for (Move m : generateMoves(pos)) {
                CAPTURE(before, toUci(m));
                if (pos.make(m)) {
                    requireMatchesFreshParse(pos);
                    pos.unmake(m);
                    legal.push_back(m);
                }
                REQUIRE(pos.fen() == before);
                REQUIRE(pos.key() == beforeKey);
            }
            if (legal.empty()) break;
            Move m = legal[rng.next() % legal.size()];
            REQUIRE(pos.make(m));
            played.push_back(m);
        }

        for (auto it = played.rbegin(); it != played.rend(); ++it)
            pos.unmake(*it);
        CHECK(pos.fen() == fen);
        CHECK(pos.key() == rootKey);
    }
}
