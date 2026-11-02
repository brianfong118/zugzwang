#include "search.h"

#include "eval.h"
#include "movegen.h"

#include <algorithm>

namespace zugzwang {

namespace {

using Clock = std::chrono::steady_clock;

struct Searcher {
    const SearchLimits& limits;
    std::stop_token stop;
    Clock::time_point start = Clock::now();
    std::uint64_t nodes = 0;
    // Depth 1 always completes, so a position with a legal move always gets one.
    bool canStop = false;
    bool stopped = false;

    bool shouldStop();
    int negamax(Position& pos, int depth, int ply, int alpha, int beta, PvLine& pv);
};

// Reading the clock or the stop request costs far more than a compare, so both wait 1024 nodes.
bool Searcher::shouldStop() {
    if (canStop && !stopped)
        stopped = nodes >= limits.nodes ||
                  (nodes % 1024 == 0 && (stop.stop_requested() ||
                                         (limits.time && Clock::now() - start >= *limits.time)));
    return stopped;
}

// Fail-soft: the result may lie outside [alpha, beta], where it is a bound on the true score.
int Searcher::negamax(Position& pos, int depth, int ply, int alpha, int beta, PvLine& pv) {
    ++nodes;
    pv.length = 0;
    if (shouldStop()) return 0;
    // The root must return a move even in a drawn position.
    if (ply > 0 && pos.isDraw(ply)) return 0;
    if (depth == 0) return evaluate(pos);

    PvLine childPv;
    int best = -kInfinity;
    for (Move m : generateMoves(pos)) {
        if (!pos.make(m)) continue;
        int score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, childPv);
        pos.unmake(m);
        if (stopped) return 0;

        if (score > best) {
            best = score;
            if (score > alpha) {
                alpha = score;
                pv.moves[0] = m;
                std::copy_n(childPv.moves.begin(), childPv.length, pv.moves.begin() + 1);
                pv.length = childPv.length + 1;
                if (score >= beta) break;
            }
        }
    }

    // No legal move: checkmate or stalemate.
    if (best == -kInfinity) return pos.inCheck() ? -kMate + ply : 0;
    return best;
}

} // namespace

std::chrono::milliseconds timeBudget(std::chrono::milliseconds remaining,
                                     std::chrono::milliseconds increment) {
    return std::min(remaining / 30 + increment, remaining / 10);
}

SearchInfo search(Position& pos, const SearchLimits& limits, std::stop_token stop,
                  const SearchReport& report) {
    // A copied Position keeps no spare history, and make must not allocate mid-search.
    pos.reserveHistory(kMaxPly);
    Searcher searcher{limits, stop};
    SearchInfo info;
    for (int d = 1; d <= std::min(limits.depth, kMaxPly); ++d) {
        searcher.canStop = d > 1;
        PvLine pv;
        int score = searcher.negamax(pos, d, 0, -kInfinity, kInfinity, pv);
        if (searcher.stopped) break;

        info = {d, score, searcher.nodes, pv};
        if (report) report(info);
        // With no legal move at the root, deeper searches cannot change the result.
        if (pv.length == 0) break;
    }
    return info;
}

} // namespace zugzwang
