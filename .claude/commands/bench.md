---
description: Run the fixed-depth bench and report nodes/nps against the last logged value
allowed-tools: Bash(cmake:*), Bash(./build/*), Read, Edit
---
1. `cmake -S engine -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`
2. `./build/zugzwang bench`; read the final `nodes N nps M` line.
3. Compare N with the latest row of the bench log in `docs/ROADMAP.md`.
   - Unchanged: the change is non-functional; report nps only.
   - Changed: the change is functional; append a row (date, short description, nodes, nps).
4. Print `Bench: N` for the commit message.

$ARGUMENTS
