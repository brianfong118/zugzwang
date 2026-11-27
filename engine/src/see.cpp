#include "see.h"

#include "attacks.h"
#include "bitboard.h"

namespace zugzwang {

namespace {

int valueOn(const Position& pos, Square s) {
    Piece p = pos.pieceOn(s);
    return p == kNoPiece ? 0 : kSeeValues[typeOf(p)];
}

// Sliders look through occupied, so pieces already gone from the exchange uncover those behind.
Bitboard attackersTo(const Position& pos, Square s, Bitboard occupied) {
    Bitboard diagonal = pos.pieces(kBishop) | pos.pieces(kQueen);
    Bitboard straight = pos.pieces(kRook) | pos.pieces(kQueen);
    return (attacks::pawn(kBlack, s) & pos.pieces(kWhite, kPawn)) |
           (attacks::pawn(kWhite, s) & pos.pieces(kBlack, kPawn)) |
           (attacks::knight(s) & pos.pieces(kKnight)) | (attacks::king(s) & pos.pieces(kKing)) |
           (attacks::bishop(s, occupied) & diagonal) | (attacks::rook(s, occupied) & straight);
}

} // namespace

bool seeGe(const Position& pos, Move m, int threshold) {
    if (m.flag() != kNormalMove) return threshold <= 0;
    Square to = m.to();

    // Settled without the exchange if short even unanswered, or ahead even after losing the mover.
    int balance = valueOn(pos, to) - threshold;
    if (balance < 0) return false;
    balance -= valueOn(pos, m.from());
    if (balance >= 0) return true;

    Bitboard occupied = pos.occupied() ^ squareBb(m.from());
    Bitboard attackers = attackersTo(pos, to, occupied) & occupied;
    Color side = ~pos.sideToMove();
    // The side to recapture is behind if it stops, so it always recaptures with its cheapest piece.
    while (Bitboard ours = attackers & pos.pieces(side)) {
        PieceType pt = kPawn;
        while (!(ours & pos.pieces(side, pt)))
            pt = PieceType(pt + 1);
        occupied ^= squareBb(lsb(ours & pos.pieces(side, pt)));
        attackers = attackersTo(pos, to, occupied) & occupied;
        side = ~side;
        // The capturer's margin if it loses this piece next; the -1 gives a tie to the first mover.
        balance = -balance - 1 - kSeeValues[pt];
        if (balance >= 0) {
            // A king cannot capture onto a square the other side still attacks.
            if (pt == kKing && (attackers & pos.pieces(side))) side = ~side;
            break;
        }
    }
    // Whoever is left to recapture has lost the exchange.
    return side != pos.sideToMove();
}

} // namespace zugzwang
