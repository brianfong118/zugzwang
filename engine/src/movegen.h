#pragma once

#include "move.h"
#include "position.h"

namespace zugzwang {

// Pseudo-legal: a move may expose the king or castle through attack; legality is checked at make.
MoveList generateMoves(const Position& pos);

} // namespace zugzwang
