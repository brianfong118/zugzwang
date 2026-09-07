#pragma once

#include "bitboard.h"
#include "types.h"

#include <array>
#include <span>

namespace zugzwang::attacks {

struct Delta {
    int file, rank;
};

inline constexpr std::array<Delta, 8> kKnightDeltas{
    {{1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}, {-1, 2}}};
inline constexpr std::array<Delta, 8> kKingDeltas{
    {{0, 1}, {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}}};
inline constexpr std::array<Delta, 4> kRookDeltas{{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}};
inline constexpr std::array<Delta, 4> kBishopDeltas{{{1, 1}, {1, -1}, {-1, -1}, {-1, 1}}};
inline constexpr std::array<std::array<Delta, 2>, 2> kPawnDeltas{
    {{{{-1, 1}, {1, 1}}}, {{{-1, -1}, {1, -1}}}}};

// Steps from s along each delta: one square for leapers, until blocked for sliders.
constexpr Bitboard walk(Square s, std::span<const Delta> deltas, bool slide,
                        Bitboard occupied = 0) {
    Bitboard attacks = 0;
    for (auto [df, dr] : deltas) {
        for (int f = fileOf(s) + df, r = rankOf(s) + dr; f >= 0 && f < 8 && r >= 0 && r < 8;
             f += df, r += dr) {
            attacks |= squareBb(makeSquare(f, r));
            if (!slide || (occupied & squareBb(makeSquare(f, r)))) break;
        }
    }
    return attacks;
}

template <typename F> constexpr std::array<Bitboard, 64> makeTable(F f) {
    std::array<Bitboard, 64> table{};
    for (int s = 0; s < 64; ++s)
        table[s] = f(Square(s));
    return table;
}

inline constexpr auto kKnightAttacks =
    makeTable([](Square s) { return walk(s, kKnightDeltas, false); });
inline constexpr auto kKingAttacks =
    makeTable([](Square s) { return walk(s, kKingDeltas, false); });
inline constexpr std::array<std::array<Bitboard, 64>, 2> kPawnAttacks{
    makeTable([](Square s) { return walk(s, kPawnDeltas[kWhite], false); }),
    makeTable([](Square s) { return walk(s, kPawnDeltas[kBlack], false); })};

// Fancy magic bitboards: ((occupied & mask) * magic) >> shift is a perfect hash into attacks.
struct Magic {
    Bitboard mask;
    Bitboard magic;
    Bitboard* attacks;
    unsigned shift;

    unsigned index(Bitboard occupied) const {
        return unsigned(((occupied & mask) * magic) >> shift);
    }
};

extern std::array<Magic, 64> rookMagics;
extern std::array<Magic, 64> bishopMagics;

// Finds the magics and fills the slider tables; must run once before any slider lookup.
void init();

constexpr Bitboard pawn(Color c, Square s) {
    return kPawnAttacks[c][s];
}
constexpr Bitboard knight(Square s) {
    return kKnightAttacks[s];
}
constexpr Bitboard king(Square s) {
    return kKingAttacks[s];
}

inline Bitboard rook(Square s, Bitboard occupied) {
    const Magic& m = rookMagics[s];
    return m.attacks[m.index(occupied)];
}

inline Bitboard bishop(Square s, Bitboard occupied) {
    const Magic& m = bishopMagics[s];
    return m.attacks[m.index(occupied)];
}

inline Bitboard queen(Square s, Bitboard occupied) {
    return rook(s, occupied) | bishop(s, occupied);
}

} // namespace zugzwang::attacks
