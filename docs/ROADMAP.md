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
- [x] PeSTO tapered eval, replacing material-only
- [x] Time management
- [x] Fresh Lichess account, upgraded to BOT via API
- [x] lichess-bot cloned into `lichess-bot/`, `config.yml` set (token, engine dir); confirm `git status` shows nothing from `lichess-bot/` before first run
- [x] First game played; initial rating logged below

## v1.x — Search improvements, one SPRT each
- [x] Transposition table
- [x] Killer + history ordering
- [x] Null-move pruning
- [x] Late move reductions
- [ ] Check extensions
- [ ] Aspiration windows
- [ ] TT move ordering, multi-bucket TT

## v2 — NNUE
- [ ] Self-play data generation (position, search score, result) from the best v1.x build
- [ ] `trainer/` PyTorch trainer exporting a quantised net
- [ ] Net loader, incremental accumulator, SIMD inference in the engine
- [ ] SPRT NNUE vs HCE; ship on pass
- [ ] Net published as a GitHub Release asset

Machines: `PC` is an AMD Ryzen 5 5600X on Windows; `laptop` is the Linux development laptop.

## Lichess rating log
| Date | Version | Machine | Bullet | Blitz | Rapid |
|------|---------|---------|--------|-------|-------|
| 2026-11-15 | v1.0 | PC | 2156 (12 games) | 2185 (12 games) | – (unplayed) |
| 2026-11-18 | v1.x: transposition table | PC | 2004 (29 games) | 1962 (41 games) | – (unplayed) |
| 2026-11-22 | v1.x: killer + history ordering | PC | 2030 (33 games) | 1990 (40 games) | – (unplayed) |
| 2026-11-24 | v1.x: null-move pruning | PC | 2094 (46 games) | 2026 (61 games) | – (unplayed) |

## Bench log
| Date | Change | Machine | Nodes | NPS |
|------|--------|---------|-------|-----|
| 2026-11-02 | First bench: iterative deepening, alpha-beta, material eval; 10 positions at depth 5 | laptop | 10503664 | 21.2M |
| 2026-11-05 | MVV-LVA move ordering | laptop | 686199 | 9.1M |
| 2026-11-05 | Quiescence search | laptop | 942116 | 18.5M |
| 2026-11-05 | PeSTO tapered eval | laptop | 1202200 | 5.3M |
| 2026-11-15 | Portability check: v1.0 built on the PC, same node count as the laptop | PC | 1202200 | 9.2M (unpinned) |
| 2026-11-17 | Transposition table: 64 MB, one entry per slot, cutoffs only; v1.0 baseline 9.0M pinned | PC | 934906 | 8.8M (pinned) |
| 2026-11-19 | Killer + history ordering: two killers per ply, butterfly history with gravity and malus; TT baseline 8.5M pinned | PC | 591710 | 8.3M (pinned) |
| 2026-11-23 | Null-move pruning: R = 3 + depth/4 from depth 2, not in check, after a pass or with only king and pawns; killer baseline 8.5M unpinned | PC | 429328 | 8.5M (unpinned) |
| 2026-11-25 | Late move reductions: quiet non-killers from the fourth legal move at depth 3+, not in or giving check, R = 0.75 + ln(depth) ln(move)/2.25 capped at depth − 2; null-move baseline 8.5M pinned | PC | 138168 | 8.1M (pinned) |

## SPRT log
| Date | Change | Machine | Result | Games | W-L-D | Elo |
|------|--------|---------|--------|-------|-------|-----|
| 2026-11-02 | Setup check: v0.2 against itself, capped at 200 games | laptop | Inconclusive, as expected (LLR 0.09) | 200 | 89-84-27 | +8.7 ± 31.8 |
| 2026-11-05 | MVV-LVA move ordering | laptop | Pass (LLR 2.95) | 362 | 263-26-73 | +272.2 ± 37.1 |
| 2026-11-05 | Quiescence search | laptop | Pass (LLR 2.94) | 436 | 247-45-144 | +174.2 ± 28.3 |
| 2026-11-05 | PeSTO tapered eval | laptop | Pass (LLR 2.95) | 308 | 289-10-9 | +522.5 ± 87.2 |
| 2026-11-05 | Soft limit at the old budget, hard limit remaining/10 | laptop | Fail (LLR -2.96) | 2648 | 913-1073-662 | -21.0 ± 11.3 |
| 2026-11-05 | Soft limit at half the budget, hard limit remaining/10 | laptop | Pass (LLR 2.97) | 2028 | 864-668-496 | +33.7 ± 13.4 |
| 2026-11-17 | Transposition table | PC | Pass (LLR 2.95) | 632 | 327-148-157 | +101.2 ± 21.8 |
| 2026-11-19 | Killer + history ordering | PC | Pass (LLR 2.95) | 464 | 293-79-92 | +173.3 ± 31.5 |
| 2026-11-23 | Null-move pruning | PC | Pass (LLR 2.95) | 604 | 316-133-155 | +108.7 ± 22.9 |
| 2026-11-25 | Late move reductions | PC | Pass (LLR 2.95) | 656 | 320-133-203 | +101.9 ± 22.7 |
