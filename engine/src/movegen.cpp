#include "movegen.h"

#include <array>
#include <initializer_list>

namespace zugzwang {

namespace {

struct CastlingOption {
    CastlingRights right;
    Square from, to;
    Bitboard mustBeEmpty;
};

// The king never crosses b1 or b8, but the rook does, so those squares must be empty too.
constexpr std::array<std::array<CastlingOption, 2>, 2> kCastlingOptions{
    {{{{kWhiteKingSide, kE1, kG1, squareBb(kF1) | squareBb(kG1)},
       {kWhiteQueenSide, kE1, kC1, squareBb(kB1) | squareBb(kC1) | squareBb(kD1)}}},
     {{{kBlackKingSide, kE8, kG8, squareBb(kF8) | squareBb(kG8)},
       {kBlackQueenSide, kE8, kC8, squareBb(kB8) | squareBb(kC8) | squareBb(kD8)}}}}};

// delta is the signed distance every pawn in the set travelled, which recovers its origin.
void addPawnTargets(Bitboard targets, int delta, MoveList& moves) {
    while (targets) {
        Square to = popLsb(targets);
        Square from = Square(to - delta);
        if (squareBb(to) & (kRank1 | kRank8)) {
            for (MoveFlag flag : {kPromoteQueen, kPromoteRook, kPromoteBishop, kPromoteKnight})
                moves.add(Move(from, to, flag));
        } else {
            moves.add(Move(from, to));
        }
    }
}

template <Color Us> void addPawnMoves(const Position& pos, MoveList& moves) {
    constexpr Direction kUp = Us == kWhite ? kNorth : kSouth;
    constexpr Direction kUpEast = Us == kWhite ? kNorthEast : kSouthEast;
    constexpr Direction kUpWest = Us == kWhite ? kNorthWest : kSouthWest;
    // A pawn that reaches this rank in one push started on its home rank.
    constexpr Bitboard kOnePushRank = rankBb(Us == kWhite ? 2 : 5);

    Bitboard pawns = pos.pieces(Us, kPawn);
    Bitboard empty = ~pos.occupied();
    Bitboard enemies = pos.pieces(~Us);

    Bitboard onePush = shift<kUp>(pawns) & empty;
    addPawnTargets(onePush, kUp, moves);
    addPawnTargets(shift<kUp>(onePush & kOnePushRank) & empty, 2 * kUp, moves);
    addPawnTargets(shift<kUpEast>(pawns) & enemies, kUpEast, moves);
    addPawnTargets(shift<kUpWest>(pawns) & enemies, kUpWest, moves);

    Square ep = pos.epSquare();
    if (ep != kNoSquare) {
        // A pawn attacks ep exactly when an enemy pawn standing on ep would attack that pawn.
        for (Bitboard b = attacks::pawn(~Us, ep) & pawns; b;)
            moves.add(Move(popLsb(b), ep, kEnPassant));
    }
}

template <typename AttacksFrom>
void addPieceMoves(Bitboard pieces, Bitboard targets, AttacksFrom attacksFrom, MoveList& moves) {
    while (pieces) {
        Square from = popLsb(pieces);
        for (Bitboard b = attacksFrom(from) & targets; b;)
            moves.add(Move(from, popLsb(b)));
    }
}

} // namespace

MoveList generateMoves(const Position& pos) {
    MoveList moves;
    Color us = pos.sideToMove();
    Bitboard occupied = pos.occupied();
    Bitboard targets = ~pos.pieces(us);

    if (us == kWhite)
        addPawnMoves<kWhite>(pos, moves);
    else
        addPawnMoves<kBlack>(pos, moves);

    addPieceMoves(
        pos.pieces(us, kKnight), targets, [](Square s) { return attacks::knight(s); }, moves);
    addPieceMoves(
        pos.pieces(us, kBishop), targets, [=](Square s) { return attacks::bishop(s, occupied); },
        moves);
    addPieceMoves(
        pos.pieces(us, kRook), targets, [=](Square s) { return attacks::rook(s, occupied); },
        moves);
    addPieceMoves(
        pos.pieces(us, kQueen), targets, [=](Square s) { return attacks::queen(s, occupied); },
        moves);
    addPieceMoves(pos.pieces(us, kKing), targets, [](Square s) { return attacks::king(s); }, moves);

    for (const CastlingOption& c : kCastlingOptions[us])
        if ((pos.castlingRights() & c.right) && !(occupied & c.mustBeEmpty))
            moves.add(Move(c.from, c.to, kCastling));

    return moves;
}

} // namespace zugzwang
