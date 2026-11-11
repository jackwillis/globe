# ADR 0004 — The Map look does not redraw ocean the cleared disc already shows

**Status:** proposed, 2026-11-09 (issue [#67](https://github.com/jackwillis/globe/issues/67), under
[#64](https://github.com/jackwillis/globe/issues/64)). The settled pixels do not change, so this is
an engineering decision for the owner to accept or reject, not a visual one.

## Context

In the Map look the ocean disc is cleared to `kMapOcean`, and every ocean cell is then painted
`kMapOcean` again: `landclass_color()` returns it for any negative height, whatever the land class,
and the smooth draw clamps an ocean cell's corner values to the ocean part of the palette. About
half of a globe's visible cells are such cells. They cost as much to rasterize as land (setup
dominates, cells being about 4 px wide).

## Decision

When a terrain job clears the disc itself, draws the Map look with smooth shading, and its ocean color
is `kMapOcean` (`RenderQuality::skip_ocean`, on by default), `TerrainScene::assign_pass` leaves a cell
out of every pass (its pass is 255) if `TerrainScene::disc_ocean_cell()` holds:

1. its value is below sea level and the palette has at least one ocean entry (`first_land_ != 0`), so
   the smooth draw paints `kMapOcean` and nothing else;
2. all four corners are in front of the horizon by more than `kDiscMargin` (1e-3 of the unit sphere).

## Why the settled frame is identical

- **Same color.** The cell would paint `kMapOcean`; the cleared disc already holds `kMapOcean` at every
  pixel the cell can reach. A cell with all corners at camera-space `z > 1e-3` has them at least
  `radius * z^2 / 2` inside the disc rim (about 5e-7 radius), the disc is convex, and the cell's
  triangles are chords, so every pixel center they cover lies inside the disc by far more than
  rounding error; `fill_disc` clears the pixel centers inside the circle.
- **No order effects.** An unclipped cell never overlaps a neighbour: triangles sharing an edge get
  bit-identical edge positions (`edge_x`), so a pixel has one owner whatever the draw order (this is
  what lets the ring order vary, ADR 0002 D2). A skipped cell therefore cannot have been covering a
  pixel another cell owns. Cells clipped at the horizon are flat polygons that may overlap
  neighbours, and are drawn last in index order; condition 2 keeps every one of them, ocean or not.
- **Not when painting over something.** A job with `preserve_bitmap` (zoom preview, later refinement
  levels) draws over old pixels that must be overwritten, so it never skips. Flat shading
  (`smooth_terrain == false`), where adjacent flat polygons are not proven disjoint, and the Relief look,
  whose ocean is not one flat color, never skip either.

## Evidence

`tests/test_terrain.cpp::test_ocean_skip` renders the same jobs with `skip_ocean` off and on and
compares every pixel: 600 random views of the real 1 degree elevation and land classes (all
hemispheres and rolls, two sizes, radii 15 to 700, random foci, near-pole views) plus 900 views of three
coarse grids with large cells, where horizon-clipped cells are big. It also checks that about half the
slices disappear and that the preserve, flat and Relief paths skip nothing. Loosening the margin to
-0.3 (skipping cells that touch the horizon) is caught by it.

Host numbers (x86-64, GCC 13, -O2): [docs/perf/settle-20261109.md](perf/settle-20261109.md). G4
numbers are not yet measured.

## Consequences

- A settled Map frame is bit-identical; so are the checksums of `globe-line-layer-bench`.
- Intermediate frames differ: ocean cells no longer appear as ring passes complete, because the disc
  already shows ocean. The visible effect is land filling in over a finished sea. Progress is counted
  over the remaining cells, so the progress bar still runs 0 to 1.
- If the Map look ever gets an ocean that is not one flat color (depth shading, a texture), or the
  disc is cleared to a different color than `kMapOcean`, `skip_ocean` must be off for it; the job checks
  `palette_.ocean == kMapOcean` and the style, and the test fails if the equivalence breaks.
