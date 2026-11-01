#pragma once

#include "attacks.h"
#include "bitboard.h"
#include "move.h"
#include "types.h"
#include "zobrist.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace zugzwang {

class Position {
  public:
    static constexpr std::string_view kStartFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Position() {
        board_.fill(kNoPiece);
        history_.reserve(kHistoryReserve);
    }

    // Empty on a FEN that is malformed or structurally illegal (see DESIGN.md for what is checked).
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
    int fullmoveNumber() const { return fullmoveNumber_; }
    Key key() const { return key_; }

    // Attackers of both colours; pass a custom occupancy to see through a moving piece.
    Bitboard attackersTo(Square s, Bitboard occupied) const;
    Bitboard attackersTo(Square s) const { return attackersTo(s, occupied()); }
    bool isAttacked(Square s, Color by) const { return attackersTo(s) & pieces(by); }
    bool inCheck() const { return isAttacked(kingSquare(sideToMove_), ~sideToMove_); }

    // m must come from generateMoves. If it is illegal, returns false with the position unchanged.
    bool make(Move m);
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

    void putPiece(Piece p, Square s) {
        byPiece_[p] |= squareBb(s);
        byColor_[colorOf(p)] |= squareBb(s);
        board_[s] = p;
        key_ ^= zobrist::piece(p, s);
    }
    void removePiece(Square s) {
        Piece p = board_[s];
        byPiece_[p] &= ~squareBb(s);
        byColor_[colorOf(p)] &= ~squareBb(s);
        board_[s] = kNoPiece;
        key_ ^= zobrist::piece(p, s);
    }
    Key epKey() const {
        bool capturable = epSquare_ != kNoSquare &&
                          (attacks::pawn(~sideToMove_, epSquare_) & pieces(sideToMove_, kPawn));
        return capturable ? zobrist::enPassant(fileOf(epSquare_)) : 0;
    }

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
