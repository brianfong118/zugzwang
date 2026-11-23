#pragma once

#include "move.h"
#include "position.h"
#include "tt.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <stop_token>

namespace zugzwang {

using Clock = std::chrono::steady_clock;

inline constexpr int kMaxPly = 128;
// Mate in n plies scores kMate - n, so the search prefers the fastest mate and the slowest loss.
inline constexpr int kMate = 32000;
inline constexpr int kMateInMaxPly = kMate - kMaxPly;
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

// A search can finish within a microsecond, so the divisor is at least 1.
inline std::uint64_t nodesPerSecond(std::uint64_t nodes, Clock::duration elapsed) {
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
    return nodes * 1'000'000 / std::max<std::int64_t>(us, 1);
}

struct SearchLimits {
    int depth = kMaxPly;
    std::uint64_t nodes = std::numeric_limits<std::uint64_t>::max();
    // No iteration starts after softTime; hardTime stops the search in the middle of one.
    std::optional<std::chrono::milliseconds> softTime = std::nullopt;
    std::optional<std::chrono::milliseconds> hardTime = std::nullopt;
    // Null-move pruning can change the score, so the test against plain minimax turns it off.
    bool forwardPruning = true;
};

using SearchReport = std::function<void(const SearchInfo&)>;

struct TimeBudget {
    std::chrono::milliseconds soft;
    std::chrono::milliseconds hard;
};

// The table counts mate distance from the node, so an entry stays right when read at another ply.
int scoreToTt(int score, int ply);
int scoreFromTt(int score, int ply);

// History scores stay within this bound, so they rank below every killer and capture.
inline constexpr int kMaxHistory = 1 << 14;

// Scaled down near the bound it heads for, so with |bonus| <= kMaxHistory it never passes it.
int updatedHistory(int entry, int bonus);

// remaining/30 + increment spreads the clock over the game; remaining/10 caps both limits.
TimeBudget timeBudget(std::chrono::milliseconds remaining, std::chrono::milliseconds increment);

// Deepens until a limit or a stop request, calling report after each depth; pos ends unchanged.
SearchInfo search(Position& pos, const SearchLimits& limits, TranspositionTable& tt,
                  std::stop_token stop = {}, const SearchReport& report = {});

} // namespace zugzwang
