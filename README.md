# Globe

A tiny, offline-first, software-rendered interactive 3D Earth for PowerPC G4
(NetBSD/X11 and classic Mac OS 9). Status: **M1** (responsive globe): an interactive X11 window on a bundled coarse atlas, a headless
benchmark, and a replayable message core. Not yet run on the G4's real display.

- Design: [docs/v2-design.md](docs/v2-design.md)
- What exists and how it fits together: [docs/architecture.md](docs/architecture.md)
- Measured performance: [docs/perf/README.md](docs/perf/README.md)
- Toolchains and measured limits: [docs/toolchain.md](docs/toolchain.md)
- Relationship to the v1 prototype: [docs/prototype-notes.md](docs/prototype-notes.md)

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

```
./build/globe-x11                      # needs a display; drag to rotate, wheel/+/- zoom, Esc quits
./build/globe-bench tests/data/bench_drag.txt    # headless timing, JSON output
```

`-DGLOBE_SANITIZE=ON` adds ASan and UBSan. `tools/probe-retro68.sh` compiles the core
for PowerPC and 68k inside the Retro68 container (podman). Licence: GPL-3.0-or-later.
