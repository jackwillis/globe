# Refinement order (GL-19 / GL-26, concluded 2026-11-02)

The settled terrain refines **outward from the last touchpoint** in one of three shapes. The
experiment with rows, center-out bands and a Gaussian dissolve is over: the owner found the dissolve
odd and asked for the top-down and row orders to go. They are removed; their raw measurements stay
in `docs/perf/refine-order-*.json` for the record.

| Order | Shape | Distance from the focus |
| --- | --- | --- |
| `circle` (default) | expanding circles | Euclidean screen distance |
| `rectangle` | expanding rectangles shaped like the viewport, reaching its edges together | `max(|dx| / width, |dy| / height)` |
| `globe` | rings of equal great-circle angle on the sphere | angle between the cell and the focus point on the sphere |

Compare them in the X11 program with the `O` key (rectangle, circle, globe), or in any replay with
a timestamped trace line, handled by the same model path in every front end:

```
order 8000 rectangle
order 8000 circle
order 8000 globe
```

The parser is strict: `rows`, `center`, `dissolve` and `square` are unknown names.

## Focus

The refinement spreads from **halfway between the recorded touchpoint and the view center**
(`quality_for` in `src/core/app.cpp`; the model still stores the raw touchpoint). The owner asked for
this so refinement leans toward what was touched without leaving the middle of the picture for last.
Final pixels are unaffected, so no pin moves. The paragraph below describes the recorded touchpoint.

The focus is model state, never a clock read: a click or drag release records the last finite
pointer coordinate (including motion inside the drag slop), a double click records the resulting
view center, and the initial state, a resize and a non-finite value use the viewport center. A
plain click still leaves the visible frame, render generation and wake requests unchanged; it only
records the focus for future work. For `globe`, a focus outside the globe's disc falls back to the
view center, because there is no point of the sphere under it.

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

- **circle:** squared distance of the cell's projected center (mean of its four corners) against 15
  squared thresholds, equal-width steps from 0 to the farthest point of the disc.
- **rectangle:** `max(|dx| * height, |dy| * width)` against 15 thresholds scaled by
  `width * height`, so there is no division. Steps run from 0 to the largest value over the four
  viewport corners seen from the focus, so with a centered focus the left/right and top/bottom
  edges are reached in the same pass.
- **globe:** the mean of the four corner dot products with the focus unit vector against 15
  cosines of `k * 11.25` degrees. The cosines are exact literals generated offline, so every
  platform assigns the same pass whatever its libm.

No `sqrt`, `log`, `acos`, division, clock or random generator runs per cell (the plan itself does one
`sqrt` per job). The extra per-view state is one byte per cell, already in `required_bytes()`.

## Measured on the G4

`globe-refine-order --repeat 9` (`src/platform/bench/refine_order.cpp`): 640x480, 64 work units per
step, three identical views, headless, locally cross-built NetBSD/macppc `-O2 -static`, new
`/tmp` names, medians of nine, two complete runs (`docs/perf/refine-shapes-g4-run1.json`,
`-run2.json`). Timing starts after the initial outline frame and job construction and includes
projection, bounds, assignment, clearing, terrain, scanning, continuation overhead and the final
lines; it excludes packing, uploads and scanout. Wall-clock interruptions count. "First pass" is the
first completed pass, not the first changed pixel.

| View | Order | First pass ms | Total ms (run 1 / run 2) | Worst step ms (run 1 / run 2) |
| --- | --- | ---: | ---: | ---: |
| profile 1x | rectangle | 78 | 791 / 792 | 86 / 101 |
| profile 1x | circle | 78 | 711 / 711 | 25 / 25 |
| profile 1x | globe | 83-84 | 793 / 793 | 28 / 25 |
| profile 2x, off-center | rectangle | 77 | 707 / 706 | 25 / 31 |
| profile 2x, off-center | circle | 78 | 765 / 766 | 24 / 25 |
| profile 2x, off-center | globe | 86 | 765 / 759 | 24 / 25 |
| trace final view | rectangle | 78 | 723 / 723 | 30 / 25 |
| trace final view | circle | 77 | 635 / 635 | 24 / 25 |
| trace final view | globe | 83 | 725 / 734 | 24 / 25 |

For comparison, the retired orders in `refine-order-g4.json` (same views): rows 693 / 687 / 644 ms
total, center-out 673 / 700 / 644, dissolve 796 / 782 / 732. Against the retired top-down rows:
**circle is within about 3 percent at 1x and in the trace view, and about 11 percent slower at 2x**;
rectangle and globe are about 11-14 percent slower than circle at 1x and in the trace view, and
equal or faster at 2x. The reason for the differences is not established (the final full repaint is common to all; cache
behavior across cell orders is a guess). The first order measured in each view (rectangle) shows a
cold-start worst step of 86-101 ms at 1x; the retired rows order, also measured first, showed 71 ms
for the same reason, so read that cell as warm-up, not as a property of the shape.

**Final images are identical** across all orders on host and G4 for every view:
`19dd1b02e4cb2a8b` (profile 1x), `572eba5730155418` (profile 2x) and `9e6de6b83f243a80` (trace), the
same hashes the retired row, center-out and dissolve orders produced. The sampled refined pin in
`tests/golden/refine-checksum.txt`, the app replay hash and the outline goldens did not change.

Not measured: scanout, X uploads at every pass (each completed pass reports a ready frame), and any
OS 9 execution. This is headless evidence only; whether the circle default is the right feel is the
owner's hands-on call (`O` key).

## Verification

Tests (`tests/test_terrain.cpp`, `test_app.cpp`, `test_refine.py`) compare every pixel for several
foci (center, corners, off-viewport, off-center), two zoom levels, contrasting neighbors and limb
clipping, for all three orders; check each cell's pass against an independent oracle (the cell's
position computed from latitude and longitude, not from the production projection); check the
globe fallback, non-finite foci, and on a 160x120 viewport that the rectangle reaches the same
fraction of each axis in the same pass; check cancellation at a pass boundary; and replay each
order through the benchmark against the refined pin. Mutation checks and the build matrix are in the
commit history of the change that introduced this file.
