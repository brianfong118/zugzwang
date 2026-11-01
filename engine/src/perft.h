#pragma once

#include "position.h"

#include <cstdint>
#include <iosfwd>

namespace zugzwang {

// Counts the leaf nodes of the legal move tree, for comparing against published move counts.
std::uint64_t perft(Position& pos, int depth);

// Prints each root move with its own count, which narrows a wrong total down to one move.
void divide(Position& pos, int depth, std::ostream& out);

} // namespace zugzwang
