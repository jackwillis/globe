# Architecture (as built through M1)

The design intent is in `docs/v2-design.md`; this page says what exists.

```
include/globe/          public, portable interfaces (no OS headers)
  bytes.hpp             endian-explicit loads/stores
  geometry.hpp          Vec3, LatLon, unit-vector conversion
  projection.hpp        Camera, orthographic projection, horizon arc clipping, subdivision
  trackball.hpp         drag rotation, zoom, re-orthonormalization
  framebuffer.hpp       RGB565 framebuffer, one allocation
  raster.hpp            exact-clipping lines, disc fill
  render.hpp            render_globe(camera, atlas, palette, layers)
  atlas.hpp, crc32.hpp  GATL reader (zero-copy, validating)
  app.hpp, trace.hpp    Model / Msg / update / Effects; text message traces
  embedded_atlas.hpp    the atlas compiled into the binary
src/core, src/data, src/render   implementations of the above
src/platform/common     frame timing summary, monotonic clock, CLI number parsing
src/platform/bench      globe-bench: headless trace replay with per-frame timing
src/platform/x11        globe-x11, plus pixel_format (RGB565 -> any X visual layout)
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
