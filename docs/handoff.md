# Codex handoff — Globe

Current operating instructions, refreshed 2026-11-02. This file is committed. Older notes and
superseded branch/remote/test-count statements are [archived](history/handoff-20261101.md).
Latest continuation: GL-19/26 experiment and GL-40 proposal complete; see the final dated note and
[experiment report](refinement-order.md). Rows remain default; ice format awaits review.
Read [backlog](backlog.md), [project notes](project-notes.md) and the [ADR](adr/0001-m2-to-m5-decisions.md).
Append a dated note at the bottom when a pass ends: changes, verification, failures and limits.

## Repository and authorization

Work is on `main`; the owner renamed `v2`. Tag `v1` preserves the prototype. Origin exists, but
**do not push, merge remote branches, tag or force anything**; the owner publishes. Commit narrowly
with `Co-Authored-By: Codex <noreply@openai.com>`. Do not commit source ZIPs, generated binaries,
disk images, private TLS seeds or credentials. All earlier Claude side branches are merged.

Every first-party source starts with GPL-3.0-or-later SPDX and Copyright (c) 2026 Jack Willis.
Run policy checks after staging new source files (the script checks tracked paths). Preserve user
changes; inspect the tree before editing. No subagents are required by this handoff.

## Completed current pass

The 2026-11-02 assignment was GL-27 terrain look, GL-28 builder lake masking, GL-4 stale docs and
GL-31 fetch design only. GL-27 and GL-28 are committed; the fetch design is committed; documentation
is refreshed and recorded with this handoff. Relevant implementation files were reserved for this pass.
No surface/theme/pass refactor, dissolve, runtime compression, maps app or networking implementation
is authorized by that assignment. The fetch design has hosting/UI questions before code. The newer product ADR calls this an atlas:
Map will be the floppy default; Natural needs an unchecked land-class source. No fetch on the v0.1
floppy; full edition first. Never put the owner's email in a User-Agent or any outbound request.
The owner/Claude will decide further smoothing tuning; do not tune it unasked. Next suggested
scopes are GL-33/35/36, not additional work silently included in this completed pass. GL-34/37/39
must wait for source license/size checks.

- Gray moving base from a coarse elevation mask; hypsometric/cel-shaded tables, S/CycleStyle and
  toolbar/trace integration; Antarctic land south of -60 is white; shared-corner shaded triangles
  with clipped flat limb fallback. See [terrain](terrain.md) for costs and continuation limits.
- Pinned 50m Natural Earth lakes use cell-center even-odd filling, including holes/date-line wrap.
  Two complete builds reproduce the new one-degree pack. See [format](elevation-format.md) for the
  explicit v1 shallow-water proxy and its loss of positive inland elevation.
- [Fetch design](fetch-design.md): weather reuse and licenses, independent library/platform split,
  seed provisioning, binary HTTP limits, validation/install policy and measured crypto link cost.
- README, execution log, architecture, project/toolchain notes now distinguish current state from
  historical measurements. [Backlog](backlog.md) is the next-scope authority.

## Current validation and pins

Host GCC ASan/UBSan and clang suites: **36/36**. Both Xvfb depths, pixel readback, toolbar/popup,
repaint and quiet-idle tests pass. PowerPC and 68k portable compile probes pass; the current OS 9
application builds and HFS fork roundtrip/budget checks pass, **not run**. CI is configured, not run.
G4 app/terrain tests and headless benchmarks pass. Earlier X11 builds were owner-tried; current
real-display submission/scanout and GUI acceptance remain owner work.

Preserve app replay `9b4a458633a32d9d` and outline goldens. The old outline-only moving benchmark
`41c7adb508763323` predates the gray base. GL-27 intentionally changed settled replay
`1228dcc860a92a77` to `27336cb8117e5e7f`; GL-28 changes it to **efa030af1a10f887**. Host/G4 agree
for the current combined replay; the golden is `tests/golden/refine-checksum.txt`. Moving-base repeat-2 checksum is **71e66eac3bde93c7**, also equal on host/G4. Re-pin only for an intended change with a
reason and cross-host evidence.

GL-27 measured gray fill alone at about 4.3/5.3 ms (G4, radius 220/440, 640x480), settled profile
about 555/693 ms versus 380/386 before appearance changes. The standard trace's final view refined
in about 802 ms; longest continuation 120 ms. These are different views from older 573/786-ms
reports. Work counts are bounded; no hard wall-clock promise or real-display timing is made.

## Hardware and portability rules

Never compile on maul. Owner binaries `/tmp/globe-x11-tip3` through `tip8` are theirs: do not kill,
delete or overwrite them. Cross-build locally and push tests under fresh names with
`~/.local/bin/maulctl push nb FILE NAME`; restore chmod 755, run headless. Do not change desktop
display access. `/tmp` can disappear after reboot. Pass names include `gl19-20261102-*`, `gl27-20261102-*` and
`gl28-20261102-*`; they are tests, not owner acceptance builds.

No OS/window headers, clocks, exceptions/RTTI, std::hypot, printf %ll/%z, or hidden frame allocations
in `include/globe`, `src/core`, `src/data`, `src/render`. The existing trace formatter is the reviewed
stdio exception. Platform code stays under `src/platform`; common helpers stay platform-neutral.
Run Retro68 probes after portable changes. Do not copy Apple headers, implementations or binaries.

Sources remain outside Git: ETOPO in `~/scratch/globe-data/etopo/`, Natural Earth ZIPs in
`~/scratch/globe-data/naturalearth/`. Recheck pinned hashes; source z row zero is southernmost.
Do not add a GELV flag silently: a format change needs its own reviewed version. No relay.

## Commands

```
cmake -S . -B build -G Ninja -DGLOBE_SANITIZE=ON
cmake --build build
ctest --test-dir build --output-on-failure
ctest --test-dir build -N
cmake -S . -B build-clang -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang
ctest --test-dir build-clang --output-on-failure
git ls-files '*.cpp' '*.hpp' | xargs clang-format --dry-run --Werror
git ls-files 'src/*.cpp' ':!:src/platform/os9/*' | xargs clang-tidy -p build --quiet
tools/probe-retro68.sh
tools/check-policy.sh
tools/build-edition.sh full
tools/build-edition.sh floppy
```

Sanitizer/Xvfb/container checks may need execution outside the filesystem sandbox. Read the actual
compiler/test output. `tools/probe-retro68.sh` prints `FAIL` even if its shell exits zero: check that
no FAIL appears. Both edition reports describe builds, not runtime or CI acceptance.

For a current headless NetBSD cross-build, embed **both** atlas and elevation, then use the local
cross compiler/sysroot with `-std=c++20 -O2 -static -fno-exceptions -fno-rtti`, `-Iinclude` and
`-Isrc/platform/common`, linking bench.cpp, common/*.cpp, core/*.cpp, data/*.cpp, render/*.cpp and
the two generated resources. See [toolchain evidence](toolchain.md). Never compile on the target.

## Dated pass notes

### 2026-11-02 — Claude: product decisions and review of your GL-27/GL-28/GL-31 commits

Reviewed `df1789e`, `3af70e4`, `bf40e25` on a clean checkout of the commit: 35/35 host tests pass; I did
not re-run the G4 or read every diff line. Your honest cost report was useful: smoothing took the
G4 refine job from about 380 to 555 ms (693 at 2x) and a 120 ms continuation step is a responsiveness
limit. Next decision on that is Claude's/the owner's, not yours: do not tune it further unasked.

Product decisions (ADR addendum "product: an atlas; looks and overlays", backlog GL-33 to GL-39): the
product is an **atlas**; terrain looks are alternative tables over the same cells (Hypsometric, Cel-shaded,
Natural with a land-class grid, Thematic ColorBrewer-style, Map (Google-like flat; the floppy default),
Satellite-like as an experiment, Classic outline); the floppy ships at most three looks; rivers and lakes
are a v0.1 candidate; antialiasing is Refine-only and off on 1-bit/8-bit; **no fetch on the v0.1
floppy** (TLS seed story undesigned; link cost about 104 KB), full edition first. Suggested next tasks if
you continue: GL-33 (Map look, small), GL-35 (rivers and lakes layer, measure size), GL-36 (thematic
scheme). Do not start GL-34/37/39 (data licenses and sizes unchecked). Never put the owner's email in a
User-Agent or any outbound request.

### 2026-11-02 — Codex: completed assigned GL-27/28/4/31 design pass

- `df1789e` implements GL-27: coarse gray moving land, two palette styles with model/trace/toolbar
  control, Antarctic white, shared-corner shading and clipped flat limb fallback. The final frame
  is tested independent of its starting base. G4 isolated base costs 4.2727/5.2840 ms; matched
  profile refine 379.944/386.428 -> 554.943/692.716 ms. Standard trace: 801.8763 ms refinement and
  120.1318-ms maximum continuation in the reported run. Smoothing retained; the owner/Claude's
  subsequent cost/tuning decision is preserved above and no further tuning was done.
- `3af70e4` implements GL-28 builder scope. Rehashed the persisted 50m lakes ZIP, inspected 5.0.0
  and 412 records, chose cell centers (not sampled majority area), retained island holes/date-line
  wrapping and south-first rows. 135 selected/132 changed cells; Great Lakes 30/29, independently
  cross-checked. Two full ETOPO+lakes conversions match SHA-256
  8d0fd2e6777f24dc13ac47e0f73aa459e503c70116d66a61bb2b339b618766a6. No format/flag change;
  positive inland-water height is intentionally lost to the v1 -1 m color proxy, documented.
- Intentional combined replay pins: GL-27 27336cb8117e5e7f, then lake-aware efa030af1a10f887,
  equal on host/G4. Moving/base repeat-2 checksum remains 71e66eac3bde93c7, also equal.
  App replay remains 9b4a458633a32d9d; outline goldens unchanged. G4 app (including click rules)
  and terrain tests print ok, so GL-3's headless follow-up is complete.
- `bf40e25` writes GL-31 design only. Weather's license/pin, seed hardening, stream/TLS/DNS and
  HTTP framing were read. A matched, unexecuted retained-symbol OS 9 link probe measures 99,960
  bytes of crypto path plus 4,246 for anchors/retention (104,206 combined). It excludes a complete
  fetch feature; no networking code or dependency was added to the application. The newer owner
  decision is incorporated: no v0.1 floppy fetch, full first, and no owner email outbound.
- GL-4 refreshes live README/handoff/execution log, architecture, toolchains, project/backlog and
  size notes. Superseded handoff/bootstrap text is explicitly archived, not silently discarded;
  the concurrent owner's product/review note above is preserved. Both editions rebuilt. Floppy
  payload 273,691 bytes, HFS free 1,174,528; full native executable 343,392 bytes. HFS application
  forks roundtrip identically and the budget passes. OS 9 is built/packaged, not executed.

Verification: final GCC ASan/UBSan and clang suites **35/35** each, both private X11 depths,
click/present-only/quiet-idle checks; PowerPC and 68k portability probes pass; current OS 9 app
links. Format/policy and host-source lint checked. The merged profiler tools' unchecked atoi and
implicit conversions were found by full lint and fixed narrowly in `6871627`, with valid/invalid
CLI runs. `5e94ea8` asserts exact style-command trace roundtrip. Smooth terrain and the lake mask
were visually inspected via host framebuffer previews.

Tests first failed on missing appearance APIs, as intended. Four appearance mutants (ice, style
propagation, smoothing and water rejection) and three lake mutants (holes, row order and water
index) fail. An old toolbar-count assertion needed the new command count. Retro68 exposed unsigned
int/uint32_t max deduction; explicit uint32_t fixed it. The expected terrain pin initially failed
before the intentional cross-host re-pin. A same-length Python mutant survived in generated pyc
cache after source restoration; clearing that cache and rerunning every variant gave the expected
fail/pass sequence. The initial TLS size probe had no retained TLS symbols and zero delta; it was
discarded and replaced by a retained entry probe verified with nm, never executed.

No pushes, remote merges, tags, maul compilation, owner-binary changes or display permission changes.
No CI, current G4 desktop/scanout, native OS 9 runtime or live TLS claim. No further smoothing tuning,
Map look, river vectors, thematic look, class/imagery grid, antialiasing, pass/surface refactor,
dissolve or runtime compression in this pass. Those remain separate backlog scopes/owner gates.

### 2026-11-02 — Claude: next tasks for Codex (bell-curve dissolve; Greenland ice)

Merged and tested at `1e6ea18` (36/36 host tests; branches gone); owner has `/tmp/globe-x11-tip8` on maul.

1. **GL-19/GL-26: refinement focus and bell-curve dissolve.** Design is in the ADR addendum "refinement
   focus and a bell-curve dissolve" (`5a929c4`): read it first. Implement it as an experiment that can
   be compared with today's rows and a center-out row order, switchable in code or by a trace verb so a
   replay can pick the order. Requirements: focus is model state set by pointer release, click and
   double-click center (view center otherwise); per-cell hash randomness (no clock, no RNG state);
   Gaussian falloff through a 256-entry table and squared-distance compares, no `sqrt`/`log` per cell;
   one pass byte per cell, last pass draws everything; cancellation between passes; lines redrawn after
   or between passes (measure the cost); the **settled image must be identical for every order and every
   focus**, with a test that proves it (several foci, hash of the final frame). Do not change any pin
   unless explained; the final refined checksum should not change at all. Mutation-check the tests.
   Measure on the G4: time to first pass, total refine time, longest continuation step (the smoothing
   work left a 120 ms step; report whether the dissolve shortens the worst step). Commit as GL-19.
   Do not tune the smoothing cost; the owner has not decided yet.
2. **Greenland and glaciers (GL-40): write the proposal only.** Antarctica is white by a latitude rule;
   Greenland is not covered, so its ice sheet is elevation-colored. Propose, in the ADR, how to mark ice
   at build time from the Natural Earth glaciated-areas 50m layer (`data/SOURCES.md`; ZIPs in
   `~/scratch/globe-data/naturalearth/`): cell-center even-odd like the lakes, which GELV v1 mechanism
   could carry the flag without breaking v1 decoders (or whether v2 is needed), size effect, and a
   test plan. If it needs a format change, stop after the proposal. Do not change data files.

Rules still apply: no push, no email or personal contact details in any outbound request, new maul file
names only, never touch `tip3` to `tip8`.

### 2026-11-02 — Codex: GL-19/26 experiment and GL-40 proposal completed

Read the refreshed handoff/backlog, project notes and ADR, including the owner's newest two-task
assignment. Completed that continuation; the older next-scope suggestions were not substituted.

- `880df12` (GL-19): model focus records click/drag release, motion inside slop, and double-click
  center; initial/resize fallback is view center. Trace `order TIME rows|center|dissolve` compares
  existing rows, focus-outward bands and 16 deterministic Gaussian cell passes. One pass byte per
  cell, fixed hash, 256-entry offline log table and squared distances; no clock/RNG or frame
  allocation. Last pass explicitly repaints all cells in canonical order, with four admitted
  16-row cell slices per experimental continuation; lines run once afterward. Cancellation remains
  checked at every step. Smoothing/rasterization untouched. Rows remain default, not a product
  decision to switch. GL-26 focus is complete; broader effect UI/Ripple/Sweep/Instant remain open.
- Matched G4 nine-run medians: rows -> dissolve totals 692.629 -> 796.017 ms (profile 1x),
  686.689 -> 781.978 ms (2x/off-center), 644.300 -> 732.341 ms (trace). Dissolve's first complete
  pass: 84.284 / 81.787 / 84.168 ms; final lines: 43.609 / 44.438 / 38.117 ms. Worst observed
  dissolve step 26.207 ms, versus rows 70.753 ms in this run. The trace's matched worst steps are
  25.125 vs 24.539 ms, essentially equal: **no demonstrated scheduling improvement over today's
  rows**, and no hard timing promise. The historical 120 ms event did not recur in rows either.
  Dissolve is about 14–15% slower overall. See refinement-order.md and perf/refine-order-*.json
  for method, every order/view, median maxima, worst maxima and exclusions. No smoothing tuning.
- G4 app and terrain unit tests pass. All eight progress snapshots (two foci, passes 3/7/11 and
  settled) match host hashes. All three G4 full-frame view hashes agree across orders and with
  host; separate two-repeat G4 replay checks retain **efa030af1a10f887** for rows, center and
  dissolve. App hash **9b4a458633a32d9d** and outline goldens unchanged. No pin file changed.
  Host contact sheet `build/gl19-progress.png` inspected: screen-space pattern follows the focus.
- `802841d` (GL-40): proposal only in the ADR. Rehashed/inspected 50m glaciated 4.1.0 source:
  f48644b4523482501bcd380c4b7345ba574ab79b87df753447d772ea239b7daf, 216,484 bytes,
  377 records / 395 parts / 18,868 points. Cell-center/even-odd with holes/date-line wrap and
  height preservation; v1 cannot carry a lossless flag under its strict LUT/length/zero-flag
  rules. Proposed v2 one-bit plane adds 8,100 bytes at one degree, 129,600 at quarter degree;
  exact CRC/length/padding/compatibility and independent/mutation tests specified. These are
  format arithmetic, not built data. **Stopped at proposal for review.** No builder, decoder,
  data file or new format assignment changed.

Verification: final GCC ASan/UBSan **36/36** (62.01 s), clang **36/36** (40.75 s), including both
private X11 depths and existing refined-pin tests extended through both experimental trace orders.
PowerPC and 68k Retro68 probes pass with no printed FAIL. Format, touched-source lint, policy and
diff checks pass. OS 9 app links/packages with identical HFS fork roundtrip and passes budget:
data fork 241,234 bytes (+4,118 over prior snapshot), payload 277,809, HFS free 1,170,432. Its
separate size snapshot is in perf/refine-order-floppy-size.json; built/packaged, **not executed**.

Tests first failed on absent APIs. Five final mutants fail: lost release focus, center replaced by
rows, ignored horizontal focus, omitted final full repaint, and ignored generation cancellation.
Focus-distance and repaint mutants initially survived final-pixel assertions; independent x/y
focus checks and final-pass slice coverage caught them after strengthening the tests. Restored
normal tests pass. Clang found a missing test algorithm include; lint found unchecked fixed-camera
optionals; both fixed. A new coverage assertion initially refreshed its counter during the last
pass instead of at entry; corrected the test. Sandbox LeakSanitizer cannot run under ptrace;
final sanitizer suite ran outside that restriction and passes.

Limits: headless G4 only; no real-display/scanout, native OS 9 or CI acceptance. Dissolve temporarily
omits outlines until final lines and uploads full frames per pass; presentation costs are excluded,
so owner hands-on acceptance is still required before changing the default. No push, remote merge,
tag, compilation on maul, display-access change or touch to owner tip3–tip8. No additional backlog
work, networking, ice data, smoothing tuning or pass/surface/theme refactor in this continuation.

### 2026-11-02 — Claude fork: refinement orders replaced by rectangle, circle and globe

Owner decision: dissolve, rows and center-out are removed. Refinement of the settled terrain goes
outward from the touchpoint as `rectangle` (normalized to the viewport so the edges are reached
together), `circle` (default) or `globe` (equal great-circle angle); see `docs/refinement-order.md`.
Trace verb `order TIME rectangle|circle|globe` (old names and `square` are rejected), X11 `O` cycles them.
Final images, refined pin, replay hash and goldens are unchanged. Outline-only refine (no terrain) still
uses top-down 16-row bands. Do not reintroduce the removed orders without an owner request. Not verified:
real display, OS 9 execution, CI.

### 2026-11-02 — Assignment for Codex: GL-34 land-class grid + GL-33 Map look (written by Claude)

Authorized: GL-34 and GL-33 from [backlog](backlog.md); a build-time data pack feeds both looks.
Not authorized: fetch/networking, rivers (GL-35), thematic scheme (GL-36), satellite, antialiasing,
smoothing tuning, surface/theme/pass refactors, any push/tag/merge to remote. Claude is working
separately on the refinement *focus* (src/core/app.cpp, tests/test_app.cpp); avoid those files, and
if you must touch them, keep the edit minimal and say so.

Owner feedback to satisfy: deserts are not deserty (Sahara, Arabian, Gobi, Australian interior);
the Amazon and other forests should read as forest; Antarctica white; Greenland is not white
(GL-40 is a proposal only, leave it). Map should be the default look (Google-Maps-like: muted
land, blue water, light relief); Natural uses land classes; at most 3 looks on the floppy.

Do:
1. **Source check first** (license, size, SHA-256, record in `data/SOURCES.md`): a land-cover/
   biome source usable at 1 degree, public domain or compatible with GPL-3.0 redistribution
   (candidates: Natural Earth physical/raster themes, ESA WorldCover/CCI summaries only if
   license is clear, WWF ecoregions only if license allows). If nothing is clearly licensed,
   stop and report; do not guess. Never put the owner's email in any request or User-Agent;
   use a generic `globe-builder/0.1 (offline data build tool)` and report the exact header.
2. Builder: a class grid (desert, forest, grass/shrub, tundra, ice, crop/urban optional) at
   1 degree, one byte per cell, compressed with the merged `grid_codec`. Deterministic, two
   builds identical, source ZIPs not committed.
3. Looks: add Map (default) and Natural using the class grid; keep the existing hypsometric/
   cel styles reachable by `S`. Class colors in the palette tables, no per-pixel work.
4. Report floppy size (`tools/floppy_report.py`) and G4 refine time (1x/2x, median of 9) for
   each look; call out any look that does not fit.
5. Pins: moving-base and replay pins change only if the default look changes; if so re-pin
   with reasons and host/G4 evidence. Update `docs/terrain.md`, `data/SOURCES.md`, backlog.

Verify as before (36/36 host, ASan/UBSan, policy, Retro68 probes), mutation-check new tests,
append a dated note here. No compiling on maul; owner/Claude push test builds.

### 2026-11-02 — Assignment for Codex, after GL-34/33: progressive line reveal (GL-41, written by Claude)

Owner request: after the terrain has finished refining, the vector layers (graticule, coastlines,
borders) should reveal outward from the view center (the owner chose the center over touchpoint and halfway; `quality_for`) in the selected order shape (rectangle/circle/globe), instead of
all appearing at once. Plain lines while dragging stay as they are. Do GL-34/33 first; this touches
`src/render/terrain.cpp` too.

Design to start from (change it if measurement says so):
- Walk each polyline once per job and cut it into chunks of about 16 points; give each chunk the
  pass of its midpoint from the same pass plan the cells use (`make_pass_plan`). Store small
  (arc, start, count, pass) entries, not per-segment data; report the bytes. No per-pass re-walk:
  that is the 16x mistake GL-6 removed.
- After all terrain passes (the final canonical repaint stays as is), draw pass k's chunks, report
  the frame ready, then pass k+1. Cancellation at every step as now.
- Pacing: a timer wake between steps so the sweep is visible; step time is one named constant,
  start at about 30 ms (about half a second in total) and make it easy for the owner to tune. Any
  input cancels the reveal as it cancels refinement. Clocks only through the existing wake/Tick
  path, never inside the core; replays stay deterministic.
- The settled image must be pixel-identical to today's for every order and focus: refined pin,
  replay hash and goldens must not move. Chunk boundaries must not drop or double-draw a segment
  (draw the shared end point once) and clipped limb polyline behavior must be unchanged.
- Measure on the G4 (1x/2x, median of 9): line-walk plus chunking cost, total settle time, worst
  step, memory. X11 upload cost per step is unmeasured (GL-7 is the owner's); say so.
- Outline-only refine (no terrain) keeps its top-down bands unless trivial to include.
Tests: every-pixel equality against the unpaced result for all three orders and several foci;
oracle for chunk passes; cancellation mid-reveal; mutation checks. Append a dated note here.

Owner update: the refinement focus is now always the view center (no touchpoint state in the Model).
