#pragma once

#include "types.h"

#include <array>
#include <cstdint>
#include <string>

namespace zugzwang {

enum MoveFlag : std::uint8_t {
    kNormalMove,
    kCastling,
    kEnPassant,
    kPromoteKnight,
    kPromoteBishop,
    kPromoteRook,
    kPromoteQueen
};

// Packed as from (6 bits), to (6 bits), flag (4 bits).
class Move {
  public:
    // Left uninitialised so that declaring a MoveList does not write its whole array on every node.
    Move() = default;
    constexpr Move(Square from, Square to, MoveFlag flag = kNormalMove)
        : bits_(std::uint16_t(from | (to << 6) | (flag << 12))) {}

    constexpr Square from() const { return Square(bits_ & 63); }
    constexpr Square to() const { return Square((bits_ >> 6) & 63); }
    constexpr MoveFlag flag() const { return MoveFlag(bits_ >> 12); }
    constexpr bool isPromotion() const { return flag() >= kPromoteKnight; }
    constexpr PieceType promotionType() const {
        return PieceType(flag() - kPromoteKnight + kKnight);
    }
    constexpr bool operator==(const Move&) const = default;

  private:
    std::uint16_t bits_;
};

inline std::string toUci(Move m) {
    std::string s = squareName(m.from()) + squareName(m.to());
    if (m.isPromotion()) s += "nbrq"[m.promotionType() - kKnight];
    return s;
}

// fromFen caps a side at 16 pieces: at worst 15 queens of 27 moves, a king's 8 and 2 castles.
inline constexpr int kMaxMoves = 15 * 27 + 8 + 2;

class MoveList {
  public:
    void add(Move m) { moves_[size_++] = m; }
    Move& operator[](int i) { return moves_[i]; }
    const Move* begin() const { return moves_.data(); }
    const Move* end() const { return moves_.data() + size_; }
    int size() const { return size_; }

  private:
    std::array<Move, kMaxMoves> moves_;
    int size_ = 0;
};

} // namespace zugzwang
