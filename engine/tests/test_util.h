#pragma once

#include "position.h"

#include <catch2/catch_test_macros.hpp>

#include <string_view>

namespace zugzwang {

inline constexpr std::string_view kKiwipete =
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";

inline Position parse(std::string_view fen) {
    auto pos = Position::fromFen(fen);
    REQUIRE(pos.has_value());
    return *pos;
}

} // namespace zugzwang
