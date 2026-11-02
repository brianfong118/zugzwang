#include "uci.h"

#include "bench.h"
#include "movegen.h"
#include "perft.h"
#include "position.h"
#include "search.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <istream>
#include <mutex>
#include <optional>
#include <ostream>
#include <sstream>
#include <stop_token>
#include <string>
#include <string_view>
#include <thread>
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

struct GoCommand {
    SearchLimits limits;
    bool infinite = false;
};

// Clocks may be zero and other values must be positive. Unsupported parameters are skipped.
std::optional<GoCommand> parseGo(std::istringstream& args, Color us) {
    constexpr std::array<std::string_view, 2> kClock{"wtime", "btime"}, kIncrement{"winc", "binc"};
    GoCommand go;
    bool limited = false;
    std::optional<std::int64_t> remaining;
    std::int64_t increment = 0;
    std::string token;
    while (args >> token) {
        if (token == "infinite") {
            go.infinite = true;
            continue;
        }
        bool clock = token == "wtime" || token == "btime" || token == "winc" || token == "binc";
        if (!clock && token != "depth" && token != "nodes" && token != "movetime") continue;

        std::string text;
        std::int64_t value = 0;
        args >> text;
        auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec != std::errc{} || ptr != text.data() + text.size() || value < (clock ? 0 : 1))
            return std::nullopt;

        if (!clock) limited = true;
        if (token == kClock[us])
            remaining = value;
        else if (token == kIncrement[us])
            increment = value;
        else if (token == "depth")
            go.limits.depth = int(std::min<std::int64_t>(value, kMaxPly));
        else if (token == "nodes")
            go.limits.nodes = std::uint64_t(value);
        else if (token == "movetime")
            go.limits.time = std::chrono::milliseconds(value);
    }

    if (remaining) {
        auto budget =
            timeBudget(std::chrono::milliseconds(*remaining), std::chrono::milliseconds(increment));
        // Given movetime as well, the shorter limit wins, as in Stockfish.
        go.limits.time = std::min(go.limits.time.value_or(budget), budget);
    }
    // Without a limit the search ends only at kMaxPly, so it must wait for stop like go infinite.
    go.infinite = go.infinite || (!limited && !remaining);
    return go;
}

std::string formatInfo(const SearchInfo& info, Clock::duration elapsed) {
    std::string line = "info depth " + std::to_string(info.depth);
    if (std::abs(info.score) >= kMate - kMaxPly) {
        // UCI counts mates in moves, not plies, and negative when the side to move is mated.
        int plies = kMate - std::abs(info.score);
        line += " score mate " + std::to_string(info.score > 0 ? (plies + 1) / 2 : -plies / 2);
    } else {
        line += " score cp " + std::to_string(info.score);
    }

    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    line += " nodes " + std::to_string(info.nodes) + " nps " +
            std::to_string(nodesPerSecond(info.nodes, elapsed)) + " time " + std::to_string(ms);
    if (info.pv.length > 0) line += " pv";
    for (int i = 0; i < info.pv.length; ++i)
        line += ' ' + toUci(info.pv.moves[i]);
    return line;
}

// The stop_token overload of wait returns once stop is requested, and the predicate never ends it.
void waitForStop(std::stop_token stop) {
    std::mutex mutex;
    std::condition_variable_any stopped;
    std::unique_lock lock(mutex);
    stopped.wait(lock, stop, [] { return false; });
}

class Uci {
  public:
    explicit Uci(std::ostream& out) : out_(out) {}

    void run(std::istream& in);

  private:
    void send(const std::string& line);
    void go(std::istringstream& args);
    void stopSearch();

    std::ostream& out_;
    // The search thread prints info and bestmove while this one answers, so every write locks.
    std::mutex outMutex_;
    Position pos_ = *Position::fromFen(Position::kStartFen);
    bool infinite_ = false;
    // Declared last, so it is destroyed first: the search is stopped before what it uses goes.
    std::jthread search_;
};

// A GUI reads through a pipe, which buffers by block, so every line is flushed with endl.
void Uci::send(const std::string& line) {
    std::lock_guard lock(outMutex_);
    out_ << line << std::endl;
}

void Uci::stopSearch() {
    if (!search_.joinable()) return;
    search_.request_stop();
    search_.join();
}

void Uci::go(std::istringstream& args) {
    std::optional<GoCommand> parsed = parseGo(args, pos_.sideToMove());
    if (!parsed) {
        send("info string invalid go command");
        return;
    }
    stopSearch();
    infinite_ = parsed->infinite;
    search_ = std::jthread([this, pos = pos_, command = *parsed](std::stop_token stop) mutable {
        Clock::time_point start = Clock::now();
        SearchInfo result = search(pos, command.limits, stop, [&](const SearchInfo& info) {
            send(formatInfo(info, Clock::now() - start));
        });
        // UCI forbids answering go infinite before stop, even if the search has nothing left.
        if (command.infinite) waitForStop(stop);
        send("bestmove " + (result.pv.length > 0 ? toUci(result.pv.moves[0]) : "0000"));
    });
}

void Uci::run(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream args(line);
        std::string command;
        args >> command;

        if (command == "quit") {
            stopSearch();
            return;
        }

        if (command == "uci") {
            send("id name Zugzwang " ZUGZWANG_VERSION);
            send("id author Brian Fong");
            send("uciok");
        } else if (command == "isready") {
            send("readyok");
        } else if (command == "position") {
            if (std::optional<Position> next = parsePosition(args))
                pos_ = std::move(*next);
            else
                send("info string invalid position command");
        } else if (command == "go") {
            go(args);
        } else if (command == "stop") {
            stopSearch();
        } else if (command == "perft") {
            int depth = 0;
            if (args >> depth && (args >> std::ws).eof() && depth >= 1 && depth <= kMaxPerftDepth) {
                stopSearch();
                std::lock_guard lock(outMutex_);
                divide(pos_, depth, out_);
                out_.flush();
            } else {
                send("info string perft needs one depth from 1 to " +
                     std::to_string(kMaxPerftDepth));
            }
        } else if (command == "bench") {
            stopSearch();
            std::lock_guard lock(outMutex_);
            bench(out_);
            out_.flush();
        }
    }

    // No stop can follow the end of input: a limited search may finish, an infinite one cannot.
    if (infinite_) search_.request_stop();
    if (search_.joinable()) search_.join();
}

} // namespace

void uciLoop(std::istream& in, std::ostream& out) {
    Uci(out).run(in);
}

} // namespace zugzwang
