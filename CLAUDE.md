# Zugzwang

UCI chess engine in C++20. v1 uses hand-crafted evaluation; NNUE replaces it in v2. Plays on Lichess through the lichess-bot bridge.

## Layout
- `engine/` — C++ engine. `engine/CMakeLists.txt` is the CMake root; sources in `engine/src/`, tests in `engine/tests/`.
- `trainer/` — Python NNUE trainer (v2, empty until then).
- `lichess-bot/` — gitignored clone of lichess-bot plus its config. Never read or edit `lichess-bot/config.yml`; it holds the API token.
- `docs/DESIGN.md` — architecture decisions and their reasons. `docs/ROADMAP.md` — milestones, status, bench/SPRT logs.

## Build and test
Run from the repo root, in Git Bash on Windows (MSYS2 toolchain on PATH) or on Linux:

```
cmake -S engine -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/zugzwang            # UCI binary; `bench` prints nodes/nps, `perft N` prints divide
./build/zugzwang_tests      # Catch2; perft cases tagged [perft], deep ones also [slow]
```

Debug builds (`-DCMAKE_BUILD_TYPE=Debug`) add `-fsanitize=address,undefined` on Linux only; MinGW GCC has no ASan, so CI is where sanitizers run.
Slash commands: `/perft`, `/bench`, `/sprt [baseline] [elo1]`.

## Git rules
- Never run `git commit`, `git push`, or any command that rewrites history or discards work. `git status`, `diff`, `log`, `add` only.
- When a change is done, print a suggested commit message: one imperative line, plus `Bench: N` if search or eval changed. The user commits.
- Never create files containing tokens or credentials.

## Engineering rules
- Perft must pass before any change to board, move generation or make/unmake is called done.
- Every search or eval change reports its bench result. From v1.1, every Elo-intended change is measured with `/sprt`; state the result. Non-functional changes must leave the bench node count unchanged.
- Record design decisions in `docs/DESIGN.md`. Tick milestones in `docs/ROADMAP.md` when they land. Design discussion does not go in code comments.

## Code style
- C++20. Run `clang-format -i` on touched files before finishing.
- Comments: one line, say why, only where the reason isn't obvious. No comments that restate code, no section banners, no comments on trivial functions.
- Files `snake_case.h/.cpp`, one module each. Types `PascalCase`, functions and variables `camelCase`, constants `kPascalCase`.
- Use `<bit>` (`std::countr_zero`, `std::popcount`); no compiler intrinsics outside NNUE SIMD code.
- No exceptions, virtual dispatch or heap allocation on search and eval hot paths.
- Tests in `engine/tests/` with Catch2. Perft expected counts are never edited to make a test pass.

## Working with the user
He knows minimax, alpha-beta, transposition tables and Zobrist hashing; skip explaining those. Explain design choices and trade-offs, briefly. He writes almost none of the code himself, so keep code self-explanatory.
