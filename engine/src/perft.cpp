#include "perft.h"

#include "movegen.h"

#include <ostream>

namespace zugzwang {

std::uint64_t perft(Position& pos, int depth) {
    if (depth == 0) return 1;

    std::uint64_t nodes = 0;
    for (Move m : generateMoves(pos)) {
        if (!pos.make(m)) continue;
        nodes += perft(pos, depth - 1);
        pos.unmake(m);
    }
    return nodes;
}

void divide(Position& pos, int depth, std::ostream& out) {
    std::uint64_t total = 0;
    for (Move m : generateMoves(pos)) {
        if (!pos.make(m)) continue;
        std::uint64_t nodes = perft(pos, depth - 1);
        pos.unmake(m);
        total += nodes;
        out << toUci(m) << ": " << nodes << '\n';
    }
    out << "\nNodes searched: " << total << '\n';
}

} // namespace zugzwang
