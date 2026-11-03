# Refinement-order experiment (GL-19 / GL-26, 2026-11-02)

Rows remain the default. To compare an experiment, append one of these timestamped messages to a
trace; both front ends and the headless replay derive the choice from the same model:

```
order 8000 rows
order 8000 center
order 8000 dissolve
```

`center` visits 16-row bands outward from the focus band, ties upward. `dissolve` uses 16
screen-space Gaussian cell passes. No toolbar/effect chooser, pass/surface refactor or smoothing
tuning is included. The experiment can be reviewed before choosing a new default.

## Focus and deterministic pattern

A click or drag release records the last finite pointer coordinate, including motion within drag
slop. A double click records the resulting view center. Focus is model state, not a clock read;
initial state and resize use the viewport center. A plain click still leaves the visible frame,
render generation and wake requests unchanged; it only records the focus for future work. The
existing `up TIME` trace has no coordinates, so release uses the last recorded down/move position.

During the existing per-cell bounds preparation, one byte per cell stores its first eligible
pass (255 for hidden/empty cells). A fixed unsigned 32-bit index hash selects a bin in an offline
256-entry table `L[i] = -2 ln((i + 0.5) / 256)`. Distance is from the average projected corners to
the screen focus. Starting sigma is the projected nominal longitudinal cell size; its square
multiplies by 2.25 at each pass (sigma grows by 1.5). Squared-distance comparisons choose the pass;
there is no per-cell log/sqrt, clock, mutable random generator, sort or frame allocation.
Screen space was chosen to match a round on-screen bell; a sphere-distance variant is not built.

Every pass scans the byte array in index order, drawing eligible cells. The last pass redraws
**all** visible cells in canonical index order, even those painted earlier. This deliberately pays
for a final full fill and preserves existing edge ownership if clipped limb polygons overlap;
final-pixel equality does not depend on assuming a nonoverlapping mesh. Cells use the existing
rasterizer and 16-row clip slices, preserving its rounding and avoiding a giant whole-cell fill.
The experiment returns after at most four admitted slices or the requested work-unit budget.
Rows and center-out use the existing row renderer unchanged.

Lines redraw **once after all terrain passes**, above the terrain, through the existing bounded
arc/polyline continuation with a full-height clip. Earlier terrain passes can be shown without
outlines: this is a visible experiment tradeoff. Each completed dissolve pass reports the full
frame as ready; the final lines report another completed frame. The front ends continue to own
packing/presentation. Cancellation checks generation, framebuffer size and terrain workspace
ownership at every continuation, including pass boundaries. No hard wall-clock limit is promised.

The additional persistent workspace is exactly one byte per cell (64,800 bytes for the bundled
grid), included in `required_bytes()`, `bytes()` and the 32 MiB allocation guard. The 256 doubles
are a 2,048-byte read-only table, outside that heap workspace. The current OS 9 link/package report
is [recorded separately](perf/refine-order-floppy-size.json); it builds and fits, not executed.

## Measurement method

`globe-refine-order --repeat 9 [--trace PATH]` uses the bundled lake-aware data, 640x480, 64 work
units per step, and all three orders on each of these identical views:

- Profile 1x: 30 N / 10 E, radius 220, center focus, all line layers.
- Profile 2x: the same camera direction, radius 440, focus (80,80), all line layers.
- Standard `tests/data/bench_drag.txt` final camera/focus and its traced layer choices.

Timing starts after the initial outline frame and job construction; includes projection, bounds,
assignment, clearing, terrain, scanning, continuation overhead and final lines. It excludes input
replay, framebuffer/cache allocation, checksumming, packing, uploads and scanout. "First pass"
means first complete band for the row orders, first complete cell pass for dissolve, not first
changed pixel; those amounts of completed work differ. Total/first/final-lines are medians of nine.
The report gives both the median of each run's longest continuation and the **largest** continuation
observed across all nine runs. Wall-clock interruptions count; these are not CPU-only times.
`final_lines_ms` is measured from completion of dissolve pass 16 to completion of the lines;
zero for row orders means not separately timed, not free lines.

Host: local GCC Release. G4: locally cross-built NetBSD/macppc GCC 12.5, `-O2 -static`, no compilation
on target, fresh `/tmp/gl19-20261102-*` names, headless only. Raw host/G4 JSON and progress hashes
are in `docs/perf/refine-order-*`. Full-frame hashes here use FNV-1a over every RGB565 pixel with
the standard 64-bit basis; the existing replay pin uses a different sampled, repeated-frame hash.
They must not be compared as though they were the same hash function.

## G4 results and decision

| View | Order | First completed band/pass ms | Total ms | Worst step across 9 runs ms |
| --- | --- | ---: | ---: | ---: |
| profile_1x | rows | 66.509 | 692.629 | 70.753 |
| profile_1x | center | 76.377 | 672.606 | 26.373 |
| profile_1x | dissolve | 84.284 | 796.017 | 26.207 |
| profile_2x_offcenter | rows | 119.257 | 686.689 | 24.613 |
| profile_2x_offcenter | center | 77.100 | 699.826 | 25.080 |
| profile_2x_offcenter | dissolve | 81.787 | 781.978 | 25.746 |
| trace | rows | 66.450 | 644.300 | 25.125 |
| trace | center | 76.299 | 643.818 | 24.797 |
| trace | dissolve | 84.168 | 732.341 | 24.539 |

Dissolve costs about 14–15% more total time in these matched views, including its explicit final
full-cell repaint. Its final line redraw costs 43.609 / 44.438 / 38.117 ms (the two profile views
and trace respectively). First-pass latency is longer than rows at 1x and in the trace; it is
shorter at 2x with the off-center focus, where the baseline's first top band covers different work.

The worst dissolve step was 26.207 ms over these 27 G4 runs, versus the older reported 120.132 ms
continuation. **That does not prove a scheduling improvement over today's baseline:** rows also
stayed below 120 ms in this run (worst 70.753 ms), and the trace's 25.125 vs 24.539 ms worst is
essentially equal. Interrupted wall times vary substantially; the per-run median maxima are also
in the raw report. Do not claim a hard response deadline or attribute the historical 120 ms spike
to smoothing. Smoothing and cell rasterization were not tuned.

Host and G4 full-frame hashes agree for every order in all three views:
`19dd1b02e4cb2a8b`, `572eba5730155418`, `9e6de6b83f243a80`. The established app replay
`9b4a458633a32d9d`, sampled refined replay `efa030af1a10f887` and outline goldens remain unchanged.
No pin file changed. Progress snapshots use two distinct foci at passes 3, 7, 11 and settled with
lines; the host contact sheet was visually inspected. This is headless evidence, not desktop or
scanout acceptance. Full-frame uploads at every dissolve pass may add display cost; it is excluded
from the measured totals and remains owner acceptance work.

**Retain as an opt-in experiment; keep rows as the default.** The screen-space pattern visibly
tracks the touchpoint, but total time, blank outlines during terrain, and presentation cadence
need the owner's hands-on decision. This pass does not make that product choice.

## Verification and failures

GCC ASan/UBSan and clang: 36/36 each, including both private X11 depths and the codec tests merged
before this pass. Tests compare every pixel for several foci (including outside the viewport),
zoom levels, contrasting neighboring terrain and limb clipping; verify center-out first-band
selection, per-pass/final-full-fill work, deterministic focus-sensitive assignment, cancellation
at pass boundaries and exact trace/model roundtrips. The existing refined-pin test now also
replays center and dissolve through the actual benchmark, not only a constructed quality profile.

Five mutants are rejected: lost release focus, center-out replaced by rows, ignored horizontal
focus, omitted final full repaint, and ignored generation cancellation. Initially the focus-distance and
final-repaint mutants survived final-image-only tests: final pixels alone cannot prove focus
or the required final repaint. Independent x/y focus assertions and admitted-slice coverage now
catch both. The restored code passes again. G4 app and terrain unit tests also pass; all eight
partial/settled snapshot hashes match the host. Separate G4 replay validation (two repetitions per
order, `perf/refine-order-replays-g4.json`) reproduces `efa030af1a10f887` for rows, center and dissolve;
its timings are validation-run means/maxima, not the nine-run medians above. New tests first failed on the missing APIs. Clang
caught a missing test `<algorithm>` include; lint caught unchecked fixed-camera optionals. Those
were fixed. A test counter initially updated during the final pass instead of only at its entry;
the pass-boundary snapshot fixed that assertion. Sanitizer execution required leaving the process
sandbox because LeakSanitizer cannot operate under ptrace; the unrestricted run passes.

Both PowerPC and 68k Retro68 compile probes pass (no printed FAIL); the OS 9 application links,
packages with identical HFS fork roundtrip and meets budget. Current OS 9 data fork 241,234 bytes,
payload 277,809, HFS free 1,170,432; data-fork delta +4,118 from the prior 237,116-byte snapshot.
No native OS 9 runtime, CI, current real-display or smoothing-tuning claim.
