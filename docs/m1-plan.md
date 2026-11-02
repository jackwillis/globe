# M1 plan: responsive globe

Status: **agreed 2026-11-01**. Scope follows `docs/v2-design.md` section 11 (M1) and the
decisions below. Not in M1: refinement stages and LOD (M2, M3), network I/O (M4), land fill and
terrain (M5), the Mac OS 9 front end.

## Decisions

| Question | Decision |
| --- | --- |
| Drag behavior | Virtual trackball (chosen at planning). **Superseded 2026-11-01:** after hands-on use the owner found it rolled the globe and behaved oddly at the rim, so the default is now a north-up turntable; the trackball stays behind the `T` key. |
| Platforms | NetBSD/X11 only. OS 9 follows later on the same core; the Retro68 compile probe keeps guarding portability meanwhile. |
| Atlas content | Natural Earth 1:110m coastlines and borders (public domain), one file. |

## Steps (one narrow commit group each, tests first)

1. **Trackball camera.** *(done)* Orientation-based camera, screen-to-sphere unprojection, drag as
   rotation between two sphere points, zoom as radius, renormalization against drift. Tests:
   drag round trip, no heading drift over many small drags, poles, zoom limits.
2. **Atlas format and builder** *(done)* (`tools/atlas-builder`, laptop only). Versioned, fixed
   little-endian, quantized lon/lat, explicit polyline boundaries, deterministic output,
   provenance and attribution embedded. Prints byte size. Documented in `docs/atlas-format.md`.
3. **Atlas decoder** *(done)* (`src/data`). Bounded, validating, no hidden allocation; rejects truncated,
   oversized and corrupt input. Mutation test over every byte position of a small atlas.
4. **Line rasterizer and layers** *(done)* (`src/render`). Viewport clipping, integer Bresenham into
   RGB565, ocean disc, coastlines, borders, optional graticule, built on the M0 horizon clipping
   and subdivision. Golden-frame tests (small fixed frames, checked in as text).
5. **Elm core** *(done)* (`src/core`). `Model` (camera, quality stage, window size), `Msg` (drag, wheel,
   key, resize, redraw), `update` returning `Effects`; deterministic with the clock passed in.
   Replayable message-trace test. Moving stage only; the stage enum exists for M2.
6. **X11 adapter** *(done; tested under Xvfb and run by the owner on the G4's display)* (`src/platform/x11`). Window, XImage conversion from RGB565 using the
   visual's masks and byte order, blocks while idle, redraws on change. Cross-built on the
   laptop; maul only runs it (display access is the owner's, see the weather project's notes).
7. **Measurement.** *(done for render and pack; X submission on real hardware not yet measured)* Injectable clock; per-frame time, segments drawn, peak memory, idle CPU,
   time to first frame. Cross-build, push to maul, run. Results recorded in `docs/perf/`.

## Acceptance

- Host: CMake build, ctest, sanitizers, clang-format and clang-tidy clean.
- A recorded message trace replays to identical model states and identical frame bytes.
- On the G4: continuous rotate and zoom with the bundled atlas and no network; drag-frame
  latency measured and reported with its limits; the window does not redraw when idle.
- Atlas size and app binary size reported by a script, not promised.

## Risks

- Per-frame cost on a 467 MHz G4 is unknown until step 7; the rasterizer and atlas decimation
  levels may need tuning, and the doc forbids guessing: measure first.
- Display access on maul belongs to the owner; step 6 may need you to run the binary from the
  logged-in session.
- Trackball needs care with floating-point drift and with the horizon at high zoom.

## Status at the end of the unattended session (2026-11-01)

Met: all seven steps exist and are tested; a recorded session replays to identical frames (pinned
on host and G4); idle redraws are zero (Xvfb); atlas and binary sizes are reported
(`docs/perf/README.md`, `docs/atlas-format.md`).

Not yet met, and why:

- **Running on the G4's real display.** `globe-x11` is cross-built and pushed to maul
  (`/tmp/globe-x11`), links and starts there, then stops at "cannot open display": the remote
  account has no desktop access. Run it from the logged-in desktop session:
  `/tmp/globe-x11 --size 640x480`, or for timing,
  `/tmp/globe-x11 --size 640x480 --script /tmp/globe-bench_drag.txt --exit-when-done --stats`.
- **Drag-frame latency as the user would feel it.** Measured: render about 64 ms plus pack about
  17 ms at 640x480. Not measured: X submission and display on the G4.
- **"Responsive" on the G4.** At about 12 frames per second before submission, 640x480 is not
  comfortably interactive; see the candidate improvements in `docs/perf/README.md`.

## Update (2026-11-01, later)

The owner ran `globe-x11` on the G4's display and reported that dragging felt weird and slower than
v1. Changes made in response: north-up turntable dragging by default (trackball on `T`),
align-north (`N`), double-click to zoom and center, graticule hidden while dragging, and two
renderer speed-ups (drag-frame render 48 ms to 20.8 ms on the G4; see `docs/perf/README.md`).
Not yet confirmed by the owner: whether the new drag feels right, and real-display frame rate.

Owner feedback after trying the changes: the drag feels great and the grid is good; the coastline
toggle was dropped (coastlines are always drawn). See the addendum in
`docs/adr/0001-m2-to-m5-decisions.md`.
