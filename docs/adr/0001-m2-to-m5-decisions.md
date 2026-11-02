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
