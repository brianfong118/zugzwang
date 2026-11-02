# Zugzwang

[![CI](https://github.com/brianfong118/zugzwang/actions/workflows/ci.yml/badge.svg)](https://github.com/brianfong118/zugzwang/actions/workflows/ci.yml)

A UCI chess engine in C++20, being built one measured milestone at a time. The goal is a bot on Lichess with hand-crafted evaluation, then a self-trained NNUE.

**Status: v0.1.** The board, move generation and make/unmake are complete and verified. It does not play chess yet: UCI and a first search come in v0.2, and the search and evaluation it takes to Lichess in v1.0. See the [roadmap](docs/ROADMAP.md).

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

`./build/zugzwang` reads commands from standard input. So far it understands `position`, `perft`, `bench` and `quit`. Arguments run as a single command, so `./build/zugzwang bench` prints the bench result and exits. `perft` prints each first move with its own count, which narrows a wrong total down to one move:

```
Zugzwang 0.1.0
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
