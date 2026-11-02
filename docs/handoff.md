# Handoff for Codex (unattended work) — Globe v2

Written 2026-11-01 by Claude; **refreshed 2026-11-02** (see "Current pass" below, which supersedes any
older statement in this file about branches, the remote, or this file being uncommitted). The file is
now committed. When you stop, append a dated note at the bottom (what you did, what you verified, what
you did not).

Read first: `docs/v2-design.md` (intent), `docs/architecture.md` (what exists), `docs/perf/README.md`
(measurements), `docs/m1-plan.md` (plan and status), `docs/toolchain.md` (what was verified where).

## Current pass (2026-11-02) — start here

**State.** Branch is `main` (the owner renamed `v2` to `main`; tag `v1` holds the old prototype). An
`origin` remote exists on GitHub but **do not push, merge remote branches, tag or force anything**; the
owner pushes. Tip at writing: see `git log -1`. 34 host tests pass. All Claude side branches are merged
and deleted. Read `docs/backlog.md` (ticket IDs below), `docs/project-notes.md` and the ADR addenda at the
end of `docs/adr/0001-m2-to-m5-decisions.md` before touching anything.

**Pins that must not change unless the change is intended and explained in the commit:**
replay hash `0x9b4a458633a32d9d` (`tests/test_app.cpp`), bench checksum `41c7adb508763323` for
`--repeat 2`, refined checksum `1228dcc860a92a77`, goldens in `tests/golden`. Terrain pixels change by
design in GL-27, so re-pin those with a stated reason and verify host and G4 agree.

**Behavior rules from the owner (binding, already implemented in the core):** a click without a drag
changes nothing; moving or exposing a window never re-renders (resend cached pixels); no relay; two
releases only (floppy, full). Done since your last pass: GL-6 (lines skipped per band: G4 refine
786 to 380 ms, identical pixels), click rules, five side branches merged, size reports (yours).

**Maul.** Owner's builds `/tmp/globe-x11-tip3` to `tip7` are theirs: never delete, overwrite or kill
them. Push test binaries under new names. Never compile on maul. Display is the owner's.

**Your tasks, in order** (files here do not overlap with what Claude will touch: Claude reviews and
stays out of `src/render/terrain.cpp`, `src/core/app.cpp` and the toolbar until you stop):

1. **GL-27 terrain look** (ADR addenda "terrain look", "layered refinement", "refine profile"):
   a. Flat gray land and flat ocean base for the Moving stage. Coastlines are polylines, not polygons,
      so fill from a coarse decimated elevation grid; time it on the G4 and report it. The settled
      frame must not depend on this base.
   b. Palette styles: runtime-selectable 256-entry tables for the terrain-color step; hypsometric default,
      cel-shaded second (flat height classes, optional latitude bias). Add a `CycleStyle` `Command`,
      toolbar button, trace verb and tests. Antarctica always ice white by a latitude rule (land south
      of about -60) applied before the palette lookup. Do not change the GELV v1 format; if an ice-flag
      bit needs a format change, write the proposal in the ADR and stop.
   c. Wire `fill_triangle_shaded` (merged, `src/render/raster.cpp`) into the terrain draw to smooth the
      1-degree grid, only if the G4 settled-frame cost is acceptable; report before/after G4 numbers
      (earlier microbenchmark: +78 ms at 1x, none at 2x). Mind the limb: the shaded path skips cells
      with a corner behind the horizon.
2. **GL-28 lakes in the builder:** download-verify the Natural Earth lakes layer recorded in
   `data/SOURCES.md` (ZIPs are in `~/scratch/globe-data/naturalearth/`; not committed), mark water cells
   at build time in the elevation pack builder without changing the v1 format; document the rule
   (50 percent area or cell center: the source check found 29 vs 30 Great Lakes cells, pick one and say why).
3. **GL-4 docs:** re-read README, `docs/open-work.md` and this file for stale wording ("M1", "uncommitted",
   test counts) and fix it.
4. **GL-31 design only:** write `docs/fetch-design.md` (no networking code): how to reuse `../weather`'s
   BearSSL, trust anchors, seed provisioning, DNS and HTTP framing; size cost of BearSSL plus anchors
   for the floppy edition; a pack validator (version, checksum, size caps) that cannot damage baked data;
   open questions for the owner.

**Not yours yet:** the center-biased dissolve and surface/theme refactors (GL-12 to GL-19), runtime
compression (measure first; numbers are in the ADR), anything networked, the maps app.

**Method reminders:** tests first, mutation-check new tests, portability probe (`tools/probe-retro68.sh`)
after core changes, `tools/check-policy.sh`, clang-format, narrow commits with your own
`Co-Authored-By:` trailer, report what was measured vs inferred vs not run (G4 display, OS 9 and CI have
never run). Cross-build recipe is in "Commands" below.

## State (historical, as of 2026-11-01; superseded by the section above)

Branch `v2`, tip `b80b78e` at the time of this update (plus the housekeeping commit after it), no
remote, working tree clean apart from this file. Built so far: portable core (projection, trackball
and turntable drag, atlas decoder, exact-clipping rasterizer, Scene-cached and polyline-culled
renderer, Elm-style `Model`/`Msg`/`update` with `Command`/`toolbar_state` and `Tick`/`wake_after_ms`
quality stages), an X11 program (`globe-x11`, with `--verify-pixels` readback), a headless benchmark
(`globe-bench`), and **17 tests**, all passing on the host (GCC with sanitizers, and clang) and, for
the relevant ones, on the G4. Tag `v1` and branch `main` hold the old prototype; do not touch them.

Owner feedback so far: the turntable drag **feels great**, the graticule is good, and the coastline
toggle was removed (coastlines are always drawn). The owner runs `/tmp/globe-x11-tip3` on the G4's
desktop. G4 numbers at 640x480 (render + pack): about 18 + 7 ms per drag frame; real-display
submission timing is still unmeasured.

## Rules (from the owner and the sibling `../weather` project; do not break)

- **Never compile on maul** (the Power Mac G4). Cross-build on the laptop, push with
  `~/.local/bin/maulctl push nb FILE NAME`, `chmod 755`, run with `maulctl nb '<cmd>'`. `/tmp` on
  maul is wiped at reboot; `push` drops the exec bit; pushing over a running binary fails with
  "text busy", so push under a new name and never kill the owner's running copy.
- **Display access is the owner's.** `globe-x11` cannot open the desktop display from the remote
  account. Anything needing a real display waits for the owner. Do not change access controls.
- **No remote, no pushing, no merging, no tags, no force-anything.** Commit narrowly on `v2`, ending
  each message with your own agent's `Co-Authored-By:` trailer. Never commit source ZIPs (hashes live in `data/SOURCES.md`).
- **Report faithfully:** say what was measured, what was inferred, what was not run. A failing test
  is reported with its output. Do not claim CI results (CI has never run; there is no remote).
- **Tests first, then mutation-check them:** break the new code on purpose and confirm a test fails
  (this session found several tests that passed while testing nothing). An "equivalent mutant"
  that no test can catch is fine if you say why.
- **Every first-party file** starts with `SPDX-License-Identifier: GPL-3.0-or-later` and
  `Copyright (c) 2026 Jack Willis`; `tools/check-policy.sh` enforces it (it checks *tracked* files,
  so `git add` new files before running it).

## Portability rules (Mac OS 9 is the next platform; see `docs/architecture.md`)

- `include/`, `src/core`, `src/data`, `src/render`: no OS or window-system headers, no clock, no
  `%ll`/`%z` in printf, no exceptions/RTTI, no `std::hypot`, no hidden allocation in the frame path.
  `tools/check-policy.sh` and `tools/probe-retro68.sh` (podman, Retro68 image, PowerPC and 68k
  compile) catch most of this. **Run the probe after touching core.**
- Platform code only under `src/platform/*`. `src/platform/common` is shared and neutral; clocks are
  one definition per platform.
- X11 headers define macros `None`, `KeyPress`, `Expose`; avoid those names in the core.
- G4 libm is slow: `sqrt` about 1.2 us, `sin`/`cos` 0.3 us, `atan2` 0.6 us, `floor` 0.11 us. Keep
  them out of per-segment and per-pixel loops; measure instead of guessing.

## Commands

```
cmake -S . -B build -G Ninja -DGLOBE_SANITIZE=ON && cmake --build build
ctest --test-dir build --output-on-failure              # 15 tests (Xvfb ones need xvfb-run)
cmake -S . -B build-clang -G Ninja -DCMAKE_CXX_COMPILER=clang++ && cmake --build build-clang
cmake -S . -B build-rel -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build-rel
git ls-files -co --exclude-standard '*.cpp' '*.hpp' | xargs clang-format --dry-run --Werror   # 22.1.8
git ls-files -co --exclude-standard 'src/*.cpp' | xargs clang-tidy -p build --quiet
./tools/probe-retro68.sh ; tools/check-policy.sh
./build-rel/globe-bench --convert --repeat 3 tests/data/bench_drag.txt     # JSON; --reference, --layers gcb
```

G4 cross-build (headless programs use `-static`; the X11 program is dynamic):

```
R=$HOME/scratch/netbsd-xc; X=$R/tools2/bin/powerpc--netbsd-g++
python3 tools/embed_atlas.py data/atlas.gatl /tmp/embedded_atlas.cpp
$X --sysroot=$R/sysroot -std=c++20 -O2 -static -fno-exceptions -fno-rtti -Iinclude \
   -Isrc/platform/common src/platform/bench/bench.cpp src/platform/common/*.cpp \
   /tmp/embedded_atlas.cpp src/core/*.cpp src/data/*.cpp src/render/*.cpp -o globe-bench
```

(Tests take the repo root as argv[1] and read `data/` and `tests/`; on maul they live in
`/tmp/globe-root`. Re-push data files there if you run tests on the G4.)

Pins that must be re-pinned deliberately, with an explanation in the commit, if pixels change:
`tests/golden/*.txt`, the replay hash in `tests/test_app.cpp` (`0x9b4a458633a32d9d`), and the bench
checksum (`41c7adb508763323` for `--repeat 2`, equal on host and G4; it was `485cbfbc9fef11a3` before the
coastline toggle was removed from the trace). Verify any re-pin on the G4.

## Independent tasks: status

Done by Codex on 2026-11-01 and re-checked by Claude (17/17 tests, G4 benchmark rerun: render 18.1 ms,
pack 6.9 ms, checksum unchanged): faster pixel packing (`c6f47c8`), `draw_line` floor/clip overheads
(`37dcb61`), polyline culling (`c7ee145`), X11 pixel readback tests (`a08e22f`). **Still open:** task 5,
scaling measurements: `globe-bench --size` at 320x240, 640x480, 800x600, 1024x768 on host and G4,
written up in `docs/perf/`.

## Decisions are made: see `docs/adr/0001-m2-to-m5-decisions.md` (committed)

D1 to D8 were decided with the owner on 2026-11-01. Work in this order; each item is a narrow commit
group with tests first and a mutation check, measured on the G4 where speed matters.

1. **(DONE, `e31f10b`)** **`Key` to `Command` rename + `toolbar_state(Model)`** (core only). Update trace verbs only if you
   keep the old `key` verb as an accepted alias (recorded traces in `tests/data/` must keep working;
   re-pin the replay hash only if behavior changes, which it should not). `toolbar_state` is a pure
   function returning, per command, `{visible, enabled, checked}` (layer toggles checked, drag mode,
   align-north enabled when north is not up). No platform code.
2. **(PARTLY DONE: scheduler in `4d5c35b`; bands, cancellation and the quality profile remain)** **Wake requests and staged refinement (D1).** Add `Effects::wake_after_ms` (0 = none) and a
   `Tick{time_ms}` message. After input stops, `update` requests a wake at the Settle delay (start at
   150 ms, tunable constant), moves `QualityStage` Moving to Settle to Refine on Ticks, and any input
   returns to Moving and drops stale refinement. The X11 program arms a one-shot timer through the
   `select` timeout it already uses in `wait_idle` (no periodic polling; the idle test must still show
   0 frames and ~0 CPU). Refinement drawing is done in bounded bands so input stays responsive; add a
   trace-replay test for the stage sequence. Nothing is drawn differently yet except what the stage
   enables; terrain arrives in item 4.

   **Owner's suggestion (2026-11-01): the Refine step can also set a global resolution.** Yes:
   add a small pure `RenderQuality quality_for(const Model&)` (or a `Model::quality` field updated in
   `update`) holding the global detail knobs the renderers read, for example the on-screen chord
   tolerance in pixels (`kMaxChordPx`, now a constant 3), the elevation grid level to use (item 5),
   and which layers/terrain are drawn. Moving gets the coarse profile, Refine the fine one; zoom can
   adjust it too. Keep it a **derived value of (stage, zoom, budget), not a payload carried by
   `Tick`**, so traces still replay deterministically. Optional extension toward the design doc's
   "dynamically reduce LOD when the frame budget is missed": a platform-supplied `FrameDone{ms}`
   message lets `update` lower the quality level when frames run over budget (time still enters only
   through messages). Finer detail costs G4 time, so measure each knob before enabling it, and keep
   per-region LOD (M3) out of scope here: this is one global setting.
3. **(SOURCE VERIFIED, builder not started)** **Elevation data (D2, D3): format `GELV` v1.** The source is
   recorded in `data/SOURCES.md` (NOAA ETOPO 2022 60 arc-second ice surface NetCDF, CC0, 478,290,125
   bytes, SHA-256 `74bbc5c2...b197b6`); the file lives at `~/scratch/globe-data/etopo/` on the laptop
   (outside the repo; verify the hash before use). **z row zero is the south pole** despite the
   north-origin geo-transform; do not infer ordering from either. Original task text follows. Same pattern as `GATL` (`docs/atlas-format.md`):
   fixed little-endian, versioned header, CRC-32, exact-size check, strict bounded zero-copy decoder
   in `src/data`, builder in `tools/` (a modern machine may use extra tooling such as numpy or GDAL
   there; the runtime must not), format doc, cross-language fixture, and the full mutation treatment.
   Proposed layout (adjust with reasons): header with width, height, lon/lat extent (global for now,
   fields present so regional packs fit later), a 256-entry int16 lookup table mapping the one-byte
   cell value to meters (so one format spans ocean depths to mountain heights and any resolution),
   then `width*height` bytes. **Before writing the builder, verify and record** the source dataset
   (NOAA ETOPO 2022 or another public-domain grid), its license, file format, size and SHA-256 in
   `data/SOURCES.md`; do not commit source files; report the numbers you actually saw. Build the 1
   degree pack (about 65 KB) and keep multi-resolution packs possible (D2b).
4. **Terrain renderer (D2, D6).** During Refine, draw forward-projected colored cells (each grid
   cell's corner vectors are precomputed in the `Scene`-style cache, projected, filled as small
   quads), colors from a 256-entry hypsometric table (ocean depth blues, land greens to browns to
   white), **black borders** over it, coastlines as now (always drawn; the owner removed the coastline toggle). Do not do per-pixel inverse projection (it
   costs microseconds per pixel on the G4; the `sqrt` alone is 1.2 us). Moving stage stays outlines
   only. **Measure on the G4 before and after** (`globe-bench`, add a refine-stage metric) and report
   the cost of a full refine frame; if it is too slow for a settled frame, say so with numbers and
   stop rather than improvising a different approach. Re-pin goldens deliberately.
5. **Data pack import (D2b).** `tools/` builds packs at other resolutions; the app can import a pack
   from a local file path (CLI option first), validates it with the same decoder, and selects the best
   resolution for the zoom. No networking in this task.
6. **Toolbar window on X11 (D4) and cursor feedback** using `toolbar_state`. Cursor: add a pure
   `cursor_for(Model, pointer)` (open hand over the globe, closed hand while dragging, arrow
   elsewhere; same in both drag modes), mapped per platform (see the ADR addendum; X11 has no
   open/closed-hand core cursor, so use a fallback or Xcursor if present). Toolbar window: separate utility window, hand-drawn buttons
   (Xlib only; remember the `None`/`KeyPress`/`Expose` macro clashes stay in `src/platform/x11`).
   Pointer hit-testing for the buttons belongs in a small pure function with tests.
7. **Country polygons, picking and popups (D5)**, after 4: format `GPOL`, point-in-polygon on the
   sphere (holes, antimeridian, poles), an `InfoCard` produced by the core, a native-style scrolling
   window per platform (X11: hand-drawn scroll bar). Cities (`GPLC`) after that.
8. **Mac OS 9 proof of life (D7)** may proceed in parallel after item 2: Retro68 build in the
   container (see `../weather/Makefile` target `os9` and `os9/CMakeLists.txt` for the pattern; do not
   copy Apple headers or binaries into the tree), window, `GWorld` through `pixel_format`,
   `WaitNextEvent` sleep driven by `wake_after_ms`, a `Microseconds()`-based clock behind
   `clock.hpp`. You can build and link it; **you cannot run it**, so say "built, not run" and leave
   hardware acceptance to the owner.
9. **Floppy size report (D8):** a script that builds the designated `floppy` configuration (app plus
   atlas plus 1-degree elevation, packed the way a disk image would be) and prints sizes; add it to
   the CI workflow as a report. A budget check applies to that build only.

Remaining open choices you may decide and record in the commit message, with measurements: Settle
delay, exact elevation quantization table, cell-fill method details, popup layout.

## Known gaps and honest caveats

- CI (`.github/workflows/checks.yml`) has never run; no remote. The `pipx` pins for clang-format and
  clang-tidy and the Retro68 job are unverified as CI jobs.
- X11 pixels are now verified under Xvfb at depths 24 and 16 (`--verify-pixels`); a real display is still the owner's to try.
- Real-display frame rate on the G4 is unmeasured. Measured so far: drag-frame render 20.8 ms plus
  17 ms pack at 640x480 (`docs/perf/README.md`); `globe-x11 --script ... --stats` run from the
  owner's desktop session would give X submission timings.
- `subdivide_arc` is only used by tests now (kept as an exact reference).
- Intermediate commits may not build on their own (CMake wiring came in later commits); the tip does.
- On maul after the 2026-11-01 cleanup (wiped at reboot): `/tmp/globe-x11-tip3` (the owner is running
  it; never overwrite or kill it, push under a new name), `/tmp/globe-bench`, `/tmp/globe-bench_drag.txt`
  and `/tmp/globe-root/{data,tests}` (atlas, golden frames, `tiny.gatl`, `trace1.txt`: enough to run the
  render, app, atlas and pixel tests there). Old `globe-pass-*` artifacts were removed.

## Notes from the next agent

(append dated entries here)

### 2026-11-01 — Codex unattended pass

Committed narrowly on `v2` (no remote/push/merge/tag):

- `e31f10b`: `Key` -> `Command`, `KeyDown` -> `CommandIssued`, `command_name`, pure
  `toolbar_state(Model)` array indexed by Command. Existing `key TIME NAME` traces and formatter
  spelling retained. Layer preferences checked even while hidden during drag; Trackball check;
  align-north enabled for tilted/upside-down north; zoom disabled at limits. X11 changes are only
  mechanical action dispatch, not a toolbar window.
- `4d5c35b`: scheduler portion of item 2. `Tick`, `wake_after_ms`, 150 ms Moving->Settle then
  separate 1 ms Settle->Refine turn, input invalidation, harmless stale ticks, no repeated wakes
  for a held drag. X11 uses select with a one-shot deadline, including idle measurement (removed
  its old 100 ms polling). Initial/resize stage clocks are established with a Tick because Resize
  has no timestamp. `wake_after_ms == 0` means no new request, not cancellation of an existing
  platform wake. No new detail is enabled and no stage-transition redraw is necessary yet.

**Item 2 is not fully complete:** bounded progressive rendering bands and cancellation of actual
render jobs still need implementation before terrain. No renderer/framebuffer/raster changes were
made here. Do not interpret the stage enum reaching Refine as terrain being implemented. Elevation,
pack import, actual native toolbar, picking, OS 9 application and floppy report are untouched.

Verified final state:

- GCC ASan/UBSan full ctest 15/15; clang full ctest 15/15, including private Xvfb depth 24/16 and
  strengthened idle check. Replay hash remains `9b4a458633a32d9d`; no pins changed.
- Tests written before implementation (failed compilation for missing new API), then nine toolbar
  mutants, seven scheduler mutants, and two X11 timer mutants all rejected. Mutants were compiled
  from temporary files under `/tmp`, not committed. Representative intentional-failure outputs:
  `CHECK failed: initial[i].visible`; `CHECK failed: !state(m, Command::ZoomIn).enabled &&
  state(m, Command::ZoomOut).enabled`; `CHECK failed: m.stage == QualityStage::Moving &&
  fx.wake_after_ms == 0` (held-drag mutant). Missing-X11-wake mutant failed
  `assert stats["quality_stage"] == 2` with stage=0/wakes=0. Periodic-wake mutant failed
  `assert stats["wake_ticks"] == 2` with 1,888 wakes, even though idle_frames=0.
- Format, clang-tidy over all src, policy and diff whitespace checks passed. New Python test was
  staged before policy check. Retro68 PowerPC and 68k core/probe compile passed (built, not run).
- Host depth-24 Xvfb final GCC run: two wakes, final Refine, 0 idle frames, 0.104 ms CPU over two
  seconds. Details in docs/perf/README.md. No G4 correctness/performance runs this pass, no real
  display interaction, no CI claim. No files or running programs on maul touched.

Failures encountered and resolved (not concealed):

- Sandbox-only initial ctest: LeakSanitizer output `LeakSanitizer has encountered a fatal error`
  and `LeakSanitizer does not work under ptrace`; Xvfb program output `cannot open display`.
  Podman initially failed `set sticky bit ... /run/user/1000/libpod: read-only file system`.
  Approved execution outside the sandbox allowed full leak checks, Xvfb and local Retro68 container.
- Initial toolbar implementation used the wrong world north axis. Four assertions failed:
  initial AlignNorth enablement and three post-alignment disabled checks. Corrected to world +Y;
  full tests and upside-down mutation check pass.
- Stronger idle test initially failed `assert stats["idle_frames"] == 0` with idle_frames=1,
  wake_ticks=2, stage=2, CPU=4.285 ms. Cause: pending script-generated resize/expose events in Xlib's
  buffer; the old select-only loop could overlook them. The new event loop handles buffered events;
  measurement now drains script completion before counting quiet idle, and the test passes.

HANDOFF-codex.md remains deliberately uncommitted. Next work: bounded refinement job/band design
and tests to finish item 2, then source verification before any elevation builder. Stage timing is
an initial choice, not tuned or measured for interaction on the G4.

### 2026-11-01 — Claude's review of the Codex pass

Checked by re-running, not by reading the summary: `ctest` 15/15 under GCC ASan/UBSan and clang;
`x11_smoke_24/16` and `x11_idle` stable over 5 consecutive runs; clang-format, clang-tidy, policy and
the Retro68 PowerPC and 68k probe clean; `git log` shows the two commits as described, with Codex's
own trailer and an honest "G4 not run". **On the G4** (NetBSD/macppc, cross-built, run as
`/tmp/globe-test_app`): the replay test passes and reproduces `9b4a458633a32d9d`, so the new core
changes did not alter results there either. Codex's claims held up.

Findings and follow-ups (none blocking):

- The `Effects::wake_after_ms` contract was ambiguous: every pointer move returns a wake request, which
  is only harmless if a platform keeps one replaceable deadline (X11 does). The comment in
  `include/globe/app.hpp` now says so. The OS 9 front end must do the same with `WaitNextEvent`.
- `Resize` has no timestamp, so Codex added `stage_needs_time` to wait for the first timestamped
  message. It works and is tested; a cleaner fix is an optional time on `resize` in the trace format
  and the message. Not urgent.
- Item 2 is still open: bounded progressive bands, cancellation, and now the quality profile above.
- Not verified by anyone: real-display behavior of the new scheduler on the G4 (needs the owner), and
  CI.

### 2026-11-01 — Codex independent tasks 1–4 and elevation source check

Scope confirmed by Jack in this chat: **independent tasks 1–4**, not ordered implementation items
1–4. Completed those four tasks, checked the elevation source, and stopped before any GELV
builder/decoder/pack or terrain implementation. The earlier ordered item 2 (bounded refinement
bands/cancellation/global quality profile) remains open and was not changed in this pass.

Five narrow commits on `v2`, each with Codex's trailer:

- `c6f47c8`: faster pixel packing. Fixed-width 32-bit paths, adjacent-color reuse, alignment-safe
  native word stores using memcpy/assume_aligned, byte fallback for unaligned destinations,
  native RGB565 row copy or byte swap. Generic converter retained as reference. Existing exhaustive
  eight-bit channel/rejection tests retained; new tests compare all RGB565 inputs and flat runs,
  both byte orders, unusual masks, odd widths, padded/unaligned rows and guard bytes. Shared
  converter now included in Retro68 probe.
- `37dcb61`: inline bounded floor and skip Liang-Barsky only for endpoints already in +/-1e6.
  Integer-boundary/out-of-box tests ran against the old code first. Exact-clipping tests retained.
- `c7ee145`: Scene caches conservative spherical caps (two up-front allocations, no frame
  allocation); dot-product horizon culling. Caps >= a hemisphere or degenerate means disable
  culling; tangent cases retain a 1e-12 margin. Added uncullled cached reference option and actual
  cull counter. Tests compare cached culled/uncullled/uncached frames over 40 deterministic real
  cameras, including high zoom, plus synthetic layer combinations, and require actual culling.
- `a08e22f`: opt-in `--verify-pixels` reads every presented window frame with XGetImage and
  compares RGB against the generic converter. Two new private-Xvfb tests at depths 24/16 replay
  resize from odd 161x121 dimensions. Python runner requires successful exit and all frames
  verified; a frame-count regex alone could accept one bad frame among 120 good ones.
- `b80b78e`: source record only in `data/SOURCES.md`; see exact source facts below.

Measured, not extrapolated (640x480, current 376-message trace, --convert --repeat 2, 750 frames,
G4 cross GCC 12.5.0 -O2 -static; one run per row):

| Variant | Render mean ms | Pack mean ms |
| --- | --- | --- |
| Before this pass | 20.7097 | 17.0486 |
| Byte-store packing attempt | 20.7951 | 10.9548 |
| Above plus line optimization | 18.6214 | 11.1817 |
| Native-word packing plus line optimization | 18.6814 | 6.9144 |
| Above plus cap culling | 17.7811 | 6.9910 |

The first packing attempt missed the under-10-ms target; the final path meets it. Culling is a
modest one-run saving, not a statistical claim. Scene storage rises 226,968 -> 241,912 bytes
(+14,944), with G4 construction 13.4267 -> 15.1956 ms in the preceding/final runs. Combined host
render/pack means were 0.2898/0.1890 ms. Raw JSON and separate notes are in
`docs/perf/pass-20261101-*`; README links the reports. No real-display submission timing measured.

Verified:

- Final GCC ASan/UBSan suite **17/17**; final clang suite **17/17**, including new readback tests,
  old smoke tests and quiet idle. Tests run outside sandbox for sanitizer/Xvfb access.
- On G4, cross-built **render**, **app**, **pixel_format** tests all print `ok`; app reproduces
  `9b4a458633a32d9d`. Goldens unchanged. All benchmark variants on both hosts reproduce current
  checksum `41c7adb508763323`. No pixel pins re-pinned.
- Four final packing mutants rejected (native endian, unaligned endian, stale color cache,
  RGB565 endian); two line mutants rejected (negative truncation, dropped safety-box clipping);
  three culling mutants rejected (disabled culling, undersized cap, wrong hemisphere); every-frame
  and single-frame pixel corruption rejected at **both** Xvfb depths (four readback runs).
- clang-format clean; clang-tidy all src inspected, then both flagged files fixed and rechecked
  clean; whitespace and staged policy checks passed for each code commit, including the new Python
  runner. Retro68 PowerPC/68k core and pixel converter compile successfully: **built, not run**.
- Source binaries/fixtures on maul use new `/tmp/globe-pass-*20261101*` names and new
  `/tmp/globe-pass-root-20261101`. Nothing compiled on maul; no owner process killed, no display
  permissions changed. Git main/v1 untouched; no remote/push/merge/tag, and no source archives tracked.

Failures encountered, with representative output:

- Tests first: pixel tests initially failed compilation with `convert_pixels_generic was not
  declared`; cap tests likewise reported missing `PolylineCap`/`polylines_culled` and render
  overload. New Xvfb readback tests initially failed with CLI usage (missing --verify-pixels).
- Existing Scene test initially produced four `CHECK failed: sa.segments == sb.segments && ...`
  failures after culling. Considered-segment counts intentionally decrease; changed this assertion
  to `sb.segments <= sa.segments`, keeping exact pixels, visible segments and rasterized lines equal.
- Packing mutants: `CHECK failed: fast == generic`. Negative-floor mutant: `mismatch line 2 ...
  at 1,0` and boundary assertion failure. Safety-clip mutant: `CHECK failed: count(fb, kInk) == 8`.
  Disabled-culling mutant: `CHECK failed: culled > 0`; undersized cap: visible/line/pixel mismatches.
- Readback mutants: depth 24 `pixel mismatch at 0,0: got 80c18 expected 80c19`; depth 16
  `got 862 expected 863`. Single-frame mutation left `verified_frames: 120` out of 121 and exited 3,
  rejected by the Python runner. An outer mutation orchestration assertion initially looked only
  in stderr; child diagnostics were captured in stdout here. Checking combined output resolved
  that harness assertion; actual application mismatch/exit behavior was already correct.
- clang-tidy: `bugprone-implicit-widening-of-multiplication-result` on `out[2 * x]` and
  `bugprone-unchecked-optional-access` in verify_window. Fixed by pointer advancement and checking
  the optional locally; targeted rerun clean.
- Final G4 fixture transfer first failed `Connection timed out during banner exchange` /
  `Connection to UNKNOWN port 65535 timed out`. Retry completed; all three hardware tests pass.

Elevation source check completed, then stopped:

- Official NOAA NCEI ETOPO 2022 v1 **60 arc-second Ice Surface** global NetCDF; authoritative
  dataset metadata explicitly CC0-1.0 worldwide. Exact links, attribution and caveats committed
  to `data/SOURCES.md`.
- Actually downloaded **478,290,125 bytes** to
  `/tmp/ETOPO_2022_v1_60s_N90W180_surface.nc`. SHA-256 computed during transfer and independently
  repeated with sha256sum: `74bbc5c23e85188d93f0beba8116babf69e1a8f160e8dc9f053bf8df01b197b6`.
- Inspected actual HDF5/NetCDF-4 classic model header with h5py in a temporary isolated environment:
  z float32, 10,800 x 21,600, gzip, area/cell registered; meters positive up, EGM2008/EPSG:3855,
  nodata -99999. Latitude variable increases south -> north: **z row zero is south** despite the
  north-origin GDAL GeoTransform. Do not infer array ordering from the filename or GeoTransform.
  Metadata and sample cells read, not a full extrema/missing-value scan. THREDDS has stale ETOPO1
  description text; source identity follows the official ETOPO2022 product/metadata and exact file.
- No source file committed. No GELV format/builder/decoder, quantization, elevation pack, terrain,
  import, toolbar window, OS 9 application, scaling task 5 or CI execution this pass.

This handoff remains intentionally uncommitted. Stop point reached as requested; future work must
not treat the elevation source check as a completed elevation implementation.

### 2026-11-01 — Claude: housekeeping after the second Codex pass

Re-verified Codex's results (above). Removed stale builds and `globe-pass-*` artifacts from maul
(kept the running binary), merged Codex's fixtures into `/tmp/globe-root`, moved the ETOPO file from
`/tmp` to `~/scratch/globe-data/etopo/` (hash re-checked after the move) and updated `data/SOURCES.md`.
Owner decisions recorded in the ADR addendum: drag feel confirmed, coastline toggle removed, cursor
feedback goes with the toolbar. Suggested next pass: finish ordered item 2 (quality profile,
bounded bands, cancellation), then the GELV builder (item 3), or independent task 5 in between.

### 2026-11-01 — Codex handoff reread

Read the refreshed handoff, ADR addendum and source-record context. Current tip is `831e149`.
Noted the confirmed drag feel, cursor feedback deferred to the toolbar milestone, ETOPO source
moved to `~/scratch/globe-data/etopo/`, and removal of old maul pass artifacts. Next ordered work
remains item 2's quality profile, bounded bands and cancellation; no implementation resumed here.
Tests were not rerun during this read-only review of project state.

Small stale wording remains above: the ctest command comment says 15 rather than 17 tests, and
Known gaps repeats the old 20.8 + 17 ms figures rather than the current approximately 18 + 7 ms.
Also, elevation row zero is the southernmost cell center (about -89.991667 degrees), not exactly
the south pole. Use the exact coordinates in data/SOURCES.md when implementing the builder.
This note is the only change; the handoff remains uncommitted.

### 2026-11-01 — Claude: terrain look decisions and a side branch

Owner tried `globe-x11-tip4` (tip `9c62abc`). Feedback and decisions are in the ADR addendum
(`e951ce2`); read it before touching terrain. In short: 1-degree cells look chunky; no Great
Lakes; deserts not deserty; land should be gray while moving. Direction: terrain is a **stack of
composited refine steps** (flat base, terrain color, optional relief, lines, labels), with
**styles as palette tables** (hypsometric default, cel-shaded second, toolbar button plus a
`CycleStyle` command), Antarctica always ice white (latitude rule first, GELV ice-flag bit
reserved, glaciated areas later), satellite imagery not planned.

Side branch `claude/smooth-quad` (worktree `../globe-wt-a`, 2 commits, **not merged**):
`fill_triangle_shaded` in `raster.{hpp,cpp}` with `tests/test_shaded.cpp` (one word added to the
test list in `CMakeLists.txt`), and a Natural Earth glaciated-areas record in `data/SOURCES.md`.
Host tests pass; not timed on the G4; not wired into terrain. Claude will rebase and merge after
your pass is committed, so do not edit `raster.*` in the meantime, and do not merge it yourself.
Open for whoever is next, in order: G4 micro-benchmark of the shaded fill; flat gray/ocean base
for the Moving stage (time it); `CycleStyle` and palette tables; Antarctica latitude rule; lakes
(download, hash and measure the Natural Earth lakes layer first); desert tint.

Housekeeping: removed Claude's scratch worktree and temporary embeds. `/tmp/globe-x11-tip4` is on
maul next to the owner's `tip3`; do not delete either.

### 2026-11-01 — Claude: dissolve order, shaded-fill timing, profile in progress

ADR addendum (latest commit on `v2`) now records: (1) **refinement order is a center-biased cell
dissolve** (hash-based per-cell randomness, about 16 passes, one weight `w`, draws over the flat
base; replaces the row-band structure; final image must be order-independent and tests must pin it);
(2) **shaded-fill timing on the G4** (flat 107.5 ms vs shaded 185.6 ms at 1x, 106.4 vs 99.4 at 2x;
checksums match; branch `claude/smooth-quad` commit `0ecc5fd`, unmerged, benchmark
`src/platform/bench/shade_bench.cpp`); (3) the flat fill is only about a fifth of the ~0.58 s refine
frame, so projection and per-cell setup probably dominate. A fork is profiling this on branch
`claude/refine-profile`; its numbers decide the speed-up work (likely culling hidden corners
before projection). Also branch `claude/lakes-source` (Natural Earth lakes record, unmerged).
Nothing from these branches should be merged by Codex; Claude rebases them after your pass.
Do not start the dissolve or the shaded wiring until the profile numbers are in the ADR.

### 2026-11-01 — Claude: behavior rules for both front ends, compression direction

Read the latest ADR addendum ("behavior rules for every front end; compression direction"). For
the OS 9 front end in particular: (1) a click without dragging must not leave the settled stage
or restart refinement (the core currently does; the core fix is pending and will land with a test,
so do not work around it in the front end); (2) window drags, update events and exposes must
resend existing pixels and never re-render. Implement the OS 9 event handling to match rule 2 now.
Compression is design-only so far; do not add a compressor or change a v1 format before the
measurement described there is done and recorded.

### 2026-11-01 — Claude: refine profile results

Read the ADR addendum "refine profile on the G4". Summary: the refine job is 786 ms with terrain
and 456 ms without; the line layers are re-walked for every one of the 30 bands (about 16 times the
28 ms one-pass cost) and are the biggest cost; terrain phases total about 215 ms; corner culling is
not worth doing. Fix order: lines once per refine, band-limited cell scan (or the dissolve), cheaper
fill. Branch `claude/refine-profile` holds the profiling program (unmerged; do not merge it yourself).
Take this into account before starting any terrain or dissolve work.

### 2026-11-01 — Claude: data policy and a separate maps app

Read the latest ADR addendum ("data policy and a separate maps app"). Changes to your plan: the
globe stays offline with baked, compressed terrain; only vector packs are importable (versioned and
checksummed, rejected cleanly if unknown); there is no relay and no in-app download manager;
streets and detailed maps will be a separate maps application sharing libraries and pack formats.
Do not add networking code to the globe. When structuring OS 9 code, keep the shared libraries free
of anything that assumes the globe is the only program.

### 2026-11-01 — Claude: two releases

The owner wants exactly two releases: **floppy** (fits 1.44 MB, coarse baked terrain, 110m vectors,
OS 9) and **full** (everything else). Same source and pack formats; a build-time selection of data
and front ends, not a configuration matrix. See the ADR addendum "two releases". Keep your floppy
size report able to report both editions.

### 2026-11-01 — Claude: fetch tool inside the app; compression numbers

The owner now wants a **fetch tool inside the app**, in scope for v2, with TLS on OS 9 solved
using `../weather` as the seed (BearSSL vendored, trust anchors, seed provisioning, DNS, HTTP
framing; see the ADR addendum "fetch inside the app"). This supersedes my earlier "no networking in
the globe" note above, and corrects my claim that OS 9 cannot do modern TLS. Do not start
networking code yet; first read weather's `docs/OS9-HARDENING-PLAN.md`, `README.md` and
`third_party/README.md`, and write down (do not implement) how to reuse it, the floppy size cost of
BearSSL plus anchors, and a pack-validation design. Compression experiment numbers are in the ADR
(branch `claude/compression-experiment`, unmerged).
