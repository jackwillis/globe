# ADR 0001: Decisions for staged refinement, terrain, data and UI

Status: accepted, 2026-11-01 (owner: Jack Willis). Context: `docs/v2-design.md` sections 4 to 7 and
the M1 hands-on results in `docs/perf/README.md`.

## Decisions

| # | Question | Decision |
| --- | --- | --- |
| D1 | How does the core ask for "settle" timing? | `update()` returns a wake request as data (`Effects::wake_after_ms`); the platform arms a one-shot timer (select timeout on X11, the sleep argument of `WaitNextEvent` on OS 9) and sends a `Tick{time_ms}` message. No periodic polling; idle CPU stays zero; traces replay. |
| D2 | Elevation source and resolution | One coarse global elevation grid colors land and ocean depth. **1 degree first** (about 65 KB at one byte per cell). Higher resolutions are optional data packs, see D2b. Perlin noise is not used. |
| D2b | How do higher resolutions reach the app? | **Local import first, network later.** A modern-machine tool builds packs (same strict format, any resolution) from NOAA's public data; the user copies them to the target and imports. In-app download is M4, from independent hosts, through the design doc's provider pipeline. The elevation format is multi-resolution capable from day one. The project hosts nothing. |
| D3 | How do the data formats grow? | **Separate small versioned files, one per concern** (elevation grid, country polygons, places), each with the atlas's pattern: fixed little-endian, versioned header, CRC-32, strict bounded zero-copy decoder, builder in `tools/`, format doc, cross-language tests, mutation tests. A build loads only what it needs. |
| D4 | Toolbar | Rename `Key` to `Command`; add a pure `toolbar_state(Model)` (which buttons exist, are on, are enabled). X11 gets a separate utility window with hand-drawn buttons first; OS 9 later uses a native floating palette. Align-north becomes a button. |
| D5 | Popups (cities, countries) | After terrain. Countries first (Natural Earth polygons, public domain: click to identify), then cities (Wikidata snapshot, CC0). The core produces an `InfoCard` (title and lines); each platform shows it in a native scrolling window. |
| D6 | Palette | Hypsometric terrain colors from a 256-entry color table, **black borders** on top by default. Further themes are a later option. (Decided by Claude as a consequence of D2; the owner asked for terrain colors and black borders.) |
| D7 | Mac OS 9 "proof of life" | A window showing the globe with drag, zoom, double-click and keys, built with Retro68 in the container (as `../weather`), drawing through `pixel_format`, event loop with the wake-request sleep. No toolbar, no downloads. Hardware acceptance by the owner. |
| D8 | Floppy (1.44 MB) | An experiment with a size report: a designated `floppy` build (app, atlas, 1-degree elevation) gets a CI size report and a budget; it does not block features in the normal edition. |

## Consequences and order of work

1. `Key` to `Command` rename and `toolbar_state` (core only; small).
2. `Effects::wake_after_ms`, `Tick`, the Settle/Refine stages and progressive drawing in bands, with
   cancellation when new input arrives (D1). Staged refinement is the prerequisite for terrain.
3. Elevation: format (GELV), builder, decoder with the full validation and mutation treatment (D2,
   D3). The source file format, license, size and hash must be verified when the builder is written;
   the builder runs on a modern machine and may use extra tooling there, but the runtime stays free
   of it.
4. Terrain renderer: forward-projected colored cells during Refine (not per-pixel inverse
   projection, which costs microseconds per pixel on the G4), black borders, measured on the G4
   before and after (D2, D6).
5. Data pack import (D2b), then network acquisition as M4.
6. Toolbar window on X11 (D4); country polygons, picking and popups (D5); places.
7. Mac OS 9 proof of life (D7) can proceed in parallel once the core is stable.
8. Floppy size report (D8) can be added at any time.

## What this does not decide

The exact elevation quantization (a per-file lookup table is proposed so one format spans ocean
depths and mountain heights), the Settle delay (design doc suggests 120 to 200 ms; tune on the G4),
and the popup layout. Each gets decided when implemented, with measurements.

## Addendum, 2026-11-01 (after hands-on use on the G4)

- The owner confirmed the north-up turntable drag feels right, and that the graticule is good.
- The coastline toggle was removed as a user feature: coastlines are always drawn. The graticule and
  borders toggles remain.
- **Cursor feedback is part of the toolbar milestone (D4), not earlier.** Plan: a pure
  `cursor_for(Model, pointer)` in the core returning an intent (open hand over the globe, closed hand
  while dragging, arrow elsewhere), used in both drag modes, next to `toolbar_state`. Each platform
  maps the intent to a native cursor. On Mac OS 9 the Appearance Manager provides theme open- and
  closed-hand cursors (to be confirmed against Retro68's own headers when that work starts, without
  copying Apple files); the X11 core cursor font has no open/closed hand, so X11 needs a fallback
  such as `XC_hand2` and `XC_fleur`, or the cursor theme through Xcursor if present on the target.

## Addendum, 2026-11-01 (terrain look, after the tip4 demo)

Owner feedback on the first terrain build (`globe-x11-tip4`, committed tip `9c62abc`) and the
decisions that followed. This amends D6; nothing below is implemented yet unless it says so.

- **Feedback:** the 1-degree cells look very chunky; there are no Great Lakes (the 110m coastline
  has no lakes and a lake is one cell or less at 1 degree); deserts do not look like deserts
  (elevation alone cannot tell the Sahara from the Amazon); land should be gray while the globe
  is being moved.
- **Layered refinement instead of modes.** Terrain is a stack of refine steps that composite, each
  a bounded, cancellable pass: (1) *base*, flat gray land and a flat ocean color, cheap enough for
  the Moving stage; (2) *terrain color*, the elevation palette; (3) *relief*, optional slope
  shading; (4) *lines*, borders, coastlines, graticule; (5) *labels and popups*, later. A settled
  frame must be identical however the steps were reached, so tests pin the final image. Each step
  needs its own G4 timing. How the flat base is filled during motion is open: the coastlines are
  polylines, not polygons, so the candidate is a coarse decimated elevation grid filled flat.
- **Styles are palettes for the terrain-color step**, not render paths: a runtime-selectable
  256-entry table, chosen by a toolbar button and a `CycleStyle` command. Hypsometric is the
  default; cel-shaded (flat height classes, optionally biased by latitude) is the second. Layer
  visibility is a separate toolbar toggle, like graticule and borders.
- **Smoothing without more data.** Interpolate a palette index across each cell (shaded quads) so
  the 1-degree grid reads as soft terrain. `fill_triangle_shaded` exists (branch
  `claude/smooth-quad`, not merged, not timed on the G4); wiring into terrain is blocked on a G4
  timing of the settled frame. Finer packs are an opt-in extra with a size report, because 0.5 degree
  is about 260 KB and 0.25 degree about 1 MB raw (arithmetic, not measured) against a 1.44 MB floppy.
- **Antarctica is always ice white.** First a latitude rule (land south of about -60 degrees) as a
  shared override before the palette lookup; reserve an ice-flag bit in GELV; later fill it from
  Natural Earth glaciated areas. Source recorded in `data/SOURCES.md` (version 4.1.0; 50m is 216,484
  bytes, 377 records); build-time input only, never shipped.
- **Great Lakes:** add the Natural Earth lakes polygons as a build-time input that marks water
  cells in the pack, and optionally draw lake outlines. Source not yet checked (size, hash, license).
- **Deserts:** needs more than elevation: latitude tinting first, or a coarse land-cover grid as a
  further small versioned file. Not decided.
- **Satellite imagery: not planned.** A 1024x512 RGB565 texture is about 1 MB, and mapping it
  means inverse projection per pixel (`sqrt`, `atan2`: slow on the G4; several hundred ms per frame
  by arithmetic, not measured). If wanted later it is an optional imported pack, never on the
  floppy, and its license must be confirmed first.
- **Not decided here:** how much of the layer split ships before OS 9 work starts.

## Addendum, 2026-11-01 (refinement order; shaded-fill timing)

- **Refinement order: center-biased dissolve over cells, not a top-to-bottom sweep.** Each terrain
  cell gets a fixed pseudo-random number from a hash of its index (no clock or RNG state, so replays
  and host/G4 checksums stay deterministic). At view time a visible cell's appear-time is roughly
  `random + w * distance_from_center`, bucketed into about 16 passes; pass k draws the cells in
  bucket k as ordinary whole-cell fills, over the flat base, and cancellation stops between
  passes. `w` is one tunable weight, to be chosen on the G4. The bucket is computed in the pass
  that already culls hidden cells, so there is no sort. The final image must not depend on the
  order, which holds only while cells do not overlap and shared edges leave no gaps; tests must pin
  this. It does not make refinement faster, only nicer to watch. A single-pixel dissolve is
  rejected (it defeats span fills). Fallbacks: center-out row bands (cheap, keeps the current band
  code) or coarser blocks of cells if the speckle looks noisy. Replaces the row-band structure in
  `terrain.cpp`, so it waits for the current Codex pass to land.
- **Shaded fill timing (measured, G4, 640x480, median of 30; commit `0ecc5fd` on branch
  `claude/smooth-quad`, unmerged):** 1x zoom (~3.8 px per cell) flat 107.5 ms vs shaded 185.6 ms; 2x
  zoom flat 106.4 ms vs shaded 99.4 ms; host 2.4/3.8 ms and 2.0/2.1 ms; checksums match across host and
  G4. Against the refine frame of about 0.58 s (Codex's figure, not re-measured here) shading adds
  roughly 78 ms at 1x and nothing at 2x, so smoothing is affordable. Not measured: the shaded fill
  inside the real refine path, cancellation, display submission. The shaded path skips cells with a
  corner behind the horizon, so limb pixels differ slightly from the flat path.
- **Where the refine time goes (inference, profile in progress):** the flat fill is about 107 ms of
  roughly 580 ms, so the larger share is probably projecting all ~65,000 corners and per-cell
  setup. Fix by measurement, not by guess; the profile result will be recorded here.
