# M1 plan: responsive globe

Status: **agreed 2026-11-01**. Scope follows `docs/v2-design.md` section 11 (M1) and the
decisions below. Not in M1: refinement stages and LOD (M2, M3), network I/O (M4), land fill and
terrain (M5), the Mac OS 9 front end.

## Decisions

| Question | Decision |
| --- | --- |
| Drag behavior | Virtual trackball. The camera keeps a full orientation; `make_camera` (center + heading) stays as a convenience for tests and initial placement. |
| Platforms | NetBSD/X11 only. OS 9 follows later on the same core; the Retro68 compile probe keeps guarding portability meanwhile. |
| Atlas content | Natural Earth 1:110m coastlines and borders (public domain), one file. |

## Steps (one narrow commit group each, tests first)

1. **Trackball camera.** Orientation-based camera, screen-to-sphere unprojection, drag as
   rotation between two sphere points, zoom as radius, renormalization against drift. Tests:
   drag round trip, no heading drift over many small drags, poles, zoom limits.
2. **Atlas format and builder** (`tools/atlas-builder`, laptop only). Versioned, fixed
   little-endian, quantized lon/lat, explicit polyline boundaries, deterministic output,
   provenance and attribution embedded. Prints byte size. Documented in `docs/atlas-format.md`.
3. **Atlas decoder** (`src/data`). Bounded, validating, no hidden allocation; rejects truncated,
   oversized and corrupt input. Mutation test over every byte position of a small atlas.
4. **Line rasterizer and layers** (`src/render`). Viewport clipping, integer Bresenham into
   RGB565, ocean disc, coastlines, borders, optional graticule, built on the M0 horizon clipping
   and subdivision. Golden-frame tests (small fixed frames, checked in as text).
5. **Elm core** (`src/core`). `Model` (camera, quality stage, window size), `Msg` (drag, wheel,
   key, resize, redraw), `update` returning `Effects`; deterministic with the clock passed in.
   Replayable message-trace test. Moving stage only; the stage enum exists for M2.
6. **X11 adapter** (`src/platform/x11`). Window, XImage conversion from RGB565 using the
   visual's masks and byte order, blocks while idle, redraws on change. Cross-built on the
   laptop; maul only runs it (display access is the owner's, see the weather project's notes).
7. **Measurement.** Injectable clock; per-frame time, segments drawn, peak memory, idle CPU,
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
