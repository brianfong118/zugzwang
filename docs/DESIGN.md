# Design

Decisions and their reasons. When a decision changes, mark the old one superseded rather than deleting it.

## Toolchain
- C++20, CMake, Ninja. GCC/Clang via MSYS2 UCRT64 on Windows; GCC on Linux CI. One compiler family means one flag set shared by dev machine and CI: `-O3 -march=native` in Release; `-fsanitize=address,undefined` in Debug on Linux (MinGW GCC lacks ASan, so CI runs the sanitizers).
- `<bit>` over intrinsics: portable, identical codegen at `-O3`.
- Flags live on one `zugzwang_options` INTERFACE target that every executable links, so the engine and the tests are always built the same way.
- Engine sources compile once into a static library `zugzwang_core`; `main.cpp` holds only the entry point. The test binary links the same objects the engine ships, and supplies its own `main` so it can run `attacks::init()` before Catch2 starts.
- MinGW builds link `-static`. The engine binary is copied next to lichess-bot and must not depend on whichever MSYS2 runtime DLLs are on PATH; a toolchain newer than the installed `libstdc++-6.dll` otherwise fails at load with `STATUS_ENTRYPOINT_NOT_FOUND`.
- Catch2 via FetchContent. Perft is the primary correctness check; unit tests cover FEN round-trips, UCI parsing and eval symmetry (mirrored position, negated score).
- CI builds Release and Debug on every push and runs the tests under sanitizers, excluding `[slow]` perft cases.

## Board
- Bitboards: one per piece type per colour, two colour occupancies, plus a 64-entry mailbox so piece-at-square is a lookup, not a 12-bitboard scan.
- Magic bitboards for sliders, magics found at startup. Avoids committing a table; measured cost is ~100 ms per process with an untuned seed (rooks need ~1M candidate multipliers, bishops ~80k). Per-rank tuned seeds would cut that to ~10 ms if startup time ever matters; it does not for lichess-bot or SPRT. PEXT can be added behind an `#ifdef` later.
- Leaper tables (pawn, knight, king) are `constexpr`: zero startup cost and testable with `STATIC_REQUIRE`. Slider tables cannot be, since the random magic search exceeds compile-time evaluation limits.
- `attacks::init()` is explicit, called from `main()` and the test main, rather than a static initialiser. Nothing that runs during static initialisation can then depend on the tables being ready.
- Slider lookups stay inline in the header (`Magic` table declared `extern`) because they sit on the move-generation hot path; the search that fills the tables lives in the `.cpp`.
- One fixed-seed SplitMix64 PRNG (`prng.h`) serves magic search and Zobrist keys: the same magics and hashes every run, so bench and hash-dependent behaviour are reproducible.
- Make/unmake with an undo stack rather than copy-make: cheaper once state grows to include NNUE accumulators.
- Zobrist hash updated incrementally from day 1: needed for repetition detection in v1 and the TT in v1.1.
- 16-bit moves (from, to, 4 flag bits for promotion/castling/en passant). Compact in move lists and TT entries.
- Pseudo-legal generation, legality checked in make. Simpler than legal generation; the wasted work is small.
- `Position::fromFen` returns `std::optional` rather than throwing: the UCI loop must survive a bad `position` command, and the codebase has no exceptions. Validation is structural only: eight ranks of eight, exactly one king per side, no pawns on the back ranks, en passant square on the correct rank with the pushed pawn behind it. A castling right whose king or rook is not on its home square is dropped rather than rejected (as Stockfish does), so the board guarantees the invariant "right set ⇒ king and rook at home" and castling generation need not re-check it.
- The en passant square is stored and printed as given, but hashed only when a pawn of the side to move can actually capture. FIDE repetition compares the set of legal moves, so an uncapturable ep square must not distinguish two otherwise identical positions.
- Zobrist keys are a `constexpr` table generated from the fixed-seed PRNG; the sixteen castling keys are indexed by the rights mask so an update is `key ^= castling[old] ^ castling[new]`. Piece placement updates the key incrementally from the first `putPiece`; `computeKey()` recomputes from scratch and exists only to check the incremental key in tests and debug builds.

## Search
- v1: iterative deepening, negamax alpha-beta, quiescence (captures only, stand-pat), MVV-LVA ordering. Quiescence is in v1 because material-only eval without it hangs pieces at the horizon and the bot would look broken rather than weak.
- v1.x: transposition table, killer and history ordering, null-move pruning, late move reductions, check extensions, aspiration windows. One change per SPRT.
- Search runs on a worker thread; the UCI thread only parses input and sets an atomic stop flag. Required for `stop` to work mid-search.

## Evaluation
- v1: tapered material + PeSTO piece-square tables. Public, known-good values; removes tuning from the critical path to Lichess.
- v2: NNUE. HalfKA-style features, one hidden layer, int16 accumulators updated incrementally in make/unmake, SIMD inference. Training data from self-play of the best v1.x build so the net is trained on this engine's own data, not Stockfish's. Trainer in `trainer/` (Python, PyTorch). Nets attached to GitHub Releases, not committed.

## Time management
- v1: budget `remaining/30 + increment` per move, hard cap `remaining/10`. Stop iterating when the next depth is predicted not to fit.

## UCI and Lichess
- `uci`, `isready`, `ucinewgame`, `position`, `go` (`wtime btime winc binc movetime depth nodes infinite`), `stop`, `quit`. Extensions: `bench`, `perft <depth>` (divide output).
- The official lichess-bot Python bridge connects the binary to Lichess. A hand-written bridge signals nothing; the official one already handles the Bot API, challenges and clocks.

## Measuring strength
- `bench`: fixed positions at fixed depth, prints total nodes and nps. The node count is a fingerprint — it changes iff search or eval behaviour changed.
- SPRT with fastchess at 8+0.08 against the previous accepted build. Lichess rating is the public scoreboard, not the development signal.

## Repository policy
- Public from day 1, MIT.
- Claude Code writes the code; the user reviews and commits. `.claude/settings.json` denies `git commit` and `git push` so the boundary is enforced, not just requested.
- Gitignored: build output, `lichess-bot/` (holds the API token), training data, nets, opening books, `.claude/settings.local.json`.
