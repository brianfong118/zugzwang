#include "test_util.h"
#include "uci.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>

using namespace zugzwang;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::EndsWith;

namespace {

const std::string kStartTotal = "Nodes searched: 20\n";

std::string run(std::string_view commands) {
    std::istringstream in{std::string(commands)};
    std::ostringstream out;
    uciLoop(in, out);
    return out.str();
}

} // namespace

TEST_CASE("perft prints each root move and the total", "[uci]") {
    CHECK(run("position fen 4k3/8/8/8/8/8/4P3/4K3 w - - 0 1\nperft 1\n") ==
          "e2e3: 1\ne2e4: 1\ne1d1: 1\ne1f1: 1\ne1d2: 1\ne1f2: 1\n\nNodes searched: 6\n");

    std::string depth2 = run("perft 2\n");
    CHECK_THAT(depth2, ContainsSubstring("e2e4: 20\n"));
    CHECK_THAT(depth2, EndsWith("Nodes searched: 400\n"));
}

TEST_CASE("position sets up FENs and plays moves", "[uci]") {
    CHECK_THAT(run("perft 1\n"), EndsWith(kStartTotal));
    CHECK_THAT(run("position startpos\nperft 1\n"), EndsWith(kStartTotal));
    CHECK_THAT(run("position startpos moves e2e4 e7e5 g1f3\nperft 1\n"),
               EndsWith("Nodes searched: 29\n"));
    CHECK_THAT(run("position fen " + std::string(kKiwipete) + "\nperft 1\n"),
               EndsWith("Nodes searched: 48\n"));
}

TEST_CASE("move text resolves to the right special move", "[uci]") {
    // The new queen checks e8, which leaves the king three moves.
    CHECK_THAT(run("position fen 4k3/P7/8/8/8/8/8/4K3 w - - 0 1 moves a7a8q\nperft 1\n"),
               EndsWith("Nodes searched: 3\n"));
    // The captured d5 pawn is gone, so it has no move of its own.
    CHECK_THAT(run("position fen 4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2 moves e5d6\nperft 1\n"),
               EndsWith("Nodes searched: 4\n"));
    // The rook now on f1 takes f7 and f8 from the king and stops black castling short.
    CHECK_THAT(run("position fen r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1 moves e1g1\nperft 1\n"),
               EndsWith("Nodes searched: 23\n"));
}

TEST_CASE("a bad position command reports it and keeps the current position", "[uci]") {
    for (std::string bad : {
             "position",
             "position nonsense",
             "position fen",
             "position fen garbage",
             "position startpos nonsense e2e4",
             "position startpos moves e2e5",      // not a move
             "position startpos moves e2e4 e2e4", // valid first move must not stick
             "position fen 4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1 moves e2c3", // pinned
             "position fen 4k2R/8/8/8/8/8/8/4K3 w - - 0 1",               // king capturable
             "position fen krKQQQQQ/PrQ4Q/QQ5Q/Q6Q/Q6Q/Q6Q/Q6Q/QQQQQQQQ w - - 0 1", // too many
         }) {
        CAPTURE(bad);
        std::string output = run(bad + "\nperft 1\n");
        CHECK_THAT(output, ContainsSubstring("info string invalid position"));
        CHECK_THAT(output, EndsWith(kStartTotal));
    }
}

TEST_CASE("perft rejects a missing or invalid depth", "[uci]") {
    for (std::string bad : {"perft", "perft 0", "perft -1", "perft x"}) {
        CAPTURE(bad);
        CHECK_THAT(run(bad + "\n"), ContainsSubstring("info string perft needs"));
    }
}

TEST_CASE("the loop ignores unknown input and stops at quit", "[uci]") {
    CHECK(run("").empty());
    CHECK(run("\n   \nnonsense\n").empty());
    CHECK(run("quit\nperft 1\n").empty());
    CHECK_THAT(run("perft 1\r\nquit\r\n"), EndsWith(kStartTotal));
}
