# Architecture (as built through M1)

The design intent is in `docs/v2-design.md`; this page says what exists.

```
include/globe/          public, portable interfaces (no OS headers)
  bytes.hpp             endian-explicit loads/stores
  geometry.hpp          Vec3, LatLon, unit-vector conversion
  projection.hpp        Camera, orthographic projection, horizon arc clipping, subdivision
  trackball.hpp         trackball drag, zoom, re-orthonormalization (the Trackball DragMode)
  framebuffer.hpp       RGB565 framebuffer, one allocation
  raster.hpp            exact-clipping lines, disc fill
  render.hpp            Scene (cached vertex vectors and conservative polyline caps), render_globe
  atlas.hpp, crc32.hpp  GATL reader (zero-copy, validating)
  app.hpp, trace.hpp    Model / Msg / update / Effects; Command / toolbar_state; DragMode (Turntable default, Trackball);
                        double-click, align-north, graticule-hidden-while-dragging; text traces
  embedded_atlas.hpp    the atlas compiled into the binary
src/core, src/data, src/render   implementations of the above
src/platform/common     portable adapter helpers: pixel_format (RGB565 -> any window-system layout),
                        frame timing summary, CLI parsing; clock.hpp (+ clock_posix.cpp)
src/platform/bench      globe-bench: headless trace replay with per-frame timing
src/platform/x11        globe-x11 (the only file that includes X11)
tools/                  atlas builder (Python), embed_atlas.py, Retro68 probe, policy check
```

## Rules the code follows

- **Core has no platform calls.** Nothing under `include/globe` or `src/{core,data,render}`
  includes an OS header or reads a clock. Time arrives inside messages (`time_ms`).
- **update() is a pure function** of `(Model, Msg)`; effects are returned as data (`Effects`) and
  run by the platform program. Any recorded sequence of messages replays to identical states
  (`tests/test_app.cpp`, `tests/data/trace1.txt`).
- **No exceptions, no RTTI, no hidden allocation in the hot path.** Decoding the atlas and
  rendering a frame allocate nothing (the first is tested; the framebuffer is allocated once).
- **Every external input is validated:** atlas bytes (`Atlas::parse`), trace lines
  (`parse_message`), command-line numbers (`parse_*_arg`), shapefiles (`build_atlas.py`).
- **Platform differences stay in adapters:** the X11 program asks the server's `XImage` what
  layout it wants and `pixel_format.cpp` produces it.

## Data flow of a frame

```
X event / one-shot wake --> Msg --> update(Model&, Msg) --> Effects{redraw, present_only, quit,
                                                                    wake_after_ms}
                                              |
                        (only if redraw)      v
        render_globe(Framebuffer&, Model.camera, Atlas, Palette, Layers)
                                              |
                convert_pixels (RGB565 -> server layout) --> XPutImage + XSync
```

`present_only` (a `Repaint`, from an expose or update event) skips the renderer: the platform shows
the pixels it already holds. Only a platform with no valid frame yet renders. Moving a window,
exposing it or clicking it never starts a render or restarts refinement; only a resize, a camera
change or a data change does.

`globe-x11` blocks in `select` until X input or a requested one-shot wake, then drains all queued
events (keeping only the latest pointer position) before drawing. Once refinement scheduling
finishes, it sleeps indefinitely until new input. There is no periodic timer.

## Measured behavior

See `docs/perf/README.md`. In short, rendering is cheap on a modern host (about 1 ms for a
640x480 frame) and about 60 times slower on the 467 MHz G4 (about 64 ms), which is the main open
question for M2 and beyond.

## Toolbar command state (2026-11-01)

Core actions use `Command` and `CommandIssued`, independent of keyboard input. The existing
`key TIME NAME` trace spelling is retained, including formatting, so recorded traces are unchanged.
`toolbar_state(Model)` returns a fixed array indexed by `Command`: all commands are visible;
layer checks reflect preferences even while the graticule is temporarily hidden during dragging;
the drag-mode check means Trackball. Align-north is enabled for a tilted or upside-down north
projection (tolerance 1e-12); zoom buttons are disabled at their respective radius limits.
This supplies state only; the native toolbar window remains a later task.

## Quality wake scheduling (2026-11-01)

`Effects::wake_after_ms` requests one wake; zero means no new request, preserving a platform wake
already armed. Ticks from superseded requests are harmless because `update` checks the current
stage clock. Accepted input restarts Moving, including commands with no redraw. Hover, rejected
input and a press alone do not: a drag begins only after the pointer moves more than `kDragSlopPx`
from the press, and a release without a drag leaves the model untouched. While a drag is active,
a Tick requests no more wakes; release after a drag restarts the delay.
After 150 ms Moving enters Settle; a separate wake at least 1 ms later enters Refine. These are
initial scheduling constants, not G4 interaction measurements. Resize has no input timestamp, so
its first Tick establishes the new clock; X11 supplies that Tick immediately after resize.

The initial model also waits for a first Tick to establish its clock. X11 sends that Tick at
startup. `tick TIME` traces cover the same transitions deterministically. No current stage enables
additional geometry, so transitions request no redraw and the existing pixel pins remain intact.
Bounded refinement rendering and cancellation of actual drawing jobs are still to be implemented
before terrain; this change supplies only stage scheduling, not a progressive renderer.

The Xvfb idle test checks two real timer wakes, final Refine state, zero idle frames and less than
50 ms CPU over two seconds. Before measuring it drains pending resize/expose events from the
script: those are legitimate redraws, not idle work. The old select-only wait could overlook X
messages already buffered by XSync, so zero reported idle frames alone did not prove that all
pending events had been handled.

## X11 readback verification (2026-11-01)

`globe-x11 --verify-pixels` is a test option: after each XPutImage/XSync it XGetImages the actual
window and compares every RGB pixel with the RGB565 framebuffer converted by the generic reference
converter. XGetPixel decodes server layout; only unused padding bits are ignored. This catches
incorrect fast packing, submission, byte order and row layout. Verification allocations and readback
are absent from normal operation and excluded from presentation timing. A mismatch produces an
error and exit code 3; `verified_frames` counts only entirely matching frames.

Private-Xvfb tests at depth 24 and 16 replay the resize trace from an odd 161x121 initial viewport.
Their Python runner checks successful process exit, actual depth, more than 100 presented frames,
and `verified_frames == frames`. Both every-frame and single-frame pixel corruption mutants fail:
a frame-count regular expression alone could wrongly accept the latter. These are host software
server checks, not proof of behavior on the G4 desktop or Mac OS 9.
