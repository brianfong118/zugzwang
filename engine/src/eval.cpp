#include "eval.h"

#include <array>
#include <bit>
#include <initializer_list>

namespace zugzwang {

namespace {

// The Chess Programming Wiki's Simplified Evaluation Function values. Kings are never captured.
constexpr std::array<int, 5> kPieceValues{100, 320, 330, 500, 900};

} // namespace

int evaluate(const Position& pos) {
    int score = 0;
    for (PieceType pt : {kPawn, kKnight, kBishop, kRook, kQueen})
        score += kPieceValues[pt] *
                 (std::popcount(pos.pieces(kWhite, pt)) - std::popcount(pos.pieces(kBlack, pt)));
    return pos.sideToMove() == kWhite ? score : -score;
}

} // namespace zugzwang
