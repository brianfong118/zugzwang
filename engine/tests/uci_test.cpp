#include "test_util.h"
#include "uci.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <chrono>
#include <initializer_list>
#include <sstream>
#include <string>
#include <string_view>

using namespace zugzwang;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::EndsWith;
using Catch::Matchers::StartsWith;

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
             "position startpos moves e2e5",                      // not a move
             "position startpos moves e2e4 e2e4",                 // valid first move must not stick
             "position fen 4k3/8/8/8/8/8/4P3/4K3 w - - 0 1 e2e4", // no moves keyword
             "position fen 4k3/8/8/8/8/8/4P3/4K3 w - - 0 1 move e2e4",    // misspelt keyword
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
    for (std::string bad : {"perft", "perft 0", "perft -1", "perft x", "perft 65", "perft 100000",
                            "perft 99999999999", "perft 1x", "perft 3.5", "perft 1 2"}) {
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

TEST_CASE("uci and isready get their acknowledgements", "[uci]") {
    std::string handshake = run("uci\n");
    CHECK_THAT(handshake, StartsWith("id name Zugzwang "));
    CHECK_THAT(handshake, ContainsSubstring("\nid author "));
    CHECK_THAT(handshake, EndsWith("\nuciok\n"));
    CHECK(run("isready\n") == "readyok\n");
}

TEST_CASE("go reports each depth and ends with the best move", "[uci]") {
    std::string mateInOne = run("position fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1\ngo depth 2\n");
    CHECK_THAT(mateInOne, StartsWith("info depth 1 score cp "));
    CHECK_THAT(mateInOne, ContainsSubstring("\ninfo depth 2 score mate 1 nodes "));
    CHECK_THAT(mateInOne, ContainsSubstring(" pv a1a8\n"));
    CHECK_THAT(mateInOne, EndsWith("\nbestmove a1a8\n"));

    // Black's only move walks into Qh8 mate.
    CHECK_THAT(run("position fen k7/8/1K6/8/8/8/8/7Q b - - 0 1\ngo depth 3\n"),
               ContainsSubstring("info depth 3 score mate -1 "));
    CHECK_THAT(run("position fen 4k3/8/8/8/8/8/8/1N2K3 w - - 0 1\ngo depth 1\n"),
               StartsWith("info depth 1 score cp 320 "));
}

TEST_CASE("go nodes and movetime end the search on their own", "[uci]") {
    std::string byNodes = run("go nodes 1\n");
    CHECK_THAT(byNodes, StartsWith("info depth 1 "));
    CHECK_THAT(byNodes, !ContainsSubstring("info depth 2"));
    CHECK_THAT(byNodes, ContainsSubstring("\nbestmove "));
    // depth 64 keeps end of input from stopping the search, so only movetime can end it quickly.
    CHECK_THAT(run("go depth 64 movetime 20\n"), ContainsSubstring("\nbestmove "));
}

TEST_CASE("stop, quit and a new go end a running search with a best move", "[uci]") {
    CHECK_THAT(run("go infinite\nstop\n"), ContainsSubstring("\nbestmove "));
    CHECK_THAT(run("go infinite\nquit\nisready\n"), !ContainsSubstring("readyok"));
    CHECK_THAT(run("go infinite\nquit\n"), ContainsSubstring("\nbestmove "));
    // End of input means no stop can arrive, so an infinite search is stopped there.
    CHECK_THAT(run("go infinite\n"), ContainsSubstring("\nbestmove "));
    CHECK_THAT(run("go\n"), ContainsSubstring("\nbestmove "));

    std::string restarted = run("go infinite\ngo depth 1\n");
    CHECK(restarted.find("bestmove") != restarted.rfind("bestmove"));
}

TEST_CASE("go infinite holds its best move until stop, even with nothing to search", "[uci]") {
    std::string output = run("position fen k7/1Q6/1K6/8/8/8/8/8 b - - 0 1\ngo infinite\nisready\n"
                             "stop\n");
    CHECK_THAT(output, ContainsSubstring("info depth 1 score mate 0 "));
    CHECK_THAT(output, EndsWith("bestmove 0000\n"));
    CHECK(output.find("readyok") < output.find("bestmove"));
}

TEST_CASE("go reads the clock of the side to move", "[uci]") {
    // depth 64 keeps end of input from stopping the search, so only the clock can end it quickly.
    auto start = std::chrono::steady_clock::now();
    CHECK_THAT(run("go depth 64 wtime 0 btime 3600000 winc 0 binc 0\n"),
               ContainsSubstring("\nbestmove "));
    CHECK_THAT(run("position startpos moves e2e4\ngo depth 64 wtime 3600000 btime 0\n"),
               ContainsSubstring("\nbestmove "));
    // The opponent's hour would allow two minutes per move.
    CHECK(std::chrono::steady_clock::now() - start < std::chrono::seconds(10));
}

TEST_CASE("a bad go command reports it and does not search", "[uci]") {
    for (std::string bad : {"go depth", "go depth 0", "go depth x", "go depth 3x", "go nodes -5",
                            "go movetime", "go movetime 99999999999999999999", "go wtime -1",
                            "go btime", "go winc x", "go binc 1.5"}) {
        CAPTURE(bad);
        std::string output = run(bad + "\n");
        CHECK_THAT(output, ContainsSubstring("info string invalid go command"));
        CHECK_THAT(output, !ContainsSubstring("bestmove"));
    }
}
