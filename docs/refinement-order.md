# Refinement order (GL-19 / GL-26, concluded 2026-11-02)

The settled terrain refines **circle-outward from the view center**. The experiment is over: rows,
center-out bands, a Gaussian dissolve, rectangles and globe rings were all tried; the owner chose
circle, which was also the fastest of the shaped orders on the G4 (711 / 765 / 635 ms at 1x / 2x /
trace against globe 793 / 765 / 725 and rectangle 791 / 707 / 723). The other orders, the `order`
trace verb and the X11 `O` key are removed; their measurements stay in `docs/perf/refine-*.json`.
The focus is always the view center (`quality_for` in `src/core/app.cpp`): no touch state is
kept, so clicks and drags cannot affect it. Final pixels never depend on the focus.

## Precomputing during a paused drag

A drag that pauses for `kDragPauseMs` (300 ms, `app.hpp`) with the button still down starts refining
(with the graticule) but **nothing on screen changes**: `presentation_held` tells the platforms to
keep showing the plain moving frame while the refinement computes into the framebuffer. Moving
again cancels it like any input. On release the finished frame is shown at once (or, if the work is
still running, what is done so far, then the rest as usual); a release before the pause restarts the
settle clock as before. Final pixels and pins are unchanged. The platform gating (`present_refine`
in X11, `present` in OS 9) has no automated test beyond the core rules.

## How a refinement runs

With terrain, one job paints the whole frame in 16 passes:

1. Project the cell corners and bound each cell's 16-row slices (as before).
2. The same loop assigns each visible cell a pass byte (255 for hidden cells) from a **per-view
   plan** computed once per job: `make_pass_plan` in `src/render/terrain.cpp`, the data in
   `include/globe/refine_order.hpp`.
3. Pass k clears nothing and draws the cells whose byte is k, with the existing rasterizer in
   16-row clip slices (at most four slices or the work-unit budget per continuation).
4. One more pass redraws only the cells clipped at the horizon, in index order (see "The final
   pass" below; until 2026-11-06 this pass repainted every cell). The settled image is identical
   for every focus.
5. The lines (graticule, coastlines, borders) are drawn once, above all terrain, through the
   existing bounded arc/polyline continuation. Earlier passes therefore show terrain without
   outlines.

Cancellation checks the generation, the framebuffer size and the terrain workspace owner at every
continuation, including pass boundaries. Without terrain the outline-only refine keeps its
top-down 16-row bands (there is nothing to order).

### Pass assignment, per cell

The squared distance of the cell's projected center (mean of its four corners) is compared against
15 squared thresholds, equal-width steps from 0 to the farthest point of the disc. No `sqrt`, `log`,
division, clock or random generator runs per cell (the plan itself does one `sqrt` per job). The
extra per-view state is one byte per cell, already in `required_bytes()`.

Measured on the G4 (640x480, 64 work units per step, medians of nine, two runs, headless): first
pass about 78 ms, total 711 / 765 / 635 ms (profile 1x, 2x off-center, trace), worst step about
25 ms. Final images are identical for every focus: `19dd1b02e4cb2a8b` (profile 1x),
`572eba5730155418` (profile 2x), `9e6de6b83f243a80` (trace). Raw data:
`docs/perf/refine-shapes-g4-run1.json`, `-run2.json`, `refine-order-g4.json`. Not measured:
scanout, X uploads at every pass and any OS 9 execution.

## Progressive lines (GL-41)

After all terrain passes the lines no longer appear at once: they reveal circle-outward from the
view center, in the same 16 rings as the terrain, one pass per timer wake (`kLineRevealDelayMs` =
15 ms in `render.hpp` (30 ms at first, halved on 2026-11-02 for feel), about a quarter of a second in total; any input cancels it like any refinement).
`RenderQuality::progressive_lines` (default true) switches it; the unpaced path is the old single
line pass. Each polyline is cut once into chunks of 16 points (`LineChunk`, 8 bytes; 865 chunks and
7,785 workspace bytes for the bundled atlas); a chunk's pass is the pass of its midpoint, assigned
by the same plan as the terrain cells (`Scene::prepare_line_chunk`). No per-pass re-walk. When the
reveal has drawn every chunk, one canonical layer-order pass repairs crossings, so the settled
image is **identical** to the unpaced render for every focus and zoom (checked every pixel in
`tests/test_line_reveal.cpp`, and by the unchanged refined pins).

G4 (640x480, medians of nine, `globe-line-reveal-bench`, `docs/perf/line-reveal-g4.json`):
compute 726 ms paced against 680 ms unpaced at radius 220, 795 against 734 at radius 440 (about
+45-60 ms: preparing chunks 0.6 ms, revealed lines 88-105 ms against 44 ms, canonical repair
43-44 ms); scheduled pauses 16 x 30 = 480 ms at the 30 ms step (now 15 ms: 240 ms); wall 1,330 / 1,370 ms at 30 ms; worst step 29 / 25 ms; checksums
equal paced and unpaced. X upload cost per reveal step is not measured (GL-7 is the owner's).
Retired Codex draft: its earlier orders were removed with the circle-only decision.

## Line layer (prototype, 2026-11-05)

`RenderQuality::lines_first` (set by `quality_for`) draws the lines **first**, into a private
RGB565 layer in the job (`line_layer_`, key color `0x0001` = no line), then composites the layer
over the frame after the background clear and after every terrain pass (phase 10, one row per work
unit). The lines are on screen from the first presented frame and are never hidden by a pass. It
replaces the paced reveal (no reveal wakes, no canonical repair pass); the rasterizer and the terrain
passes are unchanged, and the settled image is pixel-identical (`tests/test_line_reveal.cpp`, the
refined pins, `globe-line-reveal-bench` mode `layer` has the same checksums as `unpaced`).
Presented frames per job: 1 (lines) + 16 (rings) + 1 (limb pass) + 1 (end).

Laptop (release, 640x480, medians): compute 26.7 ms against 12.9 ms unpaced (the extra is the 17
composite scans, about 8 ms of it) and against 23.8 ms paced, but wall 26.7 ms against 267 ms paced
(the 240 ms of reveal pauses are gone); the first frame with lines comes at 3.5 ms. **Not yet
measured on the G4 or OS 9** (maul was on Tiger/off). If the scans cost too much there, keep a
list of line pixels instead of scanning the layer. Costs 307 KB (+ coverage plane) per job at
640x480; the OS 9 heap budget is unchecked. The OS 9 port uses the same core, so it gets this too.

## The final pass: limb cells only (2026-11-06)

The 16 ring passes are no longer followed by a repaint of every cell. They are followed by one
more pass that redraws **only the cells clipped at the horizon** (any corner behind it: `limb_cell`),
in index order. Terrain fill work drops by about 47% (slices over 600 random views: 90.1 M with the
old full repaint, 48.2 M now); host `globe-line-reveal-bench` unpaced compute went from 12.9 and
21.0 ms to 10.6 and 9.7 ms. The refined pins are unchanged.

Why this is exact. Measuring what the old repaint changed (`globe-repair-diff`, 600 random views,
radius 50-1500, random center and heading, focus anywhere from 200 px outside the viewport, Map and
Relief, a single job and the production coarse + preserve-bitmap sequence) found one cause:
a clipped limb cell is a flat polygon, its unclipped neighbors are shaded triangles, and the two
rasterizers own a shared edge pixel differently, so both draw it. Shaded cells never overlap each
other (the triangle code sorts vertices, so a shared edge is the same function for both). Rings
alone are therefore order dependent at about 2% of views (a pixel or two on the limb), and the old
index-order repaint hid this by always letting the higher index win. The final image is now **defined**
as: every unclipped cell, then every clipped cell in index order (`TerrainRepaint::Reference`, drawn
by the job in that order for testing). Ring passes plus the limb pass give exactly that (0 of 600
views differ, single job and two-level), whatever the focus; the old canonical image differs from the
new one in 14 of 600 views (19 px, all at the limb), which is why `TerrainRepaint::Full` (the legacy
behavior) is kept for the bench only. `tests/test_terrain.cpp` checks the equivalence with
contrasting neighbors and five foci, and the slice count of the limb pass.

A first attempt, repainting limb cells in the same sweep as ring 15, was not exact: the shaded ring-15
cells with a higher index overwrote it. The limb pass must come after every ring.

Raw output: `docs/perf/repair-diff-host.txt`. Not yet measured on the G4 (maul was off NetBSD); the
pins should match there because the arithmetic is unchanged.

## Zoom preview (prototype, X11 only, 2026-11-06)

Zooming (wheel, buttons) changes only the radius about the view center, and an orthographic globe
zoomed about its center is the same picture scaled about that point. So while the view only zooms,
`zoom_preview` in `src/platform/x11/main.cpp` builds the moving frame by scaling the **last finished
terrain frame** (`scale_frame`, `src/render/zoom.cpp`: nearest neighbor, one column table per frame)
and drawing fresh lines on top (`RenderQuality::lines_only`), instead of rendering plain vectors.
Terrain stays on screen while zooming. The cache is the settled frame copied by the final refinement
job just before its lines are stamped (`RefinementJob::set_snapshot`), keyed by orientation, window
size and look; it is used for factors 0.25 to 4, with ocean-filled holes at the rim (see below). Always scaled from the cache,
never from the previous preview. When the wheel stops the refinement starts over the preview
(`preserve_bitmap`, no clear), so it sharpens the picture instead of rebuilding it; the final pixels
are the same as from scratch (`tests/test_zoom.cpp` for factors 1.1, 1.5 and 0.8). Not done: drag,
double-click zoom (it recenters), OS 9.

Zooming out of a frame whose disc overflowed the window (any zoomed-in settled frame) has nothing to
show at the rim. `scale_frame` returns the number of such hole pixels inside the new disc and fills them
with plain ocean; the preview is used anyway so the picture stays up until the refinement starts (the
first version refused any hole, the second more than 35% of the disc). It is used for factors 0.25 to 4
(`kZoomPreviewMin/Max`) relative to the cache, then the plain render takes over. `--no-zoom-preview` turns it off; the stats show
`zoom_previews`; `--finish-refinement` makes a script run each refinement to its end.

Laptop, 640x480: a preview frame costs about 0.95 ms against 0.85 ms for the plain vector frame, so
the gain here is that the terrain stays, not speed. G4 cost not measured (maul is on Tiger, not
NetBSD). Costs one extra framebuffer for the cache.

## Verification

`tests/test_terrain.cpp`, `test_app.cpp` and `test_refine.py` compare every pixel for several foci
(center, corners, off-viewport, off-center) and two zoom levels with contrasting neighbors and limb
clipping, check each cell's pass against an independent oracle (the cell's position computed from
latitude and longitude), check the non-finite focus, cancellation at a pass boundary, and that the
`order` verb is unknown.
