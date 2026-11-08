# Portable text and label placement (#36): design only

Typography is decided in [typography.md](typography.md): **Helvetica and Palatino as installed
system fonts, nothing bundled**, with sensible fallbacks. This note settles the boundary between
placement logic and text drawing so labels (#38) can proceed without a font engine. Nothing here is
implemented.

## Where the earlier research stands

The experiments on `claude/typography-design` (now merged, `tools/typography-experiments/`) remain
as evidence only: libschrift about 8 KB code and 45 KB library heap, FreeType about 209-287 KB code
and 29-40 KB heap, a 1-bit glyph pack about 6 KB per face and size. They were measured on x86-64
with stand-in fonts, are not PowerPC numbers, and are not like for like. The owner's decision makes
them moot for production: no libschrift, no FreeType, no bundled fonts or glyph packs. Do not
restart them unless a concrete system-font limitation appears. An installed proprietary font is not
permission to copy or redistribute its outlines or its rasterized glyphs; drawing through the OS is.

## The boundary

Core (portable, no QuickDraw, no X11) owns the first five stages; the platform owns the last.

1. **Candidates and priority**: `LabelCandidate { text, kind, priority, anchor (LatLon) }`, sorted
   by (priority, id), so order is deterministic.
2. **Anchor**: project to a screen point; drop anything on the far side or outside the limb margin.
3. **Measure**: through `TextMetrics`, supplied by the platform:
   `width(face, text) -> int`, `ascent(face)`, `descent(face)`, `leading(face)`, and `has_glyphs(face,
   text) -> bool`. Faces are roles (`Place`, `Country`, `Water`, `Interface`), not font names; the
   platform maps roles to Helvetica or Palatino and falls back (Geneva, Times) when absent.
4. **Collision**: axis-aligned boxes with a padding, greedily accepted in priority order against the
   accepted set (small N, a flat array; no allocation per frame beyond a bounded vector).
5. **Placement and clip**: try candidate offsets in a fixed order (right, left, above, below), keep
   the first that avoids collisions and stays inside the globe limb circle and the view; truncate
   long names to a maximum width with a trailing mark, or drop; skip text where `has_glyphs` is false.
6. **Draw** (platform): given `PlacedLabel { text, face, x, y }`, draw it.

A `FixedMetrics` provider for tests uses fixed widths (for example 6 px per character), ascent 8,
descent 2, spacing 2. Golden tests, all deterministic for a given input and provider: candidate
order and stable placement across two runs; two labels that collide (the lower priority is moved or
dropped); a label that would cross the limb; a very long place name; text with a missing glyph;
the same inputs with a second provider giving different widths (different positions, same algorithm,
no pixel comparison between platforms).

## Native drawing or platform rasterization

| | A. Native drawing (QuickDraw, X11 text) | B. Platform glyph bitmaps composed by the core |
|---|---|---|
| Transparency | needs the label drawn over the finished window, or with `srcOr` into the GWorld | core controls alpha and the overlay stays out of the frame |
| RGB565 composition | none: the OS draws into its own port | core blends into `Framebuffer`; one more conversion path |
| Clipping | OS clip region | core clips |
| Platform integration | real system fonts, hinting, antialiasing for free | needs a bitmap source per platform (QuickDraw into a scratch GWorld, X11 pixmap), the part most likely to differ |
| Complexity | low | high: caches, glyph surfaces, memory ledger entries (#55) |
| Memory | none held by Globe | a glyph cache the ledger must account for |
| Performance | one OS call per label; labels drawn once per settled frame | cheaper to redraw with the frame, dearer to build |

**Recommendation: A.** Draw labels natively after the settled frame, as `ShownFrame` already treats
overlays (a copy path or a window-time draw, never part of refinement). The placement code stays in
the core, pixel-exact parity between OS 9 and X11 is not a requirement, and B can be revisited only
if A cannot do what the design needs (for example, labels baked into exports, #43). One G4
experiment is the useful next step: draw one label with QuickDraw in the OS 9 build and time it.
That needs hardware, so it was not done here.
