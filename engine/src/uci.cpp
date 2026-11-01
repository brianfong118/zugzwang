#include "uci.h"

#include "movegen.h"
#include "perft.h"
#include "position.h"

#include <istream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace zugzwang {

namespace {

// perft recurses once per ply and no real run comes close, so this only protects the stack.
constexpr int kMaxPerftDepth = 64;

std::optional<Move> parseMove(const Position& pos, std::string_view text) {
    for (Move m : generateMoves(pos))
        if (toUci(m) == text) return m;
    return std::nullopt;
}

// Builds a separate position, so a bad command leaves the current one untouched.
std::optional<Position> parsePosition(std::istringstream& args) {
    std::string token, fen;
    args >> token;
    if (token == "startpos") {
        fen = Position::kStartFen;
        if (args >> token && token != "moves") return std::nullopt;
    } else if (token == "fen") {
        while (args >> token && token != "moves")
            fen += token + ' ';
    } else {
        return std::nullopt;
    }

    std::optional<Position> pos = Position::fromFen(fen);
    while (pos && args >> token) {
        std::optional<Move> move = parseMove(*pos, token);
        if (!move || !pos->make(*move)) return std::nullopt;
    }
    return pos;
}

} // namespace

void uciLoop(std::istream& in, std::ostream& out) {
    Position pos = *Position::fromFen(Position::kStartFen);

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream args(line);
        std::string command;
        args >> command;

        if (command == "quit") break;

        if (command == "position") {
            if (std::optional<Position> next = parsePosition(args))
                pos = std::move(*next);
            else
                out << "info string invalid position command\n";
        } else if (command == "perft") {
            int depth = 0;
            if (args >> depth && (args >> std::ws).eof() && depth >= 1 && depth <= kMaxPerftDepth)
                divide(pos, depth, out);
            else
                out << "info string perft needs one depth from 1 to " << kMaxPerftDepth << '\n';
        }
    }
}

} // namespace zugzwang
