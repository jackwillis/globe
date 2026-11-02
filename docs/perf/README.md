# Performance measurements (M1)

Measured 2026-11-01. Every number here comes from a run described below; nothing is extrapolated.
Raw outputs are in this directory.

## What was measured

`globe-bench` (src/platform/bench) replays `tests/data/bench_drag.txt` (377 messages: two drags,
five zoom steps, eight key presses; 374 frames per repetition at 640x480) through `update()` and
`render_globe()`, timing each frame with `CLOCK_MONOTONIC`. With `--convert` it also packs each
frame into a 32-bit X layout. Three repetitions, 1,122 frames per run.

Not measured, and so not claimed: X server submission on real hardware, display scanout or vsync,
idle CPU on the G4, power, and anything at other window sizes. `peak_rss_kb` is `ru_maxrss`.
Timer overhead was 20 ns on the host and 1,591 ns on the G4 (negligible against frame times).

## Results, 640x480, render + pack

| Build | render mean | render p50 | render p95 | render max | pack mean | peak RSS |
| --- | --- | --- | --- | --- | --- | --- |
| Host: Fedora, GCC 16.2.1, `-O2` (`host-release-640-baseline.json`) | 1.04 ms | 1.04 ms | 1.12 ms | 2.10 ms | 0.64 ms | 6.1 MB |
| G4, run 1 (console output; JSON not saved) | 63.6 ms | 62.9 ms | 76.4 ms | 272 ms | 17.1 ms | 9.0 MB |
| G4, run 2 (`g4-640-baseline-run2.json`) | 71.3 ms | 64.2 ms | 88.7 ms | 710 ms | 18.6 ms | 9.0 MB |
| G4, run 3 (`g4-640-baseline-run3.json`) | 64.0 ms | 63.0 ms | 76.5 ms | 384 ms | 17.2 ms | 9.0 MB |

G4 = Power Mac G4 (467 MHz class), NetBSD 11.0 macppc, `powerpc--netbsd-g++` 12.5.0, `-O2 -static`.
The median is stable across runs (62.9 to 64.2 ms); p95 varies a little (76 to 89 ms) and the
maximum a lot (272 to 710 ms), which looks like occasional system noise rather than the renderer, but
that was not investigated. About 5,600 atlas and graticule segments are visible per frame.

**Reading it:** a 640x480 frame costs about 64 ms to render and 17 ms to pack on the G4, so
roughly 12 frames per second before anything is sent to the X server. The host is about 60 times
faster. The frame checksum is identical on both machines (`979403b5c7eeb68f`), so the same pixels
are drawn.

## Where the time goes (G4, mean ms per frame, layers forced with `--layers`)

| Layers drawn | render mean | added by that layer |
| --- | --- | --- |
| none (ocean disc only) | 2.3 | 2.3 |
| graticule | 18.0 | 15.7 |
| coastlines | 30.4 | 28.1 |
| borders | 19.4 | 17.1 |
| all three | 63.2 | (2.3 + 15.7 + 28.1 + 17.1 = 63.2) |

The host splits the same way (0.02 / 0.24 / 0.52 / 0.33 ms; all three 1.06 ms). The layers are
additive, so cost scales with the number of segments, not with filled pixels: the disc fill (the
only per-pixel work) is 2.3 ms.

## Why: libm is expensive on this CPU (micro-benchmark, scratch program, not in the repo)

| call | G4 | host |
| --- | --- | --- |
| loop overhead | 21.5 ns | 4.1 ns |
| `sin` / `cos` | 298 / 297 ns | 11.6 / 11.5 ns |
| `atan2` | 606 ns | 17.8 ns |
| `sqrt` | **1,233 ns** | 4.1 ns |
| `floor` | 113 ns | 4.1 ns |
| multiply-add | 21.5 ns | 4.1 ns |

`sqrt` is about 300 times slower on the G4 than on the host, against about 25 times for `sin`. The
likely explanation is that the G4 has no hardware double-precision square root, so the compiler
calls a software routine. That is an inference from the timing; the generated code was not
inspected. The renderer calls `sqrt` (through `length()`, `normalized()`, `angle_between()`,
`subdivide_arc()`), `sin`/`cos` (per vertex in `to_unit_vector`, per subdivision point) and
`atan2` several times per segment, per frame. Adding those up with the table gives the right order
of magnitude for the measured 63 ms, so these calls are the leading suspect. This has not been
confirmed by profiling or by removing them.

## Idle behavior (host, Xvfb, `globe-x11 --idle-seconds 5`)

With the globe untouched for 5 seconds, the process drew 0 frames and used 0.86 ms of CPU. It
blocks in `XNextEvent`. This is on a software X server on the host, not on the G4, and it is
covered by the `x11_idle` test. Idle CPU on the G4 was not measured.

## Candidate improvements (not done)

Ordered by expected effect, each to be tried and measured, not assumed:

1. Cache each atlas vertex's unit vector at load time (about 195 KB as doubles, 97 KB as floats)
   so the per-frame work per vertex is one matrix-vector product, with no `sin`/`cos`.
2. Cut `sqrt`/`atan2` per segment: choose the subdivision count from the squared chord length,
   subdivide with a normalized linear interpolation (same great circle, no `sin`), and avoid
   normalizing when the arc is short.
3. Cull whole polylines using a bounding vector per polyline, and skip segments off the viewport.
4. Speed up the 17 ms pixel pack (about 55 ns per pixel now): per-row lookup into a reused word,
   or skip packing when the server's layout already matches RGB565.

Changes 1 and 2 will change some drawn pixels slightly (the points still lie on the same great
circle, but sub-points are spaced differently), so the golden frames and pinned hashes would be
regenerated, deliberately, in the same commit.

## Reproduce

```
cmake -S . -B build-rel -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build-rel
./build-rel/globe-bench --convert --repeat 3 tests/data/bench_drag.txt
```

For the G4, cross-build as described in `docs/toolchain.md`, push with `maulctl push nb`, and run
over `maulctl nb`. Nothing is compiled on maul.
