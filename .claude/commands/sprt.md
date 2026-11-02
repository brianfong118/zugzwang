---
description: SPRT the current build against a baseline binary with fastchess
argument-hint: [baseline-binary] [elo1]
allowed-tools: Bash(cmake:*), Bash(./build/*), Bash(fastchess:*), Bash(nproc:*), Bash(mkdir:*), Read, Edit
---
Baseline binary: `$1` (default `baseline/zugzwang`). Upper bound in normalized Elo: `$2` (default 5).

1. `fastchess`, `tools/book.epd` and the baseline must exist. If any is missing, stop and point to "Measuring strength" in `docs/DESIGN.md`.
2. `cmake -S engine -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`
3. Run in the background, since it takes minutes to hours, with concurrency = `nproc` − 1:
   ```
   mkdir -p sprt && fastchess -engine cmd=./build/zugzwang name=new -engine cmd=<baseline> name=base \
     -each tc=8+0.08 proto=uci -openings file=tools/book.epd format=epd order=random \
     -rounds 2000 -repeat -recover -concurrency <N> -pgnout file=sprt/last.pgn append=false \
     -config outname=sprt/config.json \
     -sprt elo0=0 elo1=<elo1> alpha=0.05 beta=0.05 model=normalized -ratinginterval 100
   ```
   Let it run until SPRT accepts H0 or H1, or the rounds run out.
4. Report result (pass / fail / inconclusive), games, W-L-D, pentanomial counts, LLR, Elo ± error. A time loss, illegal move or crash invalidates the run: report it and fix the engine first; `sprt/last.pgn` has the games. Append a row to the SPRT log in `docs/ROADMAP.md`.
5. Pass is required before an Elo-intended change is done. Fail means revert or rework, never "keep anyway". Once a passing change is committed, copy `build/zugzwang` to `baseline/zugzwang`.
