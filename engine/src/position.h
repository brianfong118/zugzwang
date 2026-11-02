#pragma once

#include "bitboard.h"
#include "move.h"
#include "types.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace zugzwang {

struct CastlingRule {
    CastlingRights right;
    Square kingFrom, kingTo, rookFrom, rookTo;
    Bitboard mustBeEmpty;
};

// By colour. The king never crosses b1 or b8, but the rook does, so those must be empty too.
inline constexpr std::array<std::array<CastlingRule, 2>, 2> kCastlingRules{
    {{{{kWhiteKingSide, kE1, kG1, kH1, kF1, squareBb(kF1) | squareBb(kG1)},
       {kWhiteQueenSide, kE1, kC1, kA1, kD1, squareBb(kB1) | squareBb(kC1) | squareBb(kD1)}}},
     {{{kBlackKingSide, kE8, kG8, kH8, kF8, squareBb(kF8) | squareBb(kG8)},
       {kBlackQueenSide, kE8, kC8, kA8, kD8, squareBb(kB8) | squareBb(kC8) | squareBb(kD8)}}}}};

class Position {
  public:
    static constexpr std::string_view kStartFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    // The only way to create a Position; empty if the FEN is malformed or illegal (see DESIGN.md).
    static std::optional<Position> fromFen(std::string_view fen);
    std::string fen() const;

    Bitboard pieces(Piece p) const { return byPiece_[p]; }
    Bitboard pieces(Color c, PieceType pt) const { return byPiece_[makePiece(c, pt)]; }
    Bitboard pieces(PieceType pt) const { return byPiece_[pt] | byPiece_[pt + 6]; }
    Bitboard pieces(Color c) const { return byColor_[c]; }
    Bitboard occupied() const { return byColor_[kWhite] | byColor_[kBlack]; }
    Piece pieceOn(Square s) const { return board_[s]; }
    Square kingSquare(Color c) const { return lsb(pieces(c, kKing)); }

    Color sideToMove() const { return sideToMove_; }
    int castlingRights() const { return castling_; }
    Square epSquare() const { return epSquare_; }
    int halfmoveClock() const { return halfmoveClock_; }
    Key key() const { return key_; }

    bool isAttacked(Square s, Color by) const;
    bool inCheck() const { return isAttacked(kingSquare(sideToMove_), ~sideToMove_); }

    // Fifty-move rule or repetition; ply is the distance from the search root (see DESIGN.md).
    bool isDraw(int ply) const;

    // m must come from generateMoves. If it is illegal, returns false with the position unchanged.
    [[nodiscard]] bool make(Move m);
    void unmake(Move m);

    // Full recompute, for checking the incrementally maintained key.
    Key computeKey() const;

  private:
    // What a move destroys and unmake cannot work out from the board afterwards.
    struct Undo {
        Key key;
        int halfmoveClock;
        Square epSquare;
        std::uint8_t castling;
        Piece captured;
    };

    // Enough that make does not reallocate in any realistic game plus search.
    static constexpr std::size_t kHistoryReserve = 1024;

    Position() {
        board_.fill(kNoPiece);
        history_.reserve(kHistoryReserve);
    }

    void putPiece(Piece p, Square s);
    void removePiece(Square s);
    Key epKey() const;

    std::vector<Undo> history_;
    std::array<Bitboard, 12> byPiece_{};
    std::array<Bitboard, 2> byColor_{};
    std::array<Piece, 64> board_;
    Color sideToMove_ = kWhite;
    std::uint8_t castling_ = kNoCastling;
    Square epSquare_ = kNoSquare;
    int halfmoveClock_ = 0;
    int fullmoveNumber_ = 1;
    Key key_ = 0;
};

} // namespace zugzwang
