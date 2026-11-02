#include "bench.h"

#include "search.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ostream>

namespace zugzwang {

namespace {

// About half a second in Release; raised as the search gets faster per depth.
constexpr int kBenchDepth = 5;

} // namespace

void bench(std::ostream& out) {
    using Clock = std::chrono::steady_clock;
    std::uint64_t nodes = 0;
    Clock::time_point start = Clock::now();
    for (std::string_view fen : kBenchFens) {
        Position pos = *Position::fromFen(fen);
        nodes += search(pos, {.depth = kBenchDepth}).nodes;
    }
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start).count();
    out << "nodes " << nodes << " nps " << nodes * 1'000'000 / std::max<std::int64_t>(us, 1)
        << '\n';
}

} // namespace zugzwang
