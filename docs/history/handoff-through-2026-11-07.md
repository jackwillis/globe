# Codex handoff — Globe

Current operating instructions, refreshed 2026-11-02. This file is committed. Older notes and
superseded branch/remote/test-count statements are [archived](history/handoff-20261101.md).
Latest continuation: see the final dated notes (2026-11-03); the ADR addenda hold the owner decisions.
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

Never compile on shared G4. Owner binaries `<temporary-test-binary>` through `tip8` are theirs: do not kill,
delete or overwrite them. Cross-build locally and push tests under fresh names with
`<private-host-control-helper> push nb FILE NAME`; restore chmod 755, run headless. Do not change desktop
display access. `/tmp` can disappear after reboot. Pass names include `gl19-20261102-*`, `gl27-20261102-*` and
`gl28-20261102-*`; they are tests, not owner acceptance builds.

No OS/window headers, clocks, exceptions/RTTI, std::hypot, printf %ll/%z, or hidden frame allocations
in `include/globe`, `src/core`, `src/data`, `src/render`. The existing trace formatter is the reviewed
stdio exception. Platform code stays under `src/platform`; common helpers stay platform-neutral.
Run Retro68 probes after portable changes. Do not copy Apple headers, implementations or binaries.

Sources remain outside Git: ETOPO in `<verified-source-cache>`, Natural Earth ZIPs in
`<verified-source-cache>`. Recheck pinned hashes; source z row zero is southernmost.
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

No pushes, remote merges, tags, shared G4 compilation, owner-binary changes or display permission changes.
No CI, current G4 desktop/scanout, native OS 9 runtime or live TLS claim. No further smoothing tuning,
Map look, river vectors, thematic look, class/imagery grid, antialiasing, pass/surface refactor,
dissolve or runtime compression in this pass. Those remain separate backlog scopes/owner gates.

### 2026-11-02 — Claude: next tasks for Codex (bell-curve dissolve; Greenland ice)

Merged and tested at `1e6ea18` (36/36 host tests; branches gone); owner has `<temporary-test-binary>` on shared G4.

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
   `<verified-source-cache>`): cell-center even-odd like the lakes, which GELV v1 mechanism
   could carry the flag without breaking v1 decoders (or whether v2 is needed), size effect, and a
   test plan. If it needs a format change, stop after the proposal. Do not change data files.

Rules still apply: no push, no email or personal contact details in any outbound request, new shared G4 file
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
tag, compilation on shared G4, display-access change or touch to owner tip3–tip8. No additional backlog
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
append a dated note here. No compiling on shared G4; owner/Claude push test builds.

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

### 2026-11-02 — QUEUED for Codex: do this only AFTER your current GL-34/33 pass is committed (written by Claude)

Do not start or fold this into the current pass. Finish, verify and commit GL-34/33 and append your
dated note first. Then, as a separate pass (separate commits), do the owner decisions below.
Do GL-41 (progressive line reveal) after this pass, not before.

Owner decisions:
1. **Circle only.** Remove the `rectangle` and `globe` refinement orders and the order choice:
   `RefinementOrder`, the pass plan's other branches and the globe cosine table, `SetRefinementOrder`,
   the `order` trace verb (it becomes unknown), the X11 `O` key, the toolbar/popup entries if any,
   OS 9 key handling if any, README lines, and tests/benches that only exist for other orders.
   Keep the circle pass plan and the view-center focus. Final pixels must be unchanged: refined
   pin, replay hash and goldens must not move except for intended look changes in item 3.
   (G4: circle 711/765/635 ms against globe 793/765/725; owner prefers circle.)
2. **Remove Cel-shaded** entirely from `TerrainStyle`, the cycle, its palette, tests and the
   floppy conditional. Remove dead code, do not leave a disabled style.
3. **Looks: Map and Relief.** Owner is letting go of the hybrid. Looks are Map (default,
   Google-Maps-like) and Relief (a plain hypsometric tint, green to tan to brown, high ground
   white like a classic atlas; no gray base, no cel). Drop the separate Natural look unless you can
   show it is not redundant with Map; its land-class colors belong to Map. `S` cycles the looks.
4. **Map is too pastel.** Make it punchier: stronger water blue, richer greens for forest and
   grass, warmer tan for desert, less white mixed in. Keep the class palette in one small table
   so the owner can nudge saturation in one place. Show the palette values in your note.
5. **Antarctica reads brown** (tips 11-14). The only white rule is `meters >= 0 && latitude < -60`
   in `terrain_color`. Before fixing, render a south-pole view and read pixel colors to find the
   cause; suspects: smoothing blending brown at the white/non-white boundary, bedrock elevations
   at or below 0 skipping the rule, edge cells above -60, the high-elevation tint. Antarctica must
   be white in both looks. Report what you measured. Greenland ice stays out of scope (GL-40).
Verify as usual (host suites, ASan/UBSan, policy, Retro68 probes, G4 timings of the looks),
mutation-check changed tests, update docs (`refinement-order.md` shrinks to circle only, `terrain.md`,
`README`), update the backlog, append a dated note. No push, tag or remote merge.

### 2026-11-02 — Claude: the queued pass is done; state for Codex's next session

Codex died of OOM mid-GL-41. Its uncommitted line-reveal work is saved, unverified, on branch
`wip/gl41-codex` (commit f784db0); `main` does not contain it. Claude then did the queued owner
decisions on `main`: circle-only refinement (rectangle, globe, the `order` verb, the O key and
`globe-refine-order` removed), Map and Relief looks only (Cel-shaded and Natural removed, Relief
white from 6,000 m, Map palette punchier and in one table), re-pinned refined Map `e86d02e2424d2a47`
and Relief `1c3eac3bcc2d3b8f` (host and G4 equal). Antarctica could not be reproduced as brown: it
renders white from every angle in both looks, so the owner may have seen the thin tan coastline
and ice-shelf lines; ask before changing it. New owner requests are GL-42 (style palette window),
GL-43 (delay a country pick for double clicks) and GL-44 (highlight the focused country), see the
backlog. Next for Codex: GL-41 on top of `main` (the WIP branch is a starting point, not trusted),
then GL-43/44. Memory is tight on the laptop: build with `-j2`.

### 2026-11-02 — Claude, unattended pass: GL-43, GL-44, GL-42, GL-41 done

`main` now has: a click picks only after the 400 ms double-click window (core pending pick,
`Effects::pick`); the picked country is outlined in orange on the shown copy of the frame (X11);
`P` opens a looks palette window (X11); GL-41 progressive line reveal (Codex's draft, ported to
circle-only and finished: 16-point chunks, 30 ms paced rings, canonical repair, identical settled
pixels; G4 +45-60 ms compute, 480 ms of pauses). Host 40/40 (x11_idle/x11_repaint can fail only
under parallel ctest load and pass alone), Retro68 probe clean, OS 9 floppy build packaged
(not run). Not done: dithered country fill, OS 9 picking and palette, GL-38 antialiasing,
GL-35 rivers, GL-36, GL-40 (needs format approval). Tip17 is the latest G4 build.

### 2026-11-03 — Claude: OS 9 first run, speed and feel (end of the night)

Done on `main`, all committed: OS 9 runs under Tiger Classic (window title shows settle ms and copy
ms; see `docs/os9-proof.md`); work slices and a presentation throttle on OS 9 and X11; line reveal
at 15 ms; refinement precomputes during a paused drag (300 ms, button down) and is shown only on
release (`presentation_held`; the platform gating has no automated test). Host 40/40, Retro68
probe clean, OS 9 floppy build packages. Not done: the settled-frame cache (GL-45), terrain-plus-lines
split, OS 9 picking and palette, dithered country fill, GL-38/35/36/40, GL-5 CI (first push to
GitHub may start it), GL-7 X11 display timing, real OS 9 hardware. shared G4 was left booted into Tiger.
`main` was pushed to origin on 2026-11-03 at the owner's explicit request (no tags).

CI ran on GitHub for the first time (run 41576444739): everything passes except clang-tidy on
three lines of `src/platform/bench/codec_bench.cpp` (lines 98 and 116 implicit widening, line 138
`atoi`). Fix those first next session; the size-report step after lint has not run yet. See GL-5.

### 2026-11-05 — Claude: OS 9 parity, merge of Codex's scheduling branch, overnight queue

On `main` (local, **not pushed**, 6 commits ahead of origin at the time of writing): the shared
`Overlays` module (night, measure, country focus, picking) used by X11 and OS 9; OS 9 now has `D`
night shading (clock from `GetDateTime` + `ReadLocation`, checked in Tiger Classic: the terminator
matched the Mac clock and time zone), `M` measure, country/city picking with a Toolbox information
window, and city markers; the floppy payload is 566,642 of 1,200,000 bytes. Codex's committed
`codex/os9-cooperative-scheduling` (00d0760, a71dbd7) is merged (026629d); its newer UNCOMMITTED
work (`classic_present.hpp`, `tests/test_classic_present.cpp`, edits to `os9/main.cpp` and
`CMakeLists.txt`) is still only in its worktree `<isolated-agent-worktree>`
and will conflict with the merged `os9/main.cpp`: rebase it on `main` before continuing.

Owner requests queued for the overnight session (in this order): (1) night shading slows dragging:
draw only the terminator line while moving or refining and full shading once settled (also skip the
country dither fill while moving); (2) the mouse scroll wheel does nothing on OS 9: diagnose which
events Classic delivers and handle them; (3) keep notes here. Untested on Tiger: `M`, clicks and the
information window (remote key `d` worked via System Events; mouse clicks via System Events did not
produce a visible result in the one try).

### 2026-11-05 (night) — Claude: merge done, quick overlays, wheel findings, Tiger is shared

Merged `codex/os9-cooperative-scheduling` (committed state only) into `main` (026629d) and then did
the owner's three requests on `main`: (1) night shading no longer slows dragging: frames that move or
are still refining draw only the terminator line and outline the focused country without the dither
fill (`Overlays::compose(..., quick)`, `draw_terminator`); the settled frame gets the full shading;
X11 and OS 9 both show the full overlays once the job finishes. (2) OS 9 scroll wheel: a classic
(non-Carbon) program never gets wheel events from `WaitNextEvent` (only Carbon events carry them),
so Page Up/Page Down now zoom (wheel drivers can send them) and unknown event codes appear at the
end of the title (`ev CODExCOUNT`); a native wheel needs a Carbon build (separate change). (3) notes.

State: `main` is ahead of origin by about 9 commits, not pushed. Host 44/44, clang-tidy and
clang-format clean on the changed files, Retro68 probe and the OS 9 floppy build pass (payload
about 567 KB of 1.2 MB). Verified on Tiger Classic before Codex's session overlapped: city markers,
`D` night shading (clock and time zone correct), the merged scheduling build runs (title shows
turn and step times). NOT verified on Tiger: the quick (line-only) mode while dragging, `M`
measuring, clicks and the information window, Page Up/Down, and any real wheel.

**Tiger is one shared machine.** Codex and Claude both launch builds there; a late screenshot
showed Codex's build (no markers or overlays), and Classic was quit between runs. Before testing on
Tiger, check `ps -ax | grep TruBlue` and the Globe volumes, and say in the handoff which build is
running; do not quit another session's Classic. Remote injection works for letter keys through
`osascript ... keystroke` but not for non-character keys (`key code`) or mouse clicks.

Codex's uncommitted work (`classic_present.hpp`, `test_classic_present.cpp`, edits to
`os9/main.cpp`/`CMakeLists.txt` in its worktree) must be rebased onto `main` before it continues:
`os9/main.cpp` changed in the merge (overlays, picking, info window, markers, quick overlays).

## Codex progressive-resolution completion — 2026-11-05

The earlier note about Codex's uncommitted scheduling work is superseded. The continuation has
integrated the newer main-branch overlays, info windows, keys and quick-overlay test. It adds
explicit refinement effects/completions, coarse-to-target terrain replacement with exact final
coverage repair, bundled half-degree data in X11, and 32-row immutable GWorld presentation.
47 host tests, eight focused sanitizer tests, PowerPC/68k probes, OS 9 packaging/fork/size and
policy/format checks pass. Native OS 9 hardware and G4 half-degree runtime acceptance remain open.

Tiger: `GlobeVerify` (creator GlP3) is mounted at `<temporary-test-binary>` and was launched
alongside the other session's existing Globe. Its band-count title identifies the new code.
Observed launch: 180 bands, 2,750 ms settle, 734 ms copy, 20 ms max turn, 7 ms max continuation.
The current production package keeps name GlobeFloppy/creator Glb2, so launching it can route to
an older running app. Do not terminate shared Classic. Application-name/creator verification
changes are confined to an archived source copy under `<temporary-test-binary>`, not production.

See docs/progressive-resolution-plan.md, docs/os9-scheduling.md,
docs/perf/progressive-resolution-host.json and docs/perf/progressive-resolution-tiger.json.
Installable current image and MacBinary are retained in the Codex worktree's ignored
`build-scheduling-evidence/GlobeProgressive-1440.dsk` and `GlobeProgressive.bin`.

Final integration: progressive-resolution work fast-forwarded into local `main` at `afa7946`, preserving main through `7c245e9`. Identified Tiger Reset repeat: 180 bands, 2,757 ms settle, 732 ms copy, 17 ms max turn, 16 ms max continuation. The overnight progress file now records completed status. No GitHub push was performed.

## 2026-11-06 (day): OS 9 parity for the new rendering work

Done in core (shared, so OS 9 gets it by building): line layer (`lines_first`), the limb-cell final
pass, `RefinementJob::progress()`, `refine_progress()`, `draw_progress_bar`, `ZoomPreview`
(`include/globe/zoom.hpp`). Ported by hand to `src/platform/os9/main.cpp`: the zoom preview
(`zoom_preview`, `ZoomPreview` calls in `apply` and the step loop; refine over the preview with
`preserve_bitmap`) and the progress bar (`begin_bar`/`end_bar` in `capture` and `copy`, opaque, ten
rows stashed and put back). Built: `tools/build-os9.sh --floppy` (payload headroom 621,013 bytes) and
`tools/probe-retro68.sh` (0 failures, both CPUs). **Not run on OS 9**: Tiger was not free. To check on
Tiger: zoom with the wheel/Page keys after a settle (terrain should stay), watch the bar fill, and
note the title's turn/step times. Extra memory: line layer + zoom cache + overlay copy, about 1.8 MB
at 640x480, inside the 24 MiB partition (12 MiB minimum, unchecked). X11-only so far:
`--stats` `max_refine_turn_ms`, `--no-zoom-preview`, `--finish-refinement`.

## 2026-11-07: shared front-end code (#16), line list, tip8 and after

What the two front ends now share, so a change lands once (`include/globe/`): `ZoomPreview`
(`zoom.hpp`: which frame to scale from, `scale_frame`), `ShownFrame`/`frame_settled`/`bar_progress`
(`shown_frame.hpp`: overlays plus the progress bar on a copy), `RefineRunner` (`refine_runner.hpp`:
job start over a preview, cancel keeping the half-refined frame, step with city markers, moving
frame), `binding_for_char` (`app.hpp`: the shortcut keys), `same_orientation` (`projection.hpp`).
What stays per platform on purpose: the event loop and wake timers, getting pixels on screen
(`XPutImage` against GWorld + `CopyBits` in bounded slices and the presentation throttle), windows,
cursors, the info card, the clock. The `apply()` skeletons (about 15 lines each) and picking (about
8) are still duplicated; a coordinator with a host interface would buy little until a third platform
or more shared logic appears (issue #16 has the plan; the revised comment text was not posted,
see the 2026-11-06 notes). `tests/test_zoom.cpp` runs the runner headlessly (preview, refine over
it equals a direct render, cancel); #14 can build its lifecycle tests the same way.

Rendering since the last handoff: the line layer (lines first), the limb-cell final pass, the zoom
preview at every level, skipping the coarse level after a pure zoom (core: `settled_camera`),
the progress bar, a pixel list for the line composites (G4 first lines at 53 ms, whole refine 539
ms, `docs/perf/g4-2026-11-06.txt`). Details and measurements: `docs/refinement-order.md`.

Builds on shared G4: `<temporary-test-binary>` (everything above except the pixel list); newer tips as pushed.
Issue #34 holds the owner's idea of low/medium/high render tiers with first-run calibration.
Not verified on OS 9 (built and probed only): zoom preview, progress bar, shared runner and keys.

## 2026-11-07, end of session

State: local `main` is 19+ commits ahead of `origin/main` and **not pushed** (the owner pushes). Latest
code commit `a838974`, docs `a3a3aea`, then this note. Scratch builds: NetBSD `<temporary-test-binary>`
on shared G4 (lost on reboot); the OS 9 app at `MacOS9Lives/globe-tip9` on shared G4's OS 9 volume. Tiger and
OS 9 are shared with Codex, which had nothing in flight.

Decided this session: see [ADR 0002](adr/0002-rendering-shared-code-and-usability.md). Direction is now
usability (umbrella #35). GitHub: created #34 (render tiers, moved to v0.2/P1), #35 (usability epic)
and #36 to #46 (text foundation, pointer readout, labels, scale and degree labels, help overlay,
animated zoom, remembered view, image export, Mac OS cursors with the owner's rule, looks palette,
menu bar); commented on #16 twice (plan, then status); retitled #40 to the help overlay.

Owner's cursor rule (#44): the drag hand by default; a pointing hand over something clickable; the
ocean counts as clickable only while a country is selected.

Not done and why: the 8 ms OS 9 step (suspected zoom-cache copy, unverified; shared G4 was on OS 9 then),
render tiers (an evaluation ticket, no code), OS 9 full edition (no ticket yet), splitting the
`apply()` skeletons (not worth it, #16). The `x11_toolbar` test fails under sanitizers when the
machine is busy (a fixed 700 ms wait for the info window); wait for the window instead of sleeping.

Next suggested: #44, #45, #46 with the owner on bare OS 9 (they can only be judged there), then #34's
calibration experiment on host, G4 and Tiger, then #36 as the base for labels and the readout.
