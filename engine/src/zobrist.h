#pragma once

#include "prng.h"
#include "types.h"

#include <array>

namespace zugzwang::zobrist {

struct Keys {
    std::array<std::array<Key, 64>, 12> pieceSquare;
    std::array<Key, 16> castling;
    std::array<Key, 8> enPassant;
    Key side;
};

constexpr Keys makeKeys() {
    Prng rng(0x5A0B21571A8D3E7FULL);
    Keys keys{};
    for (auto& squares : keys.pieceSquare)
        for (Key& k : squares)
            k = rng.next();
    for (Key& k : keys.castling)
        k = rng.next();
    for (Key& k : keys.enPassant)
        k = rng.next();
    keys.side = rng.next();
    return keys;
}

inline constexpr Keys kKeys = makeKeys();

constexpr Key piece(Piece p, Square s) {
    return kKeys.pieceSquare[p][s];
}
constexpr Key castling(int rights) {
    return kKeys.castling[rights];
}
constexpr Key enPassant(int file) {
    return kKeys.enPassant[file];
}
constexpr Key side() {
    return kKeys.side;
}

} // namespace zugzwang::zobrist
