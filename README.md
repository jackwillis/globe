# Globe

An interactive, software-rendered Earth for PowerPC G4: NetBSD/X11 and classic Mac OS 9.
The current v2 work includes a gray moving land/ocean base, cancellable settled terrain,
hypsometric and cel-shaded styles, Antarctic ice and a one-degree lake mask. The X11 edition
also has a utility toolbar, country picking, city markers and scrolling information windows.

This is work toward v0.1. Earlier X11 builds have been tried by the owner; current headless
checks pass on host and G4. The OS 9 application is **built and packaged, not run**. CI is
configured but has not run. Real-display timing and current hardware UI acceptance remain open.

- [Current handoff](docs/handoff.md) and [prioritized backlog](docs/backlog.md)
- [Architecture](docs/architecture.md), [design and ADR](docs/adr/0001-m2-to-m5-decisions.md)
- [Terrain measurements](docs/terrain.md), [performance history](docs/perf/README.md)
- [Toolchains](docs/toolchain.md) and [OS 9 acceptance](docs/os9-proof.md)

## Build and use

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/globe-x11
./build/globe-bench --refine --convert --repeat 2 tests/data/bench_drag.txt
```

X11 requires an authorized display; the host UI tests use private Xvfb displays. For sanitizers,
configure a separate build with `-DGLOBE_SANITIZE=ON`. The current host suite has 35 tests.

Drag to rotate north-up; T switches to trackball mode. Wheel or +/- zooms, double-click centers
and zooms, arrows turn, N aligns north, G/B toggle graticule/borders, R resets and Q/Escape quits.
S switches the look (Map or Relief) and P (X11) opens a looks palette window; A click without dragging preserves the view; on X11 it identifies a
country (outlined in orange) or nearby city after 0.4 s, unless a second click (zoom) follows. The Style toolbar button is checked for Relief; F2 restores a hidden
toolbar. Exposing or moving a window resends cached pixels. The graticule hides during an active drag.

## Two editions

- `tools/build-edition.sh full`: native X11 executable with embedded atlas, elevation, countries
  and places; local optional elevation packs. [Import](docs/elevation-import.md),
  [toolbar](docs/toolbar.md), [countries](docs/country-format.md), [places](docs/place-format.md).
- `tools/build-edition.sh floppy`: classic PowerPC OS 9 application with atlas and one-degree
  elevation; no toolbar or place cards in this proof of life. It emits a MacBinary application,
  a real 1.44 MB HFS image and a checked [size report](docs/floppy-size.md).

Both use the same portable core and strict pack formats. Terrain is currently raw embedded v1
data; [compression has been measured](docs/perf/compression.md), with runtime adoption still open.
There is no fetch implementation yet; [the fetch design](docs/fetch-design.md) is for review.
No relay is planned. Sources and attribution are in [data/SOURCES.md](data/SOURCES.md).

`tools/probe-retro68.sh` compile-checks the portable code for PowerPC and 68k. License:
GPL-3.0-or-later. The historical v1 prototype is retained as tag `v1`.
