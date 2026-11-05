# Roadmap

Tick items as they land. Versions are git tags created by the user.

## v0.1 — Board and move generation
- [x] CMake project builds `zugzwang` and `zugzwang_tests` with the commands in CLAUDE.md
- [x] Bitboard board state, FEN parse/print
- [x] Attack tables, magics for sliders
- [x] Zobrist keys
- [x] Move generation, make/unmake
- [x] `perft <depth>` with divide output
- [x] Perft suite passes: start position d6, Kiwipete d5, CPW positions 3–6
- [x] CI green

## v0.2 — UCI and a first search
- [x] Repetition and 50-move detection
- [x] Iterative deepening, negamax alpha-beta, material-only eval
- [x] `bench` command; first bench log row
- [x] UCI loop on main thread, search on worker thread, `stop` works mid-search
- [x] `position` / `go` with all clock parameters

## v1.0 — Lichess
- [x] fastchess installed, opening book in `tools/book.epd`, baseline binary in `baseline/`
- [x] Quiescence, MVV-LVA
- [ ] PeSTO tapered eval, replacing material-only
- [ ] Time management
- [ ] Fresh Lichess account, upgraded to BOT via API
- [ ] lichess-bot cloned into `lichess-bot/`, `config.yml` set (token, engine dir); confirm `git status` shows nothing from `lichess-bot/` before first run
- [ ] First game played; initial rating logged below

## v1.x — Search improvements, one SPRT each
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
| 2026-11-02 | First bench: iterative deepening, alpha-beta, material eval; 10 positions at depth 5 | 10503664 | 21.2M |
| 2026-11-05 | MVV-LVA move ordering | 686199 | 9.1M |
| 2026-11-05 | Quiescence search | 942116 | 18.5M |

## SPRT log
| Date | Change | Result | Games | W-L-D | Elo |
|------|--------|--------|-------|-------|-----|
| 2026-11-02 | Setup check: v0.2 against itself, capped at 200 games | Inconclusive, as expected (LLR 0.09) | 200 | 89-84-27 | +8.7 ± 31.8 |
| 2026-11-05 | MVV-LVA move ordering | Pass (LLR 2.95) | 362 | 263-26-73 | +272.2 ± 37.1 |
| 2026-11-05 | Quiescence search | Pass (LLR 2.94) | 436 | 247-45-144 | +174.2 ± 28.3 |
