# Design

Decisions and their reasons. When a decision changes, mark the old one superseded rather than deleting it.

## Toolchain
- C++20, CMake, Ninja. GCC/Clang via MSYS2 UCRT64 on Windows; GCC on Linux CI. One compiler family means one flag set shared by dev machine and CI: `-O3 -march=native` in Release; `-fsanitize=address,undefined` in Debug on Linux (MinGW GCC lacks ASan, so CI runs the sanitizers).
- `<bit>` over intrinsics: portable, identical codegen at `-O3`.
- Catch2 via FetchContent. Perft is the primary correctness check; unit tests cover FEN round-trips, UCI parsing and eval symmetry (mirrored position, negated score).
- CI builds Release and Debug on every push and runs the tests under sanitizers, excluding `[slow]` perft cases.

## Board
- Bitboards: one per piece type per colour, two colour occupancies, plus a 64-entry mailbox so piece-at-square is a lookup, not a 12-bitboard scan.
- Magic bitboards for sliders, magics found at startup. Avoids committing a table; the cost is milliseconds. PEXT can be added behind an `#ifdef` later.
- Make/unmake with an undo stack rather than copy-make: cheaper once state grows to include NNUE accumulators.
- Zobrist hash updated incrementally from day 1: needed for repetition detection in v1 and the TT in v1.1.
- 16-bit moves (from, to, 4 flag bits for promotion/castling/en passant). Compact in move lists and TT entries.
- Pseudo-legal generation, legality checked in make. Simpler than legal generation; the wasted work is small.

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
