# Architecture — as built, revised 2026-11-07

The [ADR](adr/0001-m2-to-m5-decisions.md) records decisions; [backlog](backlog.md) separates current
code from later surface/pass/theme and fetch work.

| Layer | Responsibilities |
| --- | --- |
| `include/globe`, `src/core` | Geometry/projection, camera interaction, pure Model/Msg/update/Effects, quality/style selection, cursor/button intent, spherical country and visible-city picking |
| `src/data` | Strict bounded zero-copy GATL, GELV, GPOL and GPLC readers, CRC and endian handling |
| `src/render` | RGB565 framebuffer, exact lines and spans, shaded triangles, cached scene vectors/caps, bounded cancellable terrain jobs (line layer, ring passes, limb pass), city markers, and the pieces both front ends share: `RefineRunner`, `ZoomPreview`, `ShownFrame` and the overlays (night, measure, country, progress bar) |
| `src/platform/common` | Pixel-layout conversion, file/import bounds, CLI/timing helpers and the clock seam |
| `src/platform/x11` | Main window, utility toolbar and looks palette, native cursor mapping, scrolling information window, select-based one-shot wakes, XImage conversion/submission/readback |
| `src/platform/os9` | Classic non-Carbon PowerPC application, GWorld/CopyBits with adaptive row slices, Toolbox information card, Microseconds and GetDateTime clocks, wake-driven WaitNextEvent |
| `src/platform/bench` | Trace replay, refinement profiler, line-layer and zoom-preview timing and isolated moving-base timing |
| `tools` | Modern pack builders, embedded resources, policy/portability checks, edition builds and size experiments |

Portable code has no OS headers or clock reads. Time enters messages; update returns redraw,
present-only, quit and replacement-wake effects. Repaint resends existing pixels and keeps active
refinement. A click below drag slop changes no camera, stage, generation or render request; an
active drag, camera/size/data/style change invalidates old screen work. The replay remains deterministic.

Sources, cached unit vectors, shading indices and workspaces allocate at load; frame paths
allocate nothing (a refinement job allocates its line layer and snapshot buffer when it starts).
Only the current generation/token may use a terrain projection workspace. Moving views draw
outlines, or, while only the zoom changes, a scaled copy of the last finished terrain frame with
fresh lines on top (`ZoomPreview`). A settled refinement draws its lines into a private layer first
and shows them, then paints the terrain in 16 ring passes outward from the view center and one
final pass over the cells clipped at the horizon, compositing the layer after each pass; the final
image is defined by the order in `docs/refinement-order.md` and does not depend on the focus or on
what was on screen before. Antarctica and water classification precede palette lookup. Runtime
formats remain unchanged by these appearance choices.

X11 sleeps in select when idle; OS 9 uses WaitNextEvent with a long interruptible wait and a mouse
region. A pending stage deadline or bounded refinement work wakes the loop. RGB565 converts to
XImage masks/order or big-endian QuickDraw RGB555, then submits complete regions. X11 readback
checks both 16- and 24-bit private displays; real X11 display timing remains unverified, and OS 9 has run only on the owner's machines (bare OS 9 on 2026-11-07).

The full and floppy builds select data/front end at build time. Full includes country/city data
and local elevation imports; the OS 9 proof is intentionally smaller. Baked arrays remain immutable.
A proposed independent fetch library is [design only](fetch-design.md), with no dependency on the
rendering core. [Terrain measurements](terrain.md), [format specifications](elevation-format.md)
and [toolchain evidence](toolchain.md) document practical limits.

## Planned entity/navigation boundary

The [2026-11-10 product brief](design/product-direction-2026-11-10.md) extends the atlas direction. [Draft ADR 0005](adr/0005-entity-identity-and-navigation.md) and the [shared interface proposal](design/entity-navigation-contract.md) describe future stable identities, bounded resource-independent navigation references and native-font label interaction. These are design contracts awaiting prototypes and review, not changes to the as-built layers above. Existing research allocations remain proposed concepts; production GPOL/GPLC readers and Model/Msg behavior are unchanged.
