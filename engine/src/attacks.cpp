#include "attacks.h"

#include "prng.h"

#include <bit>

namespace zugzwang::attacks {

std::array<Magic, 64> rookMagics;
std::array<Magic, 64> bishopMagics;

namespace {

std::array<Bitboard, 0x19000> rookTable;
std::array<Bitboard, 0x1480> bishopTable;

void initMagics(std::span<const Delta> deltas, std::array<Magic, 64>& magics, Bitboard* table) {
    Prng rng(0x2545F4914F6CDD1DULL);
    std::array<Bitboard, 4096> occupancy, reference;
    std::array<int, 4096> epoch{};
    int attempt = 0;

    for (int sq = 0; sq < 64; ++sq) {
        Square s = Square(sq);
        // An edge square blocks nothing beyond it, so leaving edges out shrinks the table.
        Bitboard edges =
            ((kRank1 | kRank8) & ~rankBb(rankOf(s))) | ((kFileA | kFileH) & ~fileBb(fileOf(s)));
        Magic& m = magics[sq];
        m.mask = walk(s, deltas, true) & ~edges;
        m.shift = 64 - std::popcount(m.mask);
        m.attacks = table;

        // Carry-rippler: occ - mask borrows through the mask bits, enumerating every subset.
        int n = 0;
        Bitboard occ = 0;
        do {
            occupancy[n] = occ;
            reference[n++] = walk(s, deltas, true, occ);
            occ = (occ - m.mask) & m.mask;
        } while (occ);
        table += n;

        // Slots may collide only on equal attacks; epoch[] spares clearing after a failed attempt.
        for (int i = 0; i < n;) {
            for (m.magic = 0; std::popcount((m.magic * m.mask) >> 56) < 6;)
                m.magic = rng.sparse();
            ++attempt;
            for (i = 0; i < n; ++i) {
                unsigned idx = m.index(occupancy[i]);
                if (epoch[idx] < attempt) {
                    epoch[idx] = attempt;
                    m.attacks[idx] = reference[i];
                } else if (m.attacks[idx] != reference[i]) {
                    break;
                }
            }
        }
    }
}

} // namespace

void init() {
    initMagics(kRookDeltas, rookMagics, rookTable.data());
    initMagics(kBishopDeltas, bishopMagics, bishopTable.data());
}

} // namespace zugzwang::attacks
