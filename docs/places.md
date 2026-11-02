# Offline Wikidata places and popovers

Thirteen city markers are available by default. Toggle them with the Places
button or P. Click a diamond marker to open its facts; drag to rotate the globe.
Movements of up to four pixels are treated as click jitter. A popover closes with
its x button, Escape, a click on the globe, or a camera/resize/layer change.
Escape quits when no popover is open. Very small views show an enlarge-window
hint instead of overlapping the details and link buttons.

Popovers display an English label and description, a dated population statement,
coordinates, snapshot date and source revision. Copy Wikipedia copies a related
article URL; Copy Wikidata copies the exact source revision URL. The clipboard
supports X11 UTF8_STRING and STRING targets. Keep the app running while pasting
unless a desktop clipboard manager preserves the selection.

No runtime HTTP client, JSON parser, browser executable or image library is
required. The facts work offline. Following the copied external links requires
an appropriate browser/network connection. Source provenance and the selected
statements are retained in data/wikidata/places.json and data/SOURCES.md.

## Runtime format: PLNTPOI version 1

All integers are explicitly big-endian; no native structs are serialized.

Header: 8 bytes `PLNTPOI` plus NUL; u32 version (1); u32 record count; 10 ASCII bytes
of ISO snapshot date YYYY-MM-DD.

Each record: u32 numeric QID; u32 source revision; u32 longitude in offset
microdegrees (degrees + 180) × 1000000; u32 latitude (degrees + 90) × 1000000.
Then four text fields, each with a u16 byte length followed by ASCII bytes:
label (1..63 bytes), description (0..191), population text (0..95), and English
Wikipedia URL (0..255).

The runtime checks count (1..1024), dates including leap days, QID/revision values,
duplicate entities, coordinate ranges, printable ASCII, string lengths,
Wikipedia URL prefix, truncation and trailing bytes. QIDs must be below one
billion; revisions must fit u32. Future expansion beyond these limits requires a
format version change. Maximum place heap storage is approximately 656 KiB.
Call places_free before reusing a loaded Places object.

The same sphere orientation and orthographic projection used by the globe
position the markers. Hidden-hemisphere and offscreen markers are neither drawn
nor pickable. Picking chooses the nearest visible marker strictly within nine
pixels, with stable input-order tie breaking. Oversized letterboxed viewports
subtract the image offset before picking.

Markers are composed into the CPU framebuffer before conversion/submission.
The details panel is a child of the globe viewport with a small offscreen backing
pixmap. Globe redraws are clipped around it. No animation or idle polling is
introduced. Clipboard requests are served through the normal X11 event loop.

## Validation and cost

- Linux: geometry, parser, malformed/truncated input and offline-rebuild tests
  passed, including ASan/UBSan with leak detection disabled as previously noted.
- Native G4: place loading, projection, picking and marker tests passed; the
  source asset checksum matches Linux. X11 executable compiled natively.
- 16-bit/24-bit Xvfb: correct city selection, popover bounds, both source links
  through a real X11 clipboard request, close button, Escape, jitter handling,
  Places toggle, mouse rotation/zoom, separate toolbar stability and tiny resize
  tests passed. The actual Cairo popover capture was visually inspected.
- Native G4 benchmark at 640×480 with 13 cities, 120 camera positions: marker
  overlay mean 0.081113 ms, p99 0.116818 ms; point picking mean 0.026339 ms,
  p99 0.030609 ms. Entire run 1.823692 seconds. This measures only marker and
  picking stages, excluding globe rendering, text drawing and X11 presentation.
  Raw results: benchmarks/g4-places.json. No full-frame FPS claim is made.

Reproduce:

```
make check-places
make planet-places-bench
./planet-places-bench > places-benchmark.json
```

On Linux with Xvfb installed:

```
make check-x11
```

Native G4 desktop interaction remains unverified from SSH because of the existing
display authorization limitation. No access controls or packages were changed.

## Current limits

This is a curated small city layer, not a complete gazetteer. There is no live
refresh, search, automatic label placement, image download or terrain metadata.
English text is approximated to ASCII for the first core-X11 font path. Adding
more entities should include deduplication, marker decluttering and zoom-aware
selection, alongside measured native costs.
