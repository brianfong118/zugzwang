# Zugzwang

[![CI](https://github.com/brianfong118/zugzwang/actions/workflows/ci.yml/badge.svg)](https://github.com/brianfong118/zugzwang/actions/workflows/ci.yml)

A UCI chess engine in C++20, being built one measured milestone at a time. The goal is a bot on Lichess with hand-crafted evaluation, then a self-trained NNUE.

**Status: v1.0.** It searches with move ordering and quiescence, evaluates with PeSTO's tapered piece-square tables, and is set up to play on Lichess through lichess-bot. Every change meant to gain strength passed an SPRT against the build before it; the results are in the [roadmap](docs/ROADMAP.md). v1.x adds a transposition table and pruning, one measured change at a time.

## Verified move generation

Perft counts every legal move sequence to a fixed depth. One wrong chess rule changes the total, so matching [published counts](https://www.chessprogramming.org/Perft_Results) is strong evidence that the rules are right. Zugzwang matches on every position in the suite:

| Position | Depth | Nodes |
|---|---|---|
| Start position | 6 | 119,060,324 |
| Kiwipete | 5 | 193,690,690 |
| Position 3 | 6 | 11,030,083 |
| Position 4, and its colour-swapped mirror | 5 | 15,833,292 |
| Position 5 | 5 | 89,941,194 |
| Position 6 | 5 | 164,075,551 |

CI runs this suite on every push, and runs the shallower tests under AddressSanitizer and UndefinedBehaviorSanitizer.

Speed is about 21 million nodes per second from the start position and 27 million on Kiwipete, on one thread of an i5-1235U. Every move is played and checked for legality, including on the last ply. Engines that use bulk counting skip that step, so their perft speeds aren't directly comparable.

## How it works

- **Board:** a bitboard per piece type and colour, plus a 64-square array for piece lookups.
- **Sliding pieces:** magic bitboards, with the magics found at startup from a fixed seed.
- **Moves:** 16-bit moves in a fixed-size list, so generation never allocates.
- **Legality:** moves are generated pseudo-legally and checked in `make`, which undoes any illegal move itself.
- **Hashing:** a Zobrist key updated incrementally on every move.
- **Search:** iterative-deepening negamax with alpha-beta pruning on its own thread, so `stop` works mid-search. Mates score by distance, and repetitions and the fifty-move rule score as draws.
- **Move ordering:** captures first, most valuable victim and then least valuable attacker (MVV-LVA). Moves are picked one at a time, so a cutoff skips ordering the rest.
- **Quiescence:** at the end of the main search, captures and queen promotions are played out until the position is quiet, so no position is scored mid-exchange.
- **Evaluation:** PeSTO's piece values and piece-square tables, blending middlegame and endgame tables by the material left on the board.
- **Time:** a budget of about a thirtieth of the remaining clock plus the increment, never more than a tenth of the clock. No new depth starts after half the budget, and the search stops mid-depth at a tenth of the clock.
- **Bench:** `bench` searches fixed positions to a fixed depth. Its node count changes exactly when search behaviour does, so every search change records it.

[DESIGN.md](docs/DESIGN.md) records each decision and the reason for it.

## Build and test

You need CMake 3.25+, Ninja and a C++20 compiler. It's tested on Linux with GCC 13 and 15. Catch2 is downloaded when you configure.

```
cmake -S engine -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/zugzwang_tests            # everything, about 25 seconds
./build/zugzwang_tests "~[slow]"  # skips the deep perft cases
```

## Try it

`./build/zugzwang` reads commands from standard input. It speaks UCI (`uci`, `isready`, `position`, `go` with clocks, `depth`, `nodes`, `movetime` or `infinite`, `stop`, `quit`) and adds `perft` and `bench`. Arguments run as a single command, so `./build/zugzwang bench` prints the bench result and exits. `perft` prints each first move with its own count, which narrows a wrong total down to one move:

```
Zugzwang 1.0.0
position fen 4k3/8/8/8/8/8/4P3/4K3 w - - 0 1
perft 1
e2e3: 1
e2e4: 1
e1d1: 1
e1f1: 1
e1d2: 1
e1f2: 1

Nodes searched: 6
```

## License

[MIT](LICENSE)
