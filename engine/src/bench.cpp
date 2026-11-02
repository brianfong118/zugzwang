#include "bench.h"

#include "search.h"

#include <chrono>
#include <cstdint>
#include <ostream>

namespace zugzwang {

namespace {

// About half a second in Release; raised as the search gets faster per depth.
constexpr int kBenchDepth = 5;

} // namespace

void bench(std::ostream& out) {
    std::uint64_t nodes = 0;
    Clock::time_point start = Clock::now();
    for (std::string_view fen : kBenchFens) {
        Position pos = *Position::fromFen(fen);
        nodes += search(pos, {.depth = kBenchDepth}).nodes;
    }
    out << "nodes " << nodes << " nps " << nodesPerSecond(nodes, Clock::now() - start) << '\n';
}

} // namespace zugzwang
