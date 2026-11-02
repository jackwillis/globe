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

## After hiding the graticule while dragging (same trace, same machines)

The graticule is no longer drawn while a drag is in progress (`effective_layers`), and is redrawn
when the button is released (6 extra frames in this trace: 1,128 instead of 1,122).

| | render mean | render p50 | render p95 | render max | pack mean |
| --- | --- | --- | --- | --- | --- |
| Host (`host-release-640-no-grid-while-dragging.json`) | 0.84 ms | 0.84 ms | 0.95 ms | 1.73 ms | see file |
| G4 (`g4-640-no-grid-while-dragging.json`), one run | 48.0 ms | 47.3 ms | 60.0 ms | 134 ms | 17.0 ms |

On the G4 a drag frame (render plus pack) went from about 81 ms to about 65 ms, roughly 12 to 15
frames per second. Zoom steps and key presses in the trace still draw the grid, so the saving is
smaller than the 15.7 ms the graticule costs per frame. One G4 run only; the earlier baseline had
three.

## Speed-ups after hiding the graticule (2026-11-01)

Two changes, measured one at a time on the same trace (640x480, drag frames, render only; pack
is unchanged at about 17 ms). Every variant draws the same frames: the sampled frame checksum is
`485cbfbc9fef11a3` for all of them, on the host and on the G4.

1. **Scene cache.** Each atlas vertex's unit vector (and the graticule's) is computed once at load
   (`Scene`: 227 KB, 13 ms once on the G4) instead of four `sin`/`cos` calls per vertex per frame.
2. **Cheaper arcs.** `clip_arc_to_front` compares a squared quantity; `draw_arc` picks the
   subdivision count from the squared chord length (a square root only for long arcs) and places
   interior points on the chord normalized with two Newton steps, so no `sin`, `atan2` or
   `sqrt` per point. The points are still on the great circle; a test checks drawn pixels stay
   within 2 px of an exactly sampled arc for arcs from 10 to 165 degrees.

| G4, render mean / p50 / p95 | before (grid hidden) | after change 1 | after change 2 |
| --- | --- | --- | --- |
| scene cache on | n/a | 36.1 / 36.7 / 46.1 ms | **20.8 / 21.1 / 24.7 ms** |
| scene cache off (`--reference`) | 48.0 / 47.3 / 60.0 ms | 46.1 / 46.6 / 57.3 ms | 30.7 / 31.1 / 36.5 ms |

(Each cell is one run; 752 frames for the last two columns, 1,128 for "before", so treat
differences of a millisecond or two as noise. Files: `g4-640-change1-*.json`,
`g4-640-change2-*.json`, `g4-640-no-grid-while-dragging.json`.)

Host: 0.82 ms without the cache and 0.49 ms with it (change 1 only, `--repeat 5`); with both
changes, 0.69 ms without and **0.36 ms** with the cache (`host-release-640-change2-*.json`).

**Net effect on the G4:** a drag frame went from about 64 ms (grid on) to 48 ms (grid hidden) to
**20.8 ms** render, which with the 17 ms pack is about 38 ms per frame, roughly 26 frames per
second before X submission (it was about 12). Pixel packing is now the largest single cost.

> **Note (2026-11-01):** the coastline toggle was removed as a user feature, so `bench_drag.txt` no
> longer ends by switching coastlines off. Its frame checksum for `--repeat 2` is now
> `41c7adb508763323` (identical on host and G4); the `485cbfbc9fef11a3` quoted in the sections
> above belongs to the earlier trace. Timings in those sections are historical and unchanged.

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

1. Speed up the 17 ms pixel pack (about 55 ns per pixel), now the largest single cost: a 32-bit
   fast path with unrolled stores, reuse of the previous pixel's packed value in flat areas, or
   skipping packing when the server's layout already matches RGB565.
2. Cull whole polylines using a bounding vector per polyline, and skip segments off the viewport.
3. Replace `std::floor` in `draw_line` (a 113 ns library call on the G4) with an inline floor, and
   skip the Liang-Barsky clip for lines already inside the buffer.

(Done: the vertex cache and the cheaper arcs, above.)

## Reproduce

```
cmake -S . -B build-rel -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build-rel
./build-rel/globe-bench --convert --repeat 3 tests/data/bench_drag.txt
```

For the G4, cross-build as described in `docs/toolchain.md`, push with `maulctl push nb`, and run
over `maulctl nb`. Nothing is compiled on maul.

## One-shot stage wakes (2026-11-01)

Host GCC ASan/UBSan build under a private depth-24 Xvfb, `--size 160x120 --script
 tests/data/trace1.txt --idle-seconds 2 --stats` (the trace resizes to 200x150):
**2 timer wakes**, final stage Refine, **0 idle frames**, **0.104 ms idle CPU** over two seconds.
One correctness run, not a latency benchmark. Resize/expose events caused by the script are drained
before measurement starts. No extra detail is drawn by the stage transitions yet. This verifies
one-shot scheduling and quiescence on the host; it does not measure the G4 or progressive rendering.
The integration test also rejected a missing-wake mutant (0 wakes, final Moving) and a periodic-wake
mutant (1,888 wakes over two seconds, despite 0 idle frames), so frame count alone is insufficient.

## Independent optimization pass (2026-11-01)

The first three candidate improvements above are now implemented and measured, with unchanged
pixel pins. See [pixel packing](pass-20261101-packing.md), [line overhead](pass-20261101-lines.md)
and [polyline culling](pass-20261101-culling.md), with raw host/G4 JSON beside them. At 640x480 on
the G4, mean render plus pack fell from 20.7097 + 17.0486 ms to 17.7811 + 6.9910 ms (about 37.8 to
24.8 ms before X submission). Each step is one matching run; do not read that as real-display FPS.
The Xvfb suite now verifies actual pixels at depths 24 and 16 as well as smoke/idle behavior.

## Staged terrain (2026-11-01)

See [bounded refinement](../refinement.md) and [terrain](../terrain.md). Initial G4 terrain refine
render costs about 0.58 seconds spread over cancellable continuations, versus 0.157 seconds for
finer outlines only. New terrain checksum is pinned independently; moving-frame checksum remains
unchanged. Per-band conversion and real-display submission are additional, not included in that
render total. Raw host/G4 measurements are in terrain-*.json.
