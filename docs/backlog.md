# Globe — backlog and priorities

Reconciled 2026-11-02; updated after GL-19/26 experiments and the GL-40 proposal on `main`. The original backlog (written from `386f4c6`, so it
could not see the merged side branches) is kept in structure; statuses below are checked against the
repo, not assumed. Decisions live in `docs/adr/0001-m2-to-m5-decisions.md`; rules and history in
`docs/handoff.md` and `docs/project-notes.md`.

- **Priorities:** P0 prove it runs and protect existing work; P1 make v0.1 fast and shippable; P2
  surfaces, passes and the Classic style; P3 features for v0.2 and later.
- **Sizes** are relative guesses: S under a day, M a few days, L a week or more.
- **Owners:** Jack = needs the G4, the GitHub remote or a decision. Agent = code or docs Claude or
  Codex can do locally.
- **Status:** done / partial / open. Nothing here claims a result on OS 9 hardware or in CI; neither
  has run.

Phase A (P0) gates everything. v0.1 (P1) ships on today's architecture; the refactor (P2) starts
after it, so a slip there cannot hold up the release.

## P0: prove it runs and protect the work

| ID | Ticket | Size | Owner | Status | Notes / done when |
| --- | --- | --- | --- | --- | --- |
| GL-1 | Run the existing OS 9 build on the G4 | S | Jack | open | The app is now `GlobeFloppy` (`GlobeFloppy.bin` MacBinary, both forks; `GlobeFloppy-1440.dsk` image), built with `tools/build-edition.sh floppy`. Launches; drag, double-click zoom, keys and cursors work; idle quiet; window moves and updates do not re-render; results into `docs/os9-proof.md` |
| GL-2 | Get the work onto GitHub | S | Jack | changed | The five side branches are merged into `main` and deleted, so nothing to push but `main` itself (GitHub has an older commit). No Claude push: the owner pushes |
| GL-3 | Merge `claude/click-rules` | M | Agent | done | Merged (`d6ccb7f`, from `987cd37`): press without a drag leaves stage, generation and effects unchanged; expose resends cached pixels (reconciled with `4cd730b`; only `CMakeLists.txt` conflicted). Host: 34/34 tests, replay hash and goldens unchanged. 2026-11-02: current cross-built G4 test_app including click rules and replay passes, hash 9b4a458633a32d9d |
| GL-4 | Refresh stale docs | S | Agent | done | Codex updated README for the two editions and notes; project notes and handoff are in `docs/`. 2026-11-02: current README/open-work/handoff, architecture, project and toolchain notes refreshed; superseded handoff/toolchain text archived; current host count 36 |
| GL-5 | Get CI running and green on GitHub | M | Jack + Agent | open | `checks.yml` now also builds the full and floppy size reports (configured, never run). Unverified: `pipx` pins, Retro68 job |

## P1: make v0.1 fast and shippable

| ID | Ticket | Size | Owner | Status | Notes / done when |
| --- | --- | --- | --- | --- | --- |
| GL-6 | Draw the line layers once per refine | M | Agent | done | Merged (`12912d1`): lines stay per band (each band is presented complete) but each band skips arcs and polylines that cannot touch its rows. G4 640x480, median of 9: refine job 783-786 ms to 378-381 ms at 1x (1014-1020 to 388-394 at 2x); lines-only 456 to 82 ms; checksum `1228dcc860a92a77`, replay hash and goldens unchanged; 8 of 9 mutants caught (survivor practically equivalent). The 573 vs 786 ms mismatch was two different views. Original ticket text: biggest measured speed-up. Profile (G4, 640x480): refine 786 ms with terrain, 456 ms without; lines re-walked in all 30 bands, about 16 times their 28 ms single pass. Re-baseline first: `terrain.md` says about 573 ms, the later profile 786 ms. Refined checksum `1228dcc860a92a77` and moving-frame pins unchanged. Fold into GL-14 if that starts soon |
| GL-7 | Measure real X11 display timing on the G4 | S | Jack | open | `globe-x11 --script ... --stats` from the desktop session; add to `docs/perf/` |
| GL-8 | Mac-native polish for v0.1 | M | Agent | open | Menus with shortcuts, About box crediting Natural Earth, NOAA ETOPO 2022 and Wikidata, icon, Quit; window and view in Preferences |
| GL-9 | Floppy size report | M | Agent | done (reports) | Codex, `48e810f`: `tools/build-edition.sh floppy|full`, `tools/floppy_report.py`, `docs/floppy-size.md`. 2026-11-02: local full/floppy builds, HFS fork roundtrip and size budget pass again; current reports include GL-27/28. CI remains unrun. **Open:** budget check actually running in CI |
| GL-10 | Decide the minimum system and trim the partition | S | Jack | open | SIZE resource now 24 MiB preferred / 12 MiB minimum; reset from measured use |
| GL-11 | Ship the v0.1 floppy edition | M | Jack + Agent | open | Disk image and archive, version stamp, attribution, README with screenshots |

## P2: surfaces, passes and the Classic style

GL-12 to GL-14 are pure refactors: every pin (moving-frame checksum, replay hash, refine checksum)
stays unchanged and the G4 benchmark stays within noise.

| ID | Ticket | Size | Owner | Status | Notes / done when |
| --- | --- | --- | --- | --- | --- |
| GL-12 | Surface ops table with an RGB565 wrapper | M | Agent | open | Small ops table (clear, line, span, clip rows); indirect-call cost measured on the G4 |
| GL-13 | Semantic ink and a Theme | M | Agent | open | Default theme reproduces today's colors byte for byte |
| GL-14 | Split the refinement job into passes and profiles | L | Agent | open | Fixed-size pass cursors, no frame allocation; pure profile function; dirty regions; lines once (absorbs GL-6); cancellation and generation tests kept. Follows the layered-refinement addendum in the ADR (base, terrain color, relief, lines, labels) |
| GL-15 | Recording surface | S | Agent | open | Hashes the primitive stream; platform-independent goldens for Moving and Refine |
| GL-16 | Native QuickDraw surface | L | Jack + Agent | open | OS 9 draws into an offscreen GWorld; double clipping before 16-bit coordinates; A/B benchmark on the G4 |
| GL-17 | Classic outline style | M | Agent | open | A style is a theme plus a profile; 1-bit displays |
| GL-18 | Display detection and the Auto profile | M | Agent | open | Pure profile choice; a governor never removes a layer the user turned on |
| GL-19 | Refinement ordering | M | Agent | done | Experiment `880df12` (rows, center-out, Gaussian dissolve) concluded by owner decision: rows, center-out and dissolve removed; refinement now goes outward from the touchpoint as `rectangle` (viewport-shaped, edges reached together), `circle` (default) or `globe` (equal great-circle angle). Final images identical for every order and focus; refined pin, replay hash and goldens unchanged. G4 totals: circle within 3 percent of the old rows at 1x and the trace, 11 percent slower at 2x; rectangle and globe 11-14 percent slower than circle at 1x and the trace. See `refinement-order.md` |

## P3: features and later data work

| ID | Ticket | Size | Owner | Status | Notes / done when |
| --- | --- | --- | --- | --- | --- |
| GL-20 | Layers floating palette | M | Agent | open | Checkbox per layer, progress or cost while refining; saved in Preferences |
| GL-21 | Find a place, with a bigger cities pack | M | Agent | open | Cmd-F search; Wikidata CC0 snapshot builder; 19 cities grows toward a few hundred (format caps 4,096) |
| GL-22 | Day/night terminator | M | Agent | open | Sun position from the system clock |
| GL-23 | Start at the Map control panel location | S | Agent | open | Confirm `ReadLocation` in the Retro68 headers |
| GL-24 | Copy view as PICT | S | Agent | open | Bitmap first; vector after GL-15 |
| GL-25 | Great-circle distance between two clicks | S | Agent | open | Tested against known city pairs |
| GL-26 | Movable refinement focus and effect choice | S | Agent | done for focus; chooser open | Focus (release, click, double-click center; view center otherwise) is deterministic model state used by all three orders; `O` key and `order TIME rectangle|circle|globe` choose the shape. A user-facing chooser (toolbar or menu) and other effects remain open |
| GL-27 | Terrain look | M | Agent | done, cost under review | Codex `df1789e`, reviewed by Claude on a clean checkout (35/35 host tests). Gray base, CycleStyle, two palettes, Antarctic white and shared-corner shading. G4 base about 4.3/5.3 ms; refine about 555/693 ms versus 380/386; longest continuation 120 ms. Owner/Claude decides further tuning; do not tune unasked. See terrain.md |
| GL-28 | Great Lakes and a lakes layer | M | Agent | done (builder) | Codex `3af70e4`, reviewed by Claude: pinned 50m source, cell-center water mask (135 centers/132 changed; Great Lakes 30/29), twice-reproduced pack, unchanged GELV v1 proxy; host/G4 pin efa030af1a10f887. Runtime lake/rivers vectors belong to GL-35 |
| GL-29 | Compression decision and a 0.25-degree terrain pack | L | Agent | partial | Measured (ADR): 1 degree 64,800 to 29,140; 0.25 degree 1,036,800 to 342,041 (average predictor + range coder); atlas delta-varint 32,480 to 26,736. **Open:** decoder timed on the G4, runtime container, import pack with size report |
| GL-30 | Older systems and 68k feasibility spike | L | Jack + Agent | open | No Appearance Manager; 68k with FPU in an emulator; fixed-point projection costed |
| GL-31 | Fetch inside the app, with TLS from `../weather` | L | Jack + Agent | design written (`bf40e25`) | fetch-design.md: independent library, weather licensing/reuse, seed lifecycle, bounded binary framing and safe pack swap; retained-symbol crypto/anchors size delta 104,206 bytes. Owner: full edition first, no v0.1 floppy fetch. Hosting/UI and design review remain; no networking code before review |
| GL-32 | Colors for deserts | S | Agent | open | Owner feedback: deserts are not deserty. Latitude tint first, or a land-cover grid as a further small versioned file; undecided |
| GL-40 | Ice flag for Greenland and glaciers | M | Agent | proposal written; format review required | `802841d`: ADR rechecks pinned 50m glaciated polygons, center/even-odd mask, v1 incompatibility, proposed v2 bit plane (+8,100 bytes at 1 degree), strict validation/test plan. No builder/decoder/data change; stop until format approval |
| GL-41 | Progressive line reveal after terrain | M | Agent | done | Lines (graticule, coasts, borders) reveal outward from the refinement focus in the chosen order shape, paced by a wake; settled pixels and pins unchanged; handoff written 2026-11-02 | Done 2026-11-02 (Codex's draft finished by Claude): 16-point chunks bucketed by ring, 30 ms paced reveal, canonical repair pass; settled pixels identical; G4 +45-60 ms compute and 480 ms of pauses; see `refinement-order.md`.
| GL-42 | Style palette window | S | Agent | X11 done | A small floating window (X11 first, OS 9 later) to choose the look (Map, Relief) instead of only the `S` key; remembers nothing yet. Owner request 2026-11-02 | Done 2026-11-02 (X11): `P` opens a small "Globe looks" window listing Map and Relief with swatches and a mark on the current one; clicking the other row switches (a `CycleStyle` command, so replays are unchanged); `S` and the toolbar button still work. OS 9 not done.
| GL-43 | Wait for a double click before picking a country | S | Agent | done | A single click's country pick is deferred by a short constant (owner asked for about a second; I would start near the double-click time, 300-400 ms, and tune) through the Tick wake path in the core, never a clock in the core; a second click inside it cancels the pick and zooms; the plain-click rule (no redraw, no refine restart) still holds; replay tests | Done 2026-11-02: `kPickDelayMs` = 400 ms (the double-click window); the core holds a pending pick (`Model::pick_pending`) and emits `Effects::pick` from a Tick; a second press, drag, wheel, command or resize cancels it; X11 picks only on that effect.
| GL-44 | Highlight the focused country | M | Agent | outline done | The picked country's border is highlighted or its area dither-filled (owner's words); outline-only is cheapest, a dithered fill must be an overlay that never changes settled terrain pixels or pins; measure G4 cost; cleared by the next pick or click on the sea | Done 2026-11-02 (X11): the picked country's rings are drawn as a bold orange outline (`draw_country_outline`, `include/globe/highlight.hpp`) on a copy of the frame that is shown, so the rendered frame, refinement and pins are untouched and clearing needs no re-render; a click on the sea or a place card clears it; the outline follows drags. The dithered fill is not done (needs polygon clipping at the limb). OS 9 has no picking yet.
| GL-45 | Cache settled frames | M | Agent | idea | Keep the last 2-3 settled frames keyed by camera, look and layers (600 KB each at 640x480, counted in the size report) so look switches and layer toggles are instant; measure first. Owner wants the app to look and feel faster; the terrain-plus-lines split and fewer rings are related options |
| GL-33 | Map look (Google-like flat style) | S | Agent | open | Default floppy look: flat pale land, blue water, light borders, labels and cities; reuses the flat moving base |
| GL-34 | Land-class grid and the Natural look | L | Agent | open | Pick a classification (e.g. Koeppen-style), check license and size, build a small versioned grid, 2D palette (class x elevation band); answers Sahara vs Amazon |
| GL-35 | Rivers and lakes layer | M | Agent | open | Natural Earth rivers/lakes as a vector layer (atlas-format layer); measure size and G4 cost; v0.1 candidate if small |
| GL-36 | Thematic (ColorBrewer-style) terrain scheme | S | Agent | open | A named scheme for the terrain-color step; check ColorBrewer license/attribution |
| GL-37 | Satellite-like color-grid experiment | M | Agent | open | Bake a palette-indexed color grid from imagery at 1 and 0.25 degrees; measure size, compression and look; check imagery license; no texture mapping |
| GL-38 | Antialiasing in the Refine stage only | M | Agent | open | Lines and globe silhouette; off on 1-bit/8-bit; measure G4 cost first; new refined pins |
| GL-39 | Climate layers and atlas popups | L | Agent | open | Full edition: climate/biome fact in the info window; optional temperature/rainfall grids; check data licenses |

## Parked and non-goals

| Item | Why parked | Revisit when |
| --- | --- | --- |
| A separate maps app | Different product: own projection, tile cache, network code (ADR "data policy") | The globe's pack formats and libraries have settled |
| A relay between the apps and a server | Rejected by the owner | Not planned |
| Satellite imagery | Per-pixel inverse projection is slow on the G4; about 1 MB of texture; license check | Never on the floppy; only as an imported pack |
| Perlin noise as terrain | Not a replacement for real elevation | Not planned |

## Open decisions and risks

- **The OS 9 build has run once, under Tiger Classic (emulated), and rendered; interaction and real hardware are untested.** If GL-1 fails, P1 and P2 reorder around fixing it.
- **Native QuickDraw speed is unknown.** GL-16 settles it; keep both surfaces until then.
- **Refine timings depend on view and quality.** GL-6 was re-baselined and measured; GL-27 costs are separately recorded in terrain.md. Do not compare different views as a regression.
- **Floppy contents are open.** Working rule: the floppy edition is the default build minus optional
  data packs; the size report shows when it stops fitting. Two releases only, floppy and full.
- **Fetch scope and size.** BearSSL plus anchors may not fit the floppy edition (GL-31).
- **User or Auto?** Proposed: the user's layer choices always win; Auto adjusts detail and cadence only.
- **Minimum supported system.** GL-10 decides it; GL-30 explores older systems and 68k.
- **Acceptance still open:** current real-display X11 timing and native OS 9 execution. G4 headless click rules/replay now pass; that is not GUI acceptance.
