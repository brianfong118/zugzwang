#include "attacks.h"
#include "bitboard.h"
#include "prng.h"

#include <catch2/catch_test_macros.hpp>

using namespace zugzwang;

namespace {

// Independent reference built from bitboard shifts rather than coordinate stepping.
template <Direction D> Bitboard ray(Square s, Bitboard occupied) {
    Bitboard attacks = 0, b = squareBb(s);
    while ((b = shift<D>(b))) {
        attacks |= b;
        if (b & occupied) break;
    }
    return attacks;
}

Bitboard slowRook(Square s, Bitboard occ) {
    return ray<kNorth>(s, occ) | ray<kSouth>(s, occ) | ray<kEast>(s, occ) | ray<kWest>(s, occ);
}

Bitboard slowBishop(Square s, Bitboard occ) {
    return ray<kNorthEast>(s, occ) | ray<kNorthWest>(s, occ) | ray<kSouthEast>(s, occ) |
           ray<kSouthWest>(s, occ);
}

} // namespace

TEST_CASE("leaper tables are built at compile time", "[attacks]") {
    STATIC_REQUIRE(attacks::knight(kA1) == (squareBb(kB3) | squareBb(kC2)));
    STATIC_REQUIRE(std::popcount(attacks::knight(kD4)) == 8);
    STATIC_REQUIRE(std::popcount(attacks::king(kE1)) == 5);
    STATIC_REQUIRE(std::popcount(attacks::king(kH8)) == 3);
    STATIC_REQUIRE(attacks::pawn(kWhite, kA2) == squareBb(kB3));
    STATIC_REQUIRE(attacks::pawn(kBlack, kH7) == squareBb(kG6));
    STATIC_REQUIRE(attacks::pawn(kWhite, kE8) == 0);
}

TEST_CASE("magic slider attacks match ray walking", "[attacks]") {
    Prng rng(42);
    for (int sq = 0; sq < 64; ++sq) {
        Square s = Square(sq);
        for (int i = 0; i < 200; ++i) {
            Bitboard occ = i == 0 ? 0 : i == 1 ? ~0ULL : rng.next() & rng.next();
            CAPTURE(sq, occ);
            REQUIRE(attacks::rook(s, occ) == slowRook(s, occ));
            REQUIRE(attacks::bishop(s, occ) == slowBishop(s, occ));
            REQUIRE(attacks::queen(s, occ) == (slowRook(s, occ) | slowBishop(s, occ)));
        }
    }
}
