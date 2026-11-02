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

## Addendum, 2026-11-01 (behavior rules for every front end; compression direction)

### Behavior rules (owner, binding on X11 and the OS 9 front end)

These live in the portable core or in a shared platform rule so both front ends behave the same.

1. **A click without dragging changes nothing visible.** A pointer press alone must not leave the
   settled stage, bump the render generation or schedule a re-render. The Moving stage starts only
   when the pointer has moved beyond the drag slop. A release without movement leaves the model
   untouched (country picking still uses that release). Double-click zoom still changes the camera.
   Review finding, 2026-11-01: `update()` in `src/core/app.cpp` currently treats every
   `PointerDown` as input (stage Moving, `generation` bumped, settle wake scheduled), so a plain
   click discards a finished refine and re-renders it about 150 ms later. To be fixed in the
   core with a test that clicks without moving and asserts stage, generation and effects are
   unchanged. Not yet fixed.
2. **Moving or exposing a window never re-renders.** A window move, an expose or a repaint request
   resends the pixels already in the framebuffer. Only a resize, a camera change or a data change
   starts a new render or restarts refinement. X11 already ignores `ConfigureNotify` when the size
   is unchanged; the suspected remaining cause is `Expose` becoming `Repaint`, which currently asks
   for a render. Unverified on the G4. The OS 9 front end must do the same for update events
   and window drags.

### Compression direction (design only; nothing implemented, no ratios measured)

- Compression serves the floppy and downloads, not the draw loop. Decode once at load, or per tile.
  Decoders must be small portable C++ with no allocation in hot paths and an output size bounded
  by the header (corrupt data must not be able to exhaust memory).
- Vector layers (coastline, borders, country polygons): delta from the previous quantized point as
  a variable-length integer; expected roughly 2x smaller (inference).
- Elevation grids: predict each cell from its neighbors, store the error, then entropy-code with
  DEFLATE (a tinf-style inflate, about 2 KB of code, 32 KB window) or a small adaptive range coder.
  Higher-resolution packs are independent compressed tiles (for example 16 by 16 degrees) decoded on
  demand into a small cache, which keeps OS 9 memory bounded.
- Flags (ice, lakes): run-length coding. Places: shared string table with delta-coded positions.
- Not planned: LZMA (large dictionary, heavy decoder), JPEG/PNG (only useful for the dropped
  satellite imagery).
- The small embedded files keep the raw zero-copy v1 formats; compressed data is a separate
  container for the optional large packs, with the CRC over the stored bytes and the output size
  checked against the header.
- Before choosing, measure on real data: derive a 0.25 degree grid from the ETOPO source and
  compare plain DEFLATE, predictor plus DEFLATE and a range coder, plus delta-varint on the atlas
  points. Record the ratios here and in the floppy size report.

## Addendum, 2026-11-01 (refine profile on the G4: the earlier guess was wrong)

Measured by a profiling program on branch `claude/refine-profile` (commit `d30f16c`, based on `v2`
at `b1dbaaa`, unmerged; `src/platform/bench/refine_profile.cpp`, `docs/perf/refine-profile-g4.json`).
G4, 640x480, 1x zoom, median of 9 runs; host and G4 frame checksums match.

- The real refine job takes **786 ms** with terrain and **456 ms** without, so terrain adds about
  330 ms. At 2x zoom the job is 1014 ms and lines-only 709 ms.
- **The line layers are the largest cost.** Drawn in one pass they cost 28 ms, but the refine job
  walks every polyline again for each of its 30 bands, about 16 times that. This replaces the
  earlier inference that corner projection dominated.
- Terrain phases sum to about 215 ms: project all corners 18 ms, bound all cells 26 ms, scan all
  cells once per band 47 ms, fill about 123 ms (by subtraction, not measured directly). About 115 ms
  of the job is not covered by any phase (inferred: per-work-unit stepping overhead, not measured).
- 32,098 of 64,800 cells are culled; 152,080 pixels are filled at 1x.
- A hemisphere pre-test skips half of the corners but saves under 1 ms on the G4. **Corner culling
  is not worth doing.**

Fix order, ranked by the profile: (1) draw the line layers once per refine, not once per band
(clip each polyline to the band, or cache per-band segment lists); (2) make the cell scan visit
only cells overlapping the current band (or drop bands in favor of the cell dissolve, which has no
per-band scan); (3) make the per-cell fill cheaper (shaded fill, or skip fills that change no pixel).
The dissolve order and the layered stack above should be built with these in mind: lines are a
separate top layer drawn once, not interleaved with terrain passes.

## Addendum, 2026-11-01 (data policy and a separate maps app)

Owner decisions after discussing streaming OpenStreetMap data. Design only; nothing implemented.

- **Terrain is baked into the globe app, compressed.** It changes rarely, is small once compressed,
  and a baked layer cannot be mismatched or corrupt. The globe stays offline and floppy-sized.
- **Only the vector layers are updatable:** borders, countries, places and similar data ship as
  separate versioned pack files that are imported into a data folder (local import first, per D2b).
  Every pack carries a format version and a checksum. An app rejects a pack it does not understand
  and keeps running on its baked data. The terrain build must be reproducible so its checksum is
  stable. Optional higher-resolution terrain packs remain an optional extra with a size report.
- **No relay.** The owner does not want a relay machine between the apps and any server. The
  earlier relay idea and an in-app download manager in the globe are dropped.
- **Streets and other detailed maps belong to a separate maps application,** not the globe. It has
  its own projection (flat or local, not the globe's orthographic view), tile cache, zoom levels and
  network code. The globe and the maps app live in one repository, share libraries (projection,
  raster, pack decoders) and the pack formats, and can hand each other a place as latitude and
  longitude on the command line. The globe could open a regional pack the maps app fetched.
- **Networking by platform (from memory, to verify before relying on it):** the NetBSD/X11 maps app
  can fetch over HTTPS directly with the system TLS library. The public OpenStreetMap raster tile
  servers allow only light, cached use with attribution and a proper User-Agent, not bulk
  downloading, so the app must cache and fetch only what is on screen; raster tiles need a PNG
  decoder. Static vector-tile archives fetched by HTTP range requests need no server logic, but which
  public ones exist and their terms are unchecked. OpenStreetMap data is ODbL: attribution, and
  share-alike for derived databases, so a built pack carries the license text.
- **OS 9:** the owner's Mac cannot realistically do modern TLS, so a networked maps app there is
  probably not possible without a relay, which is rejected. OS 9 maps is offline regional packs
  built on a modern machine and imported, or not built.
- **Rendering streets** (hundreds of thousands of segments per city) needs tile culling and a zoom
  threshold, and is unmeasured on the G4.

## Addendum, 2026-11-01 (two releases)

Owner decision: the globe ships as **two releases, a floppy edition and a full edition**. Not a
configuration matrix.

- **Floppy:** must fit a 1.44 MB disk (the D8 size report decides what fits). Baked compressed
  terrain at the coarse resolution, 110m vectors, the OS 9 front end.
- **Full:** everything else: finer baked terrain or optional packs, extra layers, larger vector
  packs, the NetBSD/X11 front end.
- Both editions use the same source and the same pack formats; the difference is which data and
  front ends are built in, selected at build time. Exactly what goes in each is open until the
  floppy size report and the compression measurements exist.

## Measurement, 2026-11-01 (Codex: compression prototype, no runtime codec chosen)

The requested quarter-degree ETOPO pack was actually built: 1,037,352 bytes, SHA-256
`b6d9c8a4ae4cf66c67d6c247057c15e23416f6341a734965b33e07cf859b2241`.
For its 1,036,800 cell bytes, zlib-wrapped DEFLATE level 9 gives 500,301 bytes; gradient predictor
plus DEFLATE 428,179; adaptive order-0 byte range coding 804,716; predictor plus range coding
392,593. Each decode and predictor reversal exactly reproduces its input. Sizes exclude the
552-byte GELV metadata and a future container. Atlas delta/zigzag varints give 26,706 point bytes
versus 32,480 (whole-file arithmetic 29,294 versus 35,068). See `docs/perf/compression.md` and raw
JSON for method and limits. These are modern Python measurements, with no target decoder,
G4 decode timing or runtime format change; the result does not choose the eventual codec.

## Addendum, 2026-11-01 (fetch inside the app; supersedes the "no networking in the globe" rule)

Owner answers: a **fetch tool inside the app** is wanted ("if it could work"); it is in scope for
v2; and TLS on OS 9 is to be solved, with the sibling `weather` project as the starting point.

- **Supersedes** the earlier statements that the globe has no networking code and no in-app
  download manager. The no-relay decision stands. The maps app remains a separate, later project.
- **Correction to the earlier claim that OS 9 cannot realistically do modern TLS.** Reading
  `../weather`: it vendors BearSSL (MIT) in `third_party/bearssl`, generates trust anchors from a
  pinned PEM (ISRG Root X1/X2, HARICA TLS RSA Root CA 2021), has DNS, HTTP framing and fetch
  deadlines, and its README reports live verified-TLS fetches passing on "Mac OS 9 under Tiger
  Classic" (an emulated environment, not necessarily real G4 OS 9 hardware; not independently
  checked here). It requires a unique TLS seed provisioned separately and persisted safely in
  Preferences (`docs/OS9-HARDENING-PLAN.md`: HMAC_DRBG seeded from provisioned material, never
  truncate the only valid seed in place, refuse TLS if the seed is missing or short).
- **Consequences to plan:** the fetch module needs its own library (no dependence on the globe's
  portable core), a platform split (NetBSD sockets plus OS TLS or BearSSL; Open Transport on OS 9),
  trust-anchor and seed handling, a strict pack validator in front of the importer (version,
  checksum, size caps) so a bad download can never break the baked data, and a size budget check:
  BearSSL plus trust anchors count against the floppy edition and may push networking into the
  full edition only. How to reuse weather's code (vendor, copy with provenance, or a shared
  repository) is undecided; check licenses (weather is the owner's project; BearSSL is MIT).
- **Not decided:** which server hosts our packs (OpenStreetMap-derived data must be built and
  hosted by the owner, since the public servers are not for bulk use), whether the floppy edition
  can afford the code, the update policy and the user interface.

## Addendum, 2026-11-01 (compression experiment, measured)

Branch `claude/compression-experiment`, commit `57eb518` (unmerged; `tools/compression-experiment/run.py`,
`docs/perf/compression-experiment.md`). The source NetCDF was readable; the derived 1 degree grid
matches the committed `.gelv` byte for byte. Elevation sizes are cell payload only (header excluded).

| Data | Raw | Best measured | Notes |
| --- | --- | --- | --- |
| Atlas points | 32,480 | delta-varint 26,736; plus deflate ~22,900; plus lzma ~20,000 | |
| 1 degree grid | 64,800 | Paeth + arithmetic 29,140 (2.22x); avg + arithmetic 29,661; Paeth + deflate 33,895; plain deflate 41,238 | |
| 0.25 degree grid | 1,036,800 | avg + arithmetic 342,041 (3.03x); avg + deflate 401,287 (2.58x) | |

16-degree tiling overhead: +17% to +73% at 1 degree, +2% to +12% at 0.25 degree (worst for the
arithmetic coder, which relearns its statistics per tile). Recommendation of the experiment for the
floppy edition: average predictor plus a small adaptive range coder, untiled; inference (not
measured): about 60 lines of decoder against 250+ for inflate and 600+ for lzma. Delta-varint alone
(about 15 lines) is the cheap first step for atlas points. G4 decode speed was not measured.

## Addendum, 2026-11-02 (product: an atlas; looks and overlays)

Owner decisions after reviewing terrain looks.

- **The product is an atlas** (information first: climate and biome facts in popups, country facts,
  search), not a globe toy. The floppy edition will always be a struggle over what to cut; that is
  accepted, and the size report (`docs/floppy-size.md`) is how each cut is decided.
- **Terrain looks** are alternatives for one terrain-color step, each a table over the same cells
  (no blending, no stacked duplicate fills): Hypsometric (built), Cel-shaded (built), **Natural**
  (land-class grid by elevation band: biome picks the hue family, elevation the shade; needs a
  class grid, e.g. a Koeppen-style classification, license and size unchecked), **Thematic**
  (ColorBrewer-style classes on height, deliberately unnatural; good for low color depth),
  **Map** (Google-like flat pale land, blue water, light borders, labels; needs no new data),
  **Satellite-like** (experiment only: a baked color grid sampled from imagery at build time and drawn
  as ordinary cells; no texture mapping; imagery license and floppy fit unchecked), and **Classic
  outline** (black on white, 1-bit capable; GL-17).
- **Default look: Map** for the floppy edition (needs no extra data); **Natural** is the full
  edition's showpiece. The floppy ships at most three looks: Map, Hypsometric and Classic outline.
- **Overlays** stack on any look: coastlines, borders, graticule, rivers and lakes (Natural Earth
  vectors, cheap, a v0.1 candidate if small), relief shading, climate layers (full edition),
  day/night terminator, labels and cities. Rivers and lakes are in scope for v0.1 if they stay small.
- **Antialiasing:** Refine stage only, never while moving, off on 1-bit and 8-bit displays; lines and
  the globe silhouette, not cell fills; a new pin set for the refined profile; cost on the G4 to be
  measured first (expected to be a few times the plain 28 ms line pass; a guess).
- **Satellite-like look: test it,** as a measured experiment (size, look, license), not a commitment.
- **Internet on the floppy: not in v0.1.** Measured by Codex (link-only experiment, not a working
  fetch): BearSSL plus three trust anchors adds about 104,206 bytes to the OS 9 data fork, against
  930,436 bytes of floppy payload headroom, so size alone does not forbid it. The blockers are
  elsewhere: weather requires a unique, safely persisted TLS seed per install, and a floppy is copied
  between machines, so a shipped seed is unacceptable and a first-run seed story is undesigned; HTTP,
  DNS and Open Transport code are not in that number. Fetch ships in the full edition first; the
  floppy follows once the seed story and the full size are known. See `docs/fetch-design.md`.
