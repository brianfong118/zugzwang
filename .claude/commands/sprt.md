---
description: SPRT the current build against a baseline binary with fastchess
argument-hint: [baseline-binary] [elo1]
allowed-tools: Bash(cmake:*), Bash(./build/*), Bash(fastchess:*), Bash(nproc:*), Read, Edit
---
Baseline binary: `$1` (default `baseline/zugzwang`). Upper Elo bound: `$2` (default 5).

1. `cmake -S engine -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`
2. Run, with concurrency = cores − 1:
   ```
   fastchess -engine cmd=./build/zugzwang name=new -engine cmd=<baseline> name=base \
     -each tc=8+0.08 proto=uci -openings file=tools/book.epd format=epd order=random \
     -rounds 2000 -repeat -recover -concurrency <N> \
     -sprt elo0=0 elo1=<elo1> alpha=0.05 beta=0.05 -ratinginterval 100
   ```
   Let it run until SPRT accepts H0 or H1.
3. Report result (pass / fail / inconclusive), games, W-L-D, LLR, Elo ± error. Append a row to the SPRT log in `docs/ROADMAP.md`.
4. Pass is required before an Elo-intended change is done. Fail means revert or rework, never "keep anyway".
