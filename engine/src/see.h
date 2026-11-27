#pragma once

#include "move.h"
#include "position.h"

#include <array>

namespace zugzwang {

// Minor pieces are equal so trading them is even; a legal exchange never captures the king.
inline constexpr std::array<int, 6> kSeeValues{100, 300, 300, 500, 900, 0};

// Whether the exchange m starts on its square nets at least threshold; DESIGN.md gives its limits.
bool seeGe(const Position& pos, Move m, int threshold);

} // namespace zugzwang
