#pragma once

#include "position.h"

namespace zugzwang {

// Centipawns from the side to move's point of view, as negamax expects.
int evaluate(const Position& pos);

} // namespace zugzwang
