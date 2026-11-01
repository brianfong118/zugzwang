#pragma once

#include "types.h"

#include <bit>
#include <cstdint>

namespace zugzwang {

using Bitboard = std::uint64_t;

constexpr Bitboard kFileA = 0x0101010101010101ULL;
constexpr Bitboard kFileH = kFileA << 7;
constexpr Bitboard kRank1 = 0xFF;
constexpr Bitboard kRank8 = kRank1 << 56;

constexpr Bitboard fileBb(int file) {
    return kFileA << file;
}
constexpr Bitboard rankBb(int rank) {
    return kRank1 << (8 * rank);
}
constexpr Bitboard squareBb(Square s) {
    return 1ULL << s;
}

constexpr Square lsb(Bitboard b) {
    return Square(std::countr_zero(b));
}

inline Square popLsb(Bitboard& b) {
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

enum Direction : int {
    kNorth = 8,
    kSouth = -8,
    kEast = 1,
    kWest = -1,
    kNorthEast = 9,
    kNorthWest = 7,
    kSouthEast = -7,
    kSouthWest = -9
};

// Masks the edge file first so pieces never wrap from the h-file onto the a-file.
template <Direction D> constexpr Bitboard shift(Bitboard b) {
    if constexpr (D == kNorth)
        return b << 8;
    else if constexpr (D == kSouth)
        return b >> 8;
    else if constexpr (D == kEast)
        return (b & ~kFileH) << 1;
    else if constexpr (D == kWest)
        return (b & ~kFileA) >> 1;
    else if constexpr (D == kNorthEast)
        return (b & ~kFileH) << 9;
    else if constexpr (D == kNorthWest)
        return (b & ~kFileA) << 7;
    else if constexpr (D == kSouthEast)
        return (b & ~kFileH) >> 7;
    else
        return (b & ~kFileA) >> 9;
}

} // namespace zugzwang
