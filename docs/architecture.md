# Architecture — as built 2026-11-02

The [ADR](adr/0001-m2-to-m5-decisions.md) records decisions; [backlog](backlog.md) separates current
code from later surface/pass/theme and fetch work.

| Layer | Responsibilities |
| --- | --- |
| `include/globe`, `src/core` | Geometry/projection, camera interaction, pure Model/Msg/update/Effects, quality/style selection, cursor/button intent, spherical country and visible-city picking |
| `src/data` | Strict bounded zero-copy GATL, GELV, GPOL and GPLC readers, CRC and endian handling |
| `src/render` | RGB565 framebuffer, exact lines and spans, shaded triangles, cached scene vectors/caps, coarse gray land base, bounded cancellable terrain/line bands and city decoration |
| `src/platform/common` | Pixel-layout conversion, file/import bounds, CLI/timing helpers and the clock seam |
| `src/platform/x11` | Main window, utility toolbar, native cursor mapping, scrolling information window, select-based one-shot wakes, XImage conversion/submission/readback |
| `src/platform/os9` | Classic non-Carbon PowerPC proof, GWorld/CopyBits, Microseconds clock and wake-driven WaitNextEvent |
| `src/platform/bench` | Trace replay, refinement profiler and isolated moving-base timing |
| `tools` | Modern pack builders, embedded resources, policy/portability checks, edition builds and size experiments |

Portable code has no OS headers or clock reads. Time enters messages; update returns redraw,
present-only, quit and replacement-wake effects. Repaint resends existing pixels and keeps active
refinement. A click below drag slop changes no camera, stage, generation or render request; an
active drag, camera/size/data/style change invalidates old screen work. The replay remains deterministic.

Sources, cached unit vectors, shading indices and workspaces allocate at load; frame paths allocate
nothing. Only the current generation/token may use a terrain projection workspace. Moving views
fill a decimated flat mask and draw outlines. Settled bands prepare/project corners and bounds,
clear their own pixels, shade terrain, then draw lines; GL-6 rejects lines/arcs outside the band.
The final frame is independent of the starting moving base. Antarctica and water classification
precede palette lookup. Runtime formats remain unchanged by these appearance choices.

X11 sleeps in select when idle; OS 9 uses WaitNextEvent with a long interruptible wait and a mouse
region. A pending stage deadline or bounded refinement work wakes the loop. RGB565 converts to
XImage masks/order or big-endian QuickDraw RGB555, then submits complete regions. X11 readback
checks both 16- and 24-bit private displays; real display timing and OS 9 execution remain unverified.

The full and floppy builds select data/front end at build time. Full includes country/city data
and local elevation imports; the OS 9 proof is intentionally smaller. Baked arrays remain immutable.
A proposed independent fetch library is [design only](fetch-design.md), with no dependency on the
rendering core. [Terrain measurements](terrain.md), [format specifications](elevation-format.md)
and [toolchain evidence](toolchain.md) document practical limits.
