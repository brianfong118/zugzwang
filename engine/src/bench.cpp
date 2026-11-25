#include "bench.h"

#include "search.h"

#include <chrono>
#include <cstdint>
#include <ostream>

namespace zugzwang {

namespace {

// Cheap enough to run on every change; raised as the search gets faster per depth.
constexpr int kBenchDepth = 11;

} // namespace

void bench(std::ostream& out) {
    // Its own table, so the count does not depend on what the engine searched before.
    TranspositionTable tt(kHashMegabytes);
    std::uint64_t nodes = 0;
    Clock::time_point start = Clock::now();
    for (std::string_view fen : kBenchFens) {
        Position pos = *Position::fromFen(fen);
        nodes += search(pos, {.depth = kBenchDepth}, tt).nodes;
    }
    out << "nodes " << nodes << " nps " << nodesPerSecond(nodes, Clock::now() - start) << '\n';
}

} // namespace zugzwang
