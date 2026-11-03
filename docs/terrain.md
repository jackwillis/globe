# Terrain appearance (GL-27, 2026-11-02)

The moving view uses a 36 x 18 sampled land mask from the loaded elevation grid: gray land and the
flat ocean disc, with the existing lines above it. It projects cached grid corners without modifying
the refinement workspace or allocating in a frame. Antarctic land south of -60 degrees is white in
both moving and settled views; negative heights remain water. The global mask is deliberately coarse.

S cycles Hypsometric and Cel-shaded palettes in both front ends; the X11 Style button shows a checked
state for Cel-shaded. Traces accept `style TIME` or `key TIME style`. Style is model state and enters
the derived quality profile; changing it invalidates old refinement.

Settled terrain interpolates palette indices from shared cached corner averages, clamped within the
cell's land/water class. Two shaded triangles cover each front-facing quad. Cells crossing the horizon
retain the existing clipped flat fill so skipping a shaded corner never exposes stale base pixels.
The Antarctic land override precedes palette selection. No GELV format or flag change is needed.
The final frame clears its own bands and is independent of the moving base and initial framebuffer.
Corner shades add one byte per corner; two style tables add 512 bytes over the old cache.

## Measured on the G4

Same 640x480 views as the merged profiler, median of nine, local cross-build GCC 12.5 -O2 -static.
The gray mask alone costs 4.2727 ms at radius 220 and 5.2840 ms at 440 (host 0.1109/0.1181 ms).
Before this change the full refine job was 379.944/386.428 ms; after it was 554.943/692.716 ms.
These are integrated costs, larger than the earlier isolated shading microbenchmark. A separate
standard trace's final view refined in 801.8763 ms mean (two repetitions); its longest continuation
was 120.1318 ms. Moving rendering averaged 32.9700 ms in that run, including the gray mask and lines.
Conversion/submission is additional; no real display timing or hardware interaction acceptance.

Smoothing is retained as a sub-second settled step with bounded work counts, not a hard wall-clock
budget. The longest continuation remains a responsiveness limitation for owner acceptance. No dissolve,
pass/surface refactor or networking is included. Raw before/after/base/replay JSON is in `docs/perf/appearance-*`.

The standard refined pin intentionally changes 1228dcc860a92a77 -> 27336cb8117e5e7f for smoothing and
Antarctic white. The moving benchmark now includes the gray mask (71e66eac3bde93c7 at repeat 2).
Host/G4 refined pixels agree, as do the base-only checksums. The app replay hash remains
9b4a458633a32d9d and outline-only goldens remain unchanged. GCC sanitizer and clang suites pass 34/34;
PowerPC and 68k compile probes and G4 app/terrain tests pass. Mutants disabling Antarctic white,
style propagation, smoothing and water rejection in the base are all caught.

GL-28 subsequently marks inland water without changing the format; the current combined replay
pin is efa030af1a10f887. The GL-27 timing snapshots above precede that data-only change.

## Earlier implementation measurements (historical)

# Forward-projected settled terrain

Moving remains the existing outline renderer. Refine uses the embedded GELV grid: each shared cell
corner's world unit vector is cached at load time. A job incrementally projects those corners and
computes conservative screen-band membership into preallocated workspaces before changing the
framebuffer. Then each band fills forward-projected convex cell polygons, clips crossing cells at
the horizon, and overlays graticule, coastlines and black borders. No inverse projection or per-pixel
trigonometry is used. Height affects color, not the spherical silhouette.

The 256 colors are interpolated by each pack's meter lookup: deep/shallow ocean blues, lowland
greens, higher browns, high white. Scanline pixel-center inclusion is half-open so adjacent quads
share edges. Horizon edges use their projected chords, consistent with this coarse cell mesh;
this is not an exact curved-cell renderer. At one-degree resolution it is a display approximation.

World and per-view projection arrays plus compact cell-band bounds are allocated once. The initial
360x180 grid uses 3,396,080 bytes including the color table. Every job claims a workspace token;
starting another job cancels the earlier owner's future writes, even at the same model generation.
The 32 MiB cache budget is checked before allocation; an unavailable embedded cache falls back to
outlines with an explicit platform diagnostic. Source bytes must remain stable and outlive caches.

## Measured acceptance (2026-11-01)

G4, 640x480, cross GCC 12.5.0 -O2 -static, trace then --refine:

| Rendering | Full refine render | Largest continuation |
| --- | --- | --- |
| Finer outlines only, previous stage | 157.4420 ms | 0.5975 ms |
| Terrain, one repetition | 572.8182 ms | 5.2269 ms |
| Terrain, two repetitions | 576.8473 ms mean | 24.7878 ms |

Continuations are bounded by work units, not a hard real-time wall clock. The larger maximum in the
second run is reported, not assumed away. At roughly 0.58 seconds for an event-interleaved settled
render, this implementation is retained; moving frames still render immediately at the previous
quality. These totals exclude per-band packing and X submission. Real-display settling time and
perceived interaction require the owner. Host full refine render was 14.4707 ms in one run.
Raw results: docs/perf/terrain-*.json.

The optional --refine --repeat 2 frame checksum is newly pinned to `1228dcc860a92a77`, reproduced
on the G4 and host. The --repeat 1 checksum is `bfbb5fe5df7b4a80`. Original moving-frame, replay and
outline goldens are unchanged. The benchmark can export its final framebuffer with --dump-ppm.

Tests cover color, terrain disabled while moving, black overlay borders, workspace cancellation,
and opposing hemispheres. A coarse-grid strict-interior test catches incorrect horizon inclusion;
the original center-only test let that mutant survive. Limb-boundary pixel classification is not
used as the strict-interior oracle. Disabled terrain, missing fill, border color, workspace ownership
and horizon inclusion mutants are rejected. Host GCC sanitizer/clang suites pass 21/21; G4 terrain
unit tests pass. Retro68 PowerPC/68k compile only. Private-Xvfb readback now includes settled bands;
its test wrapper's exit status is authoritative (the previous permissive idle regex was removed).

## Plain vectors while moving (owner decision, 2026-11-02)

The gray moving land mask is off by default: `RenderQuality::gray_base` is false, so a drag shows
plain vectors again. The mask code (`TerrainScene::draw_base`) and its tests stay for a possible
later style. The reference (atlas) render path never draws the base. The "71e66eac3bde93c7" moving
benchmark checksum above describes the mask-on build; the default moving frames are outline-only.
