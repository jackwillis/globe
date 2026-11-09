# Performance measurements — historical M1 baseline

These are the original baseline measurements. Later [scaling](scaling.md) and
[current terrain costs](../terrain.md) supersede the
original speed questions below.

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

For the G4, cross-build as described in `docs/toolchain.md`, push with `private host-control helper push nb`, and run
over `private host-control helper nb`. Nothing is compiled on shared G4.

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

## Shaded terrain fill (2026-11-01)

`shade_bench` times `fill_triangle_shaded` (two triangles per 1-degree cell) against the existing flat `draw_cell` on the same cells, projection excluded ([raw](shade-fill-g4.json)). At 640x480 on the G4 with the globe radius at 220 px (about 3.8 px per cell, 152k pixels): flat 107.5 ms, shaded 185.6 ms (+78 ms; cost is per-cell and per-scanline setup, not per pixel). At radius 440 (2x zoom, full window): flat 106.4 ms, shaded 99.4 ms. Host: 2.4 vs 3.8 ms and 2.0 vs 2.1 ms. Frame checksums match host and G4 in all four cases.
The existing G4 terrain refine renders in about 0.58 s in total, so smoothing adds roughly 13% at 1x and nothing at 2x. Not measured: the shaded fill wired into the real refine path, band cancellation, and real-display submission. The shaded path skips limb cells (a corner behind the horizon) that the flat path clips.

## Refine profile (2026-11-01, `refine-profile-g4.json`)

`globe-refine-profile` times the settled terrain refine frame by phase at 640x480 (host median of
51 runs, G4 median of 9; host and G4 frame checksums match). G4, 1x zoom: the real `RefinementJob`
takes 786 ms with terrain and 456 ms without it; the terrain phases are about 215 ms (project
corners 18, bound cells 26, per-band cell scan 47, fills 123 by subtraction). The same line layers
drawn in one pass cost 28 ms, so band-by-band line drawing is about 16x a single pass. About 115 ms
of the job is not accounted for by any measured phase (inferred to be per-work-unit stepping
overhead). A hemisphere pre-test before projecting skips half the corners but saves under 1 ms on
the G4.

## Lines once per refine (GL-6, `gl6-lines-once-g4.json`)

The refinement job redraws each 16-row band, so every band used to re-walk all arcs. Each arc is now
tested against the band's rows first (endpoint rows plus a chord-bulge allowance `R * 0.31 * |a-b|^2`
and a margin, with a cached-cap row test per polyline) and skipped when it cannot touch the band.
Final pixels are unchanged: bench refine checksum `1228dcc860a92a77` on host and G4, and the profile
job checksums equal the base build's at both zooms. G4, 640x480, median of 9: the profile's refine job
at 1x zoom fell from 783-786 ms to 378-381 ms with terrain and from 456 ms to 82 ms lines-only (a
single pass is 28 ms); at 2x from 1014-1020 ms to 388-394 ms. The bench's refine frame (the drag
trace's final, deeper view) fell from about 575 ms to about 445 ms. That is the "573 ms" figure in
`terrain.md`: a different view from the profile's centered 1x globe, which explains the 573 vs 786 ms
discrepancy. One base run at 1495 ms was disturbed and is not used. Not measured: real-display
submission and the OS 9 front end.

## GL-29: compressed grid and point decoders on the G4 (2026-11-02)

`globe-codec-bench` (`src/platform/bench/codec_bench.cpp`) times the prototype decoders in
`src/data/grid_codec.cpp` (average predictor plus adaptive binary range coder, untiled, one stream) and
`src/data/point_codec.cpp` (delta zigzag varint), against vectors from `tools/grid-codec/grid_codec.py`.
Median of 15, `-O2` static cross build, two G4 runs agreed (`compression-decode-g4.json`):

| Data | Cells / points | Stored | G4 | Host |
| --- | --- | --- | --- | --- |
| 1 degree grid | 64,800 | 29,693 (raw 64,800) | 19.0 ms, 293 ns/cell | 1.7 ms |
| 0.25 degree grid | 1,036,800 | 342,073 (raw 1,036,800) | 301 ms, 290 ns/cell | 22.8 ms |
| Atlas points | 8,120 | 26,736 (raw 32,480) | 1.2 ms, 151 ns/point | 0.017 ms |

Decoded-cell CRC-32s (`86ea25c2`, `4716f1a5`) and the atlas tail sum match between host and G4.
Code size (PowerPC object text, excluding `crc32`): grid decoder 2,064 bytes at `-O2`, 1,476 at `-Os`;
point decoder 696 and 476. Both compile warning-free with the Retro68 PowerPC and 68k compilers
(compile only). Not measured: running on OS 9, tiled decoding, decode into the terrain scene, or
real floppy reads. Cost reading (inference): a 1 degree pack costs about 19 ms once at load, and a
0.25 degree pack about 0.3 s, so it should decode once at load or per region, not per frame.
