---
description: Build Release and run the perft suite; debug any failure
allowed-tools: Bash(cmake:*), Bash(./build/*), Read, Grep, Edit
---
1. `cmake -S engine -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`
2. `./build/zugzwang_tests "[perft]"`
3. All pass: report total time and stop.
4. Any failure: report FEN, depth, expected and actual counts. Narrow it down with `./build/zugzwang` → `position fen <FEN>` → `perft <depth>` (divide output) and compare per-move counts against a reference divide of the same position. Fix the generator. Never edit expected counts.

$ARGUMENTS
