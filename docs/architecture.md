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
  render.hpp            Scene (cached vertex vectors), render_globe(camera, scene, palette, layers)
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
X event --> Msg --> update(Model&, Msg) --> Effects{redraw, quit}
                                              |
                        (only if redraw)      v
        render_globe(Framebuffer&, Model.camera, Atlas, Palette, Layers)
                                              |
                convert_pixels (RGB565 -> server layout) --> XPutImage + XSync
```

`globe-x11` blocks in `XNextEvent` when idle and drains all queued events (keeping only the latest
pointer position) before drawing, so a burst of motion costs one frame.

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
