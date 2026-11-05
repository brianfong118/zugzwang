#include "movegen.h"

#include "attacks.h"

#include <initializer_list>

namespace zugzwang {

namespace {

enum class GenType { kAll, kNoisy };

// delta is the signed distance every pawn in the set travelled, which recovers its origin.
template <GenType G> void addPawnTargets(Bitboard targets, int delta, MoveList& moves) {
    while (targets) {
        Square to = popLsb(targets);
        Square from = Square(to - delta);
        if (squareBb(to) & (kRank1 | kRank8)) {
            moves.add(Move(from, to, kPromoteQueen));
            if constexpr (G == GenType::kAll)
                for (MoveFlag flag : {kPromoteRook, kPromoteBishop, kPromoteKnight})
                    moves.add(Move(from, to, flag));
        } else {
            moves.add(Move(from, to));
        }
    }
}

template <Color Us, GenType G> void addPawnMoves(const Position& pos, MoveList& moves) {
    constexpr Direction kUp = Us == kWhite ? kNorth : kSouth;
    constexpr Direction kUpEast = Us == kWhite ? kNorthEast : kSouthEast;
    constexpr Direction kUpWest = Us == kWhite ? kNorthWest : kSouthWest;
    // A pawn that reaches this rank in one push started on its home rank.
    constexpr Bitboard kOnePushRank = rankBb(Us == kWhite ? 2 : 5);

    Bitboard pawns = pos.pieces(Us, kPawn);
    Bitboard empty = ~pos.occupied();
    Bitboard enemies = pos.pieces(~Us);

    Bitboard onePush = shift<kUp>(pawns) & empty;
    if constexpr (G == GenType::kAll) {
        addPawnTargets<G>(onePush, kUp, moves);
        addPawnTargets<G>(shift<kUp>(onePush & kOnePushRank) & empty, 2 * kUp, moves);
    } else {
        addPawnTargets<G>(onePush & (kRank1 | kRank8), kUp, moves);
    }
    addPawnTargets<G>(shift<kUpEast>(pawns) & enemies, kUpEast, moves);
    addPawnTargets<G>(shift<kUpWest>(pawns) & enemies, kUpWest, moves);

    Square ep = pos.epSquare();
    if (ep != kNoSquare) {
        // A pawn attacks ep exactly when an enemy pawn standing on ep would attack that pawn.
        for (Bitboard b = attacks::pawn(~Us, ep) & pawns; b;)
            moves.add(Move(popLsb(b), ep, kEnPassant));
    }
}

// One generator for both lists, so they cannot disagree on what a capture or promotion is.
template <GenType G> MoveList generate(const Position& pos) {
    MoveList moves;
    Color us = pos.sideToMove();
    Bitboard occupied = pos.occupied();
    Bitboard targets = G == GenType::kAll ? ~pos.pieces(us) : pos.pieces(~us);

    if (us == kWhite)
        addPawnMoves<kWhite, G>(pos, moves);
    else
        addPawnMoves<kBlack, G>(pos, moves);

    auto addPieceMoves = [&](PieceType pt, auto attacksFrom) {
        for (Bitboard pieces = pos.pieces(us, pt); pieces;) {
            Square from = popLsb(pieces);
            for (Bitboard b = attacksFrom(from) & targets; b;)
                moves.add(Move(from, popLsb(b)));
        }
    };
    addPieceMoves(kKnight, [](Square s) { return attacks::knight(s); });
    addPieceMoves(kBishop, [=](Square s) { return attacks::bishop(s, occupied); });
    addPieceMoves(kRook, [=](Square s) { return attacks::rook(s, occupied); });
    addPieceMoves(kQueen, [=](Square s) { return attacks::queen(s, occupied); });
    addPieceMoves(kKing, [](Square s) { return attacks::king(s); });

    if constexpr (G == GenType::kAll)
        for (const CastlingRule& rule : kCastlingRules[us])
            if ((pos.castlingRights() & rule.right) && !(occupied & rule.mustBeEmpty))
                moves.add(Move(rule.kingFrom, rule.kingTo, kCastling));

    return moves;
}

} // namespace

MoveList generateMoves(const Position& pos) {
    return generate<GenType::kAll>(pos);
}

MoveList generateNoisy(const Position& pos) {
    return generate<GenType::kNoisy>(pos);
}

} // namespace zugzwang
