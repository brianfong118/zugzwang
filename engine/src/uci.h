#pragma once

#include <iosfwd>

namespace zugzwang {

// Runs commands from in until quit or end of input; attacks::init() must already have run.
void uciLoop(std::istream& in, std::ostream& out);

} // namespace zugzwang
