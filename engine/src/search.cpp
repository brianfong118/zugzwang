#include "search.h"

#include "eval.h"
#include "movegen.h"

#include <algorithm>
#include <array>
#include <utility>

namespace zugzwang {

namespace {

using MoveScores = std::array<int, kMaxMoves>;

// MVV-LVA: the victim decides first, since 8 exceeds any attacker index. Quiet moves score 0.
MoveScores scoreMoves(const Position& pos, const MoveList& moves) {
    MoveScores scores;
    int i = 0;
    for (Move m : moves) {
        Piece victim = pos.pieceOn(m.to());
        int score = 0;
        if (victim != kNoPiece || m.flag() == kEnPassant) {
            PieceType type = victim == kNoPiece ? kPawn : typeOf(victim);
            score = 8 * (type + 1) - typeOf(pos.pieceOn(m.from()));
        }
        // Promoting gains about a queen, so it ranks with capturing one.
        if (m.flag() == kPromoteQueen) score += 8 * (kQueen + 1);
        scores[i++] = score;
    }
    return scores;
}

// Selection, not a full sort: after an early cutoff the remaining moves are never ordered.
Move pickNext(MoveList& moves, MoveScores& scores, int i) {
    int best = i;
    for (int j = i + 1; j < moves.size(); ++j)
        if (scores[j] > scores[best]) best = j;
    std::swap(moves[i], moves[best]);
    std::swap(scores[i], scores[best]);
    return moves[i];
}

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
    int quiesce(Position& pos, int ply, int alpha, int beta);
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
    pv.length = 0;
    if (depth == 0) return quiesce(pos, ply, alpha, beta);
    ++nodes;
    if (shouldStop()) return 0;
    // The root must return a move even in a drawn position.
    if (ply > 0 && pos.isDraw(ply)) return 0;

    PvLine childPv;
    int best = -kInfinity;
    MoveList moves = generateMoves(pos);
    MoveScores scores = scoreMoves(pos, moves);
    for (int i = 0; i < moves.size(); ++i) {
        Move m = pickNext(moves, scores, i);
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

// Plays out captures until the position is quiet, so no leaf is scored mid-exchange.
int Searcher::quiesce(Position& pos, int ply, int alpha, int beta) {
    ++nodes;
    if (shouldStop()) return 0;
    if (pos.isDraw(ply)) return 0;

    // Standing pat: the side to move may decline every capture, so the eval is a lower bound.
    int best = evaluate(pos);
    if (best >= beta || ply >= kMaxPly) return best;
    alpha = std::max(alpha, best);

    MoveList moves = generateNoisy(pos);
    MoveScores scores = scoreMoves(pos, moves);
    for (int i = 0; i < moves.size(); ++i) {
        Move m = pickNext(moves, scores, i);
        if (!pos.make(m)) continue;
        int score = -quiesce(pos, ply + 1, -beta, -alpha);
        pos.unmake(m);
        if (stopped) return 0;

        if (score > best) {
            best = score;
            if (score > alpha) {
                alpha = score;
                if (score >= beta) break;
            }
        }
    }
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
