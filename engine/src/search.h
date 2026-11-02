#pragma once

#include "move.h"
#include "position.h"

#include <array>
#include <cstdint>

namespace zugzwang {

inline constexpr int kMaxPly = 128;
// Mate in n plies scores kMate - n, so the search prefers the fastest mate and the slowest loss.
inline constexpr int kMate = 32000;
inline constexpr int kInfinity = kMate + 1;

struct PvLine {
    std::array<Move, kMaxPly> moves;
    int length = 0;
};

// One completed iteration, scored for the side to move. The pv is empty if it has no legal move.
struct SearchInfo {
    int depth = 0;
    int score = 0;
    std::uint64_t nodes = 0;
    PvLine pv;
};

// Iterative deepening from depth 1 to depth, capped at kMaxPly; pos is unchanged on return.
SearchInfo search(Position& pos, int depth);

} // namespace zugzwang
