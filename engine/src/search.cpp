#include "search.h"

#include "eval.h"
#include "movegen.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <utility>

namespace zugzwang {

// Table entries keep scores in 16 bits and depths in 8.
static_assert(kMate <= INT16_MAX && kMaxPly <= UINT8_MAX);
// The history bonus is depth squared, and updatedHistory needs it within kMaxHistory.
static_assert(kMaxPly * kMaxPly <= kMaxHistory);

namespace {

using MoveScores = std::array<int, kMaxMoves>;
using Killers = std::array<Move, 2>;
// By side to move, from square and to square.
using History = std::array<std::array<std::array<int, 64>, 64>, 2>;

constexpr int kSecondKillerScore = kMaxHistory + 1;
constexpr int kFirstKillerScore = kMaxHistory + 2;
constexpr int kNoisyScore = kMaxHistory + 3;

// Neither a capture nor a queen promotion, so quiescence never searches it.
bool isQuiet(const Position& pos, Move m) {
    return pos.pieceOn(m.to()) == kNoPiece && m.flag() != kEnPassant && m.flag() != kPromoteQueen;
}

// MVV-LVA: the victim decides first, since 8 exceeds any attacker index.
int mvvLva(const Position& pos, Move m) {
    Piece victim = pos.pieceOn(m.to());
    int score = 0;
    if (victim != kNoPiece || m.flag() == kEnPassant) {
        PieceType type = victim == kNoPiece ? kPawn : typeOf(victim);
        score = 8 * (type + 1) - typeOf(pos.pieceOn(m.from()));
    }
    // Promoting gains about a queen, so it ranks with capturing one.
    if (m.flag() == kPromoteQueen) score += 8 * (kQueen + 1);
    return score;
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
    TranspositionTable& tt;
    std::stop_token stop;
    Clock::time_point start = Clock::now();
    std::uint64_t nodes = 0;
    // Depth 1 always completes, so a position with a legal move always gets one.
    bool canStop = false;
    bool stopped = false;
    // Newest first. Value-initialised to a1a1, which no generated move equals.
    std::array<Killers, kMaxPly> killers{};
    History history{};

    bool shouldStop();
    MoveScores scoreMoves(const Position& pos, const MoveList& moves, int ply) const;
    void recordQuietCutoff(const Position& pos, Move m, const MoveList& quietsTried, int depth,
                           int ply);
    int negamax(Position& pos, int depth, int ply, int alpha, int beta, PvLine& pv);
    int quiesce(Position& pos, int ply, int alpha, int beta);
};

// Reading the clock or the stop request costs far more than a compare, so both wait 1024 nodes.
bool Searcher::shouldStop() {
    if (canStop && !stopped)
        stopped =
            nodes >= limits.nodes ||
            (nodes % 1024 == 0 && (stop.stop_requested() ||
                                   (limits.hardTime && Clock::now() - start >= *limits.hardTime)));
    return stopped;
}

// A killer refuted a sibling of this node, which is better evidence than history's averages.
MoveScores Searcher::scoreMoves(const Position& pos, const MoveList& moves, int ply) const {
    MoveScores scores;
    int i = 0;
    for (Move m : moves) {
        int score;
        if (!isQuiet(pos, m))
            score = kNoisyScore + mvvLva(pos, m);
        else if (m == killers[ply][0])
            score = kFirstKillerScore;
        else if (m == killers[ply][1])
            score = kSecondKillerScore;
        else
            score = history[pos.sideToMove()][m.from()][m.to()];
        scores[i++] = score;
    }
    return scores;
}

// The quiet moves tried before m failed to cut off, so they lose what m gains.
void Searcher::recordQuietCutoff(const Position& pos, Move m, const MoveList& quietsTried,
                                 int depth, int ply) {
    Killers& k = killers[ply];
    if (k[0] != m) {
        k[1] = k[0];
        k[0] = m;
    }
    auto& table = history[pos.sideToMove()];
    // A cutoff with more depth left prunes a larger subtree.
    int bonus = depth * depth;
    table[m.from()][m.to()] = updatedHistory(table[m.from()][m.to()], bonus);
    for (Move q : quietsTried)
        table[q.from()][q.to()] = updatedHistory(table[q.from()][q.to()], -bonus);
}

// Fail-soft: the result may lie outside [alpha, beta], where it is a bound on the true score.
int Searcher::negamax(Position& pos, int depth, int ply, int alpha, int beta, PvLine& pv) {
    pv.length = 0;
    if (depth == 0) return quiesce(pos, ply, alpha, beta);
    ++nodes;
    if (shouldStop()) return 0;
    // The root must return a move even in a drawn position.
    if (ply > 0 && pos.isDraw(ply)) return 0;

    // Only a bound outside the window ends a node, so the root and every pv node are searched.
    std::optional<TranspositionTable::Entry> entry = ply > 0 ? tt.probe(pos.key()) : std::nullopt;
    if (entry && entry->depth >= depth) {
        int score = scoreFromTt(entry->score, ply);
        if ((entry->bound & kLowerBound && score >= beta) ||
            (entry->bound & kUpperBound && score <= alpha))
            return score;
    }

    int originalAlpha = alpha;
    PvLine childPv;
    int best = -kInfinity;
    MoveList moves = generateMoves(pos);
    MoveScores scores = scoreMoves(pos, moves, ply);
    MoveList quietsTried;
    for (int i = 0; i < moves.size(); ++i) {
        Move m = pickNext(moves, scores, i);
        bool quiet = isQuiet(pos, m);
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
                if (score >= beta) {
                    if (quiet) recordQuietCutoff(pos, m, quietsTried, depth, ply);
                    break;
                }
            }
        }
        if (quiet) quietsTried.add(m);
    }

    // No legal move: checkmate or stalemate.
    if (best == -kInfinity) best = pos.inCheck() ? -kMate + ply : 0;

    Bound bound = best >= beta ? kLowerBound : best > originalAlpha ? kExactBound : kUpperBound;
    tt.store(pos.key(), scoreToTt(best, ply), depth, bound);
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
    MoveScores scores = scoreMoves(pos, moves, ply);
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

int scoreToTt(int score, int ply) {
    if (score >= kMateInMaxPly) return score + ply;
    if (score <= -kMateInMaxPly) return score - ply;
    return score;
}

int scoreFromTt(int score, int ply) {
    if (score >= kMateInMaxPly) return score - ply;
    if (score <= -kMateInMaxPly) return score + ply;
    return score;
}

int updatedHistory(int entry, int bonus) {
    return entry + bonus - entry * std::abs(bonus) / kMaxHistory;
}

TimeBudget timeBudget(std::chrono::milliseconds remaining, std::chrono::milliseconds increment) {
    std::chrono::milliseconds hard = remaining / 10;
    // A depth started past half the budget would usually end beyond it.
    return {std::min(remaining / 30 + increment, hard) / 2, hard};
}

SearchInfo search(Position& pos, const SearchLimits& limits, TranspositionTable& tt,
                  std::stop_token stop, const SearchReport& report) {
    // A copied Position keeps no spare history, and make must not allocate mid-search.
    pos.reserveHistory(kMaxPly);
    Searcher searcher{limits, tt, stop};
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
        // The next depth usually costs more than all before it, so a late start would be cut off.
        if (limits.softTime && Clock::now() - searcher.start >= *limits.softTime) break;
    }
    return info;
}

} // namespace zugzwang
