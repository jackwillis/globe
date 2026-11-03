# Refinement order (GL-19 / GL-26, concluded 2026-11-02)

The settled terrain refines **circle-outward from the view center**. The experiment is over: rows,
center-out bands, a Gaussian dissolve, rectangles and globe rings were all tried; the owner chose
circle, which was also the fastest of the shaped orders on the G4 (711 / 765 / 635 ms at 1x / 2x /
trace against globe 793 / 765 / 725 and rectangle 791 / 707 / 723). The other orders, the `order`
trace verb and the X11 `O` key are removed; their measurements stay in `docs/perf/refine-*.json`.
The focus is always the view center (`quality_for` in `src/core/app.cpp`): no touch state is
kept, so clicks and drags cannot affect it. Final pixels never depend on the focus.

## How a refinement runs

With terrain, one job paints the whole frame in 16 passes:

1. Project the cell corners and bound each cell's 16-row slices (as before).
2. The same loop assigns each visible cell a pass byte (255 for hidden cells) from a **per-view
   plan** computed once per job: `make_pass_plan` in `src/render/terrain.cpp`, the data in
   `include/globe/refine_order.hpp`.
3. Pass k clears nothing and draws the cells whose byte is k, with the existing rasterizer in
   16-row clip slices (at most four slices or the work-unit budget per continuation).
4. The last pass repaints **every** visible cell in canonical index order. This costs a final full
   fill, and it keeps today's edge ownership where clipped limb polygons overlap, so the settled
   image is identical for every order and every focus.
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

## Verification

`tests/test_terrain.cpp`, `test_app.cpp` and `test_refine.py` compare every pixel for several foci
(center, corners, off-viewport, off-center) and two zoom levels with contrasting neighbors and limb
clipping, check each cell's pass against an independent oracle (the cell's position computed from
latitude and longitude), check the non-finite focus, cancellation at a pass boundary, and that the
`order` verb is unknown.
