#pragma once

#include <cstdint>
#include <string>

namespace zugzwang {

using Key = std::uint64_t;

enum Color : std::uint8_t { kWhite, kBlack };

enum PieceType : std::uint8_t { kPawn, kKnight, kBishop, kRook, kQueen, kKing };

// Piece = 6 * color + type, so the twelve pieces index Zobrist and bitboard tables directly.
enum Piece : std::uint8_t {
    kWhitePawn,
    kWhiteKnight,
    kWhiteBishop,
    kWhiteRook,
    kWhiteQueen,
    kWhiteKing,
    kBlackPawn,
    kBlackKnight,
    kBlackBishop,
    kBlackRook,
    kBlackQueen,
    kBlackKing,
    kNoPiece
};

// Little-endian rank-file: a1 = 0, h1 = 7, a8 = 56, h8 = 63.
// clang-format off
enum Square : std::uint8_t {
    kA1, kB1, kC1, kD1, kE1, kF1, kG1, kH1,
    kA2, kB2, kC2, kD2, kE2, kF2, kG2, kH2,
    kA3, kB3, kC3, kD3, kE3, kF3, kG3, kH3,
    kA4, kB4, kC4, kD4, kE4, kF4, kG4, kH4,
    kA5, kB5, kC5, kD5, kE5, kF5, kG5, kH5,
    kA6, kB6, kC6, kD6, kE6, kF6, kG6, kH6,
    kA7, kB7, kC7, kD7, kE7, kF7, kG7, kH7,
    kA8, kB8, kC8, kD8, kE8, kF8, kG8, kH8,
    kNoSquare
};
// clang-format on

enum CastlingRights : std::uint8_t {
    kNoCastling = 0,
    kWhiteKingSide = 1,
    kWhiteQueenSide = 2,
    kBlackKingSide = 4,
    kBlackQueenSide = 8,
    kAnyCastling = 15
};

constexpr Color operator~(Color c) {
    return Color(c ^ 1);
}

constexpr Piece makePiece(Color c, PieceType pt) {
    return Piece(6 * c + pt);
}
constexpr Color colorOf(Piece p) {
    return Color(p / 6);
}
constexpr PieceType typeOf(Piece p) {
    return PieceType(p % 6);
}

constexpr Square makeSquare(int file, int rank) {
    return Square(8 * rank + file);
}
constexpr int fileOf(Square s) {
    return s & 7;
}
constexpr int rankOf(Square s) {
    return s >> 3;
}

inline std::string squareName(Square s) {
    return {char('a' + fileOf(s)), char('1' + rankOf(s))};
}

} // namespace zugzwang
