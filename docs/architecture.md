# Architecture and next steps

`src/planet.c` owns the portable renderer; `src/main.c` handles command-line
validation and PPM output. RGB bytes have no native-endian packing assumptions.
Frame dimensions are limited to 4096 per axis, with allocation overflow checks.

Longitude zero, latitude zero maps to (0,0,1). Positive longitude points toward
positive x, north toward positive y. Camera-visible geometry has z >= 0.
Yaw rotates about y, followed by pitch about x. Orthographic projection maps
x and y to framebuffer coordinates, reversing y for screen coordinates.

Synthetic curves are sampled every two degrees and represented as straight
chords. Chords crossing z=0 are clipped by interpolation before projection.
This approximates curved spherical geometry; horizon points can lie slightly
inside the ideal circular limb. Future coastline preprocessing must document
sampling/error tolerances. Liang-Barsky clipping precedes integer Bresenham
rasterization. Invalid non-finite line coordinates are rejected.

Determinism means identical inputs within a build produce identical bytes.
Cross-platform floating-point differences are possible; the tested Linux and
G4 frame matched byte-count and POSIX checksum, but universal cross-platform
bit identity is not promised.

## Remaining implementation plan

1. Coastline stage: preprocess Natural Earth 1:110m data on Linux; document
   provenance; define a versioned, fixed-endian binary format with explicit
   polyline boundaries; test malformed inputs and allocation limits.
2. X11 stage: keep the framebuffer independent; use XPutImage with detected
   masks, byte order, bits per pixel and stride. Add event-driven rotation,
   zoom, resize and clean exit. Display authorization remains unavailable to
   the remote account; do not change access controls.
3. Measurement stage: separate transform, raster, total render, conversion,
   submission, synchronization proxy, and frame pacing. Use monotonic clocks
   and runs <=60 seconds; report distributions and instrumentation limits.
4. Optimize only after baseline measurements, retaining the reference path.

Use native builds on each platform. The Makefile uses common GNU/BSD syntax.
Start with -O2 -std=c99 and warnings; avoid fast-math and architecture-specific
paths. Remote artifacts belong under the approved account's ~/planet-lab.

## Portable performance path

The interactive window uses `map_view_fast`; `map_view` remains the reference.
The optimized path computes camera trigonometry once and carries each transformed
endpoint forward along a polyline, transforming every stored point once per layer.
It keeps the reference's floating-point operation ordering. A `ViewCache` stores
only the background disc: rotation and layer switches reuse it; theme, zoom and
size changes rebuild it. Grid sampling remains on the reference path.

The X11 backend retains its Frame and ImageBuffer until the window size changes.
`pixels.c` builds 256-entry channel lookup tables from the detected masks and packs
8/16/24/32-bit pixels into byte-addressed rows, respecting byte order and stride.
Unsupported layouts fall back to XPutPixel. No native-endian integer stores,
alignment assumptions, fast-math, assembly, AltiVec or MIT-SHM are introduced.
Image submission is followed by XSync to bound outstanding work; this is not a
scanout measurement. The application still blocks while idle.

At 640×480 the extra background cache uses 921600 bytes. Framebuffer and XImage
reuse eliminate ordinary redraw allocations, except when size or cache dimensions
change. The background cache increases the maximum 2048×2048 interactive render
memory by 12 MiB. All buffers are released at exit.

## Separate toolbar and globe viewport

The main X11 window contains two non-overlapping child windows: a 28-pixel toolbar
and the globe viewport beneath it. Default outer size is 640×508, so the viewport
remains 640×480. Parent resize events update child geometry, with at least one
pixel of viewport storage for programmatic sizes shorter than the toolbar.
Window-manager size hints request a minimum of 576×128 for usable controls.

Each child has its own exposure handling. Dragging and zooming only dirty the
viewport. Toolbar label/exposure updates render into a small offscreen pixmap and
copy to the toolbar in one operation. Globe image submission targets only its
child; oversized viewport letterbox gutters are painted independently, without
clearing the image region. Toolbar clicks cannot begin a globe drag. Pointer
coordinates and drag sensitivity are relative to the globe viewport.

The Places layer and its separate popup/clipboard handling are documented in
places.md. The toolbar now has six buttons, with Places before Reset.
