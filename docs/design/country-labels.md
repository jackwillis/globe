# Country names on settled globe frames

The pinned Natural Earth 110m country archive used for GPOL already contains
editorial label points and ranks. The builder keeps a point if it falls in an
exterior and outside its holes. Otherwise it samples the largest part and picks
the interior sample with the greatest approximate boundary clearance, scaling
longitude by the cosine of the part's midlatitude. The 24 × 24 grid is a
bounded fallback for this edition. Lower Natural Earth LABELRANK values place
first, with GPOL order as the tie-breaker.

This edition uses one anchor per country and a fixed screen-size font. Names
can cross country borders, and crowded views may suppress smaller countries.
A large country can remain visible after its sole label point passes behind
the limb. Russia, China, Australia, the United States, Brazil and Canada are
the most likely candidates for additional anchors. Canada, the Philippines,
the Bahamas and the United Kingdom also warrant a visual review of anchor
placement. The fallback uses a local planar approximation and maximizes
clearance only among sampled points. It can favor a different island from the
editorial point if the latter lies offshore in the 110m geometry.

## Packaging and presentation

The compiled table costs 2,124 bytes for 177 twelve-byte entries. A densely
packed little-endian sidecar with the same coordinates and one-byte ranks could
use 1,593 bytes plus header and checksum. The generated table saves the
reader and a separate artifact, in exchange for roughly half a KiB of data
padding. It holds no duplicate names. The expected CRC covers the complete GPOL
header fields and payload, including its name bytes and record order, and the
runtime also checks the count. The GPOL reader verifies its checksum and
structure before creating a view. An invalid GPOL or a valid GPOL with a
different checksum gets no labels. Regenerate both outputs together:

```sh
python3 tools/country-builder/build_countries.py "$PINNED_COUNTRY_ZIP" /tmp/countries.gpol --labels src/data/country_anchors.inc
cmp /tmp/countries.gpol data/countries.gpol
```

The Classic View menu and X11 tools window (F2) both offer **Country Names**.
Labels default on when the edition and font are available. Classic tries
Helvetica, then Palatino. X11 tries Helvetica, then the server's fixed font.
Both draw directly over a settled presentation using #36's placer. Pixel
verification in X11 disables the text overlay. Each front end allocates
candidate storage once at startup (about 6–7 KiB on 32-bit Classic, 8.3 KiB on
64-bit X11). The draw uses a 32-entry output array and the placer's 256-entry
index array, about 1.5 KiB total on the stack before call overhead. Native font
cache and Toolbox/X server allocations are platform-dependent.

## E1 owner-run Power Mac G4 check

1. Build a distinct PowerPC Classic app with `-DGLOBE_TIMING_TITLE=ON` and
   record its commit, binary size, OS version, screen depth, window size,
   assigned application partition and whether the run is cold or warm.
2. Pick a repeatable reset view, then repeat the same drag and zoom sequences
   with **View → Country Names** off and on. Use the same view, look, overlays
   and window dimensions for every pair. Alternate run order.
3. For each run, record elapsed time from input to the first useful frame and
   to the final presented frame. A video recording provides both timestamps.
   Record the title's settle, copy and maximum-turn timings and its `names`
   count for each matched refinement. Capture the final frame from each run.
4. Record peak allocation using a memory monitor or allocation instrumentation
   if available. Record the assigned partition separately and mark peak
   allocation unavailable if the tool only reports the partition.
5. Check whether labels disappear during dragging and zooming, return on
   settle, survive window uncover and resize, and immediately clear when
   disabled. Compare input lag and UI responsiveness across repeated pairs.

Native visual and timing acceptance is pending.
