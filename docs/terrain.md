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
