# Roadmap

Tick items as they land. Versions are git tags created by the user.

## v0.1 — Board and move generation
- [ ] CMake project builds `zugzwang` and `zugzwang_tests` with the commands in CLAUDE.md
- [ ] Bitboard board state, FEN parse/print
- [ ] Attack tables, magics for sliders
- [ ] Move generation, make/unmake, Zobrist
- [ ] `perft <depth>` with divide output
- [ ] Perft suite passes: start position d6, Kiwipete d5, CPW positions 3–6
- [ ] CI green

## v0.2 — UCI
- [ ] UCI loop on main thread, search on worker thread, `stop` works mid-search
- [ ] `position` / `go` with all clock parameters
- [ ] Repetition and 50-move detection
- [ ] `bench` command; first bench log row

## v1.0 — Lichess
- [ ] Iterative deepening, negamax alpha-beta, quiescence, MVV-LVA
- [ ] Material + PeSTO tapered eval
- [ ] Time management
- [ ] Fresh Lichess account, upgraded to BOT via API
- [ ] lichess-bot cloned into `lichess-bot/`, `config.yml` set (token, engine dir); confirm `git status` shows nothing from `lichess-bot/` before first run
- [ ] First game played; initial rating logged below

## v1.x — Search improvements, one SPRT each
- [ ] fastchess installed, opening book in `tools/book.epd`, baseline binary in `baseline/`
- [ ] Transposition table
- [ ] Killer + history ordering
- [ ] Null-move pruning
- [ ] Late move reductions
- [ ] Check extensions
- [ ] Aspiration windows
- [ ] TT move ordering, multi-bucket TT

## v2 — NNUE
- [ ] Self-play data generation (position, search score, result) from the best v1.x build
- [ ] `trainer/` PyTorch trainer exporting a quantised net
- [ ] Net loader, incremental accumulator, SIMD inference in the engine
- [ ] SPRT NNUE vs HCE; ship on pass
- [ ] Net published as a GitHub Release asset

## Lichess rating log
| Date | Version | Bullet | Blitz | Rapid |
|------|---------|--------|-------|-------|

## Bench log
| Date | Change | Nodes | NPS |
|------|--------|-------|-----|

## SPRT log
| Date | Change | Result | Games | W-L-D | Elo |
|------|--------|--------|-------|-------|-----|
