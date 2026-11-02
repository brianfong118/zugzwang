#include "search.h"

#include "eval.h"
#include "movegen.h"

#include <algorithm>

namespace zugzwang {

namespace {

struct Searcher {
    std::uint64_t nodes = 0;

    int negamax(Position& pos, int depth, int ply, int alpha, int beta, PvLine& pv);
};

// Fail-soft: the result may lie outside [alpha, beta], where it is a bound on the true score.
int Searcher::negamax(Position& pos, int depth, int ply, int alpha, int beta, PvLine& pv) {
    ++nodes;
    pv.length = 0;
    // The root must return a move even in a drawn position.
    if (ply > 0 && pos.isDraw(ply)) return 0;
    if (depth == 0) return evaluate(pos);

    PvLine childPv;
    int best = -kInfinity;
    for (Move m : generateMoves(pos)) {
        if (!pos.make(m)) continue;
        int score = -negamax(pos, depth - 1, ply + 1, -beta, -alpha, childPv);
        pos.unmake(m);

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

SearchInfo search(Position& pos, int depth) {
    Searcher searcher;
    SearchInfo info;
    for (int d = 1; d <= std::min(depth, kMaxPly); ++d) {
        info.score = searcher.negamax(pos, d, 0, -kInfinity, kInfinity, info.pv);
        info.depth = d;
        info.nodes = searcher.nodes;
    }
    return info;
}

} // namespace zugzwang
