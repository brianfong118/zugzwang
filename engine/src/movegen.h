#pragma once

#include "move.h"
#include "position.h"

namespace zugzwang {

// Pseudo-legal: a move may expose the king or castle through attack; legality is checked at make.
MoveList generateMoves(const Position& pos);

// The moves that change material, captures and queen promotions, for quiescence to search.
MoveList generateNoisy(const Position& pos);

} // namespace zugzwang
