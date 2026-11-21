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
Helvetica, then Palatino at 10 points; if neither family resolves, Country Names
is unchecked and disabled and no labels draw. A valid edition-bound anchor table
is also required. X11 tries the 12-pixel ISO-8859-1 Helvetica medium XLFD, then
the server's `fixed` font. If both fail, no labels draw and the tools toggle is
omitted. Neither frontend substitutes another font. Broader font roles and any
new fallback fonts remain in #61.
Both draw directly over a settled presentation using #36's placer. Pixel
verification in X11 disables the text overlay. Each front end allocates
candidate storage once at startup (about 6–7 KiB on 32-bit Classic, 8.3 KiB on
64-bit X11). The draw uses a 32-entry output array and the placer's 256-entry
index array, about 1.5 KiB total on the stack before call overhead. Native font
cache and Toolbox/X server allocations are platform-dependent.

## v0.1.2 acceptance and limitations — 2026-11-20

The owner ran PR #185 at `b7aa82a587dbc3a38c1ed8af3411b0d516db068e`
on the Power Mac G4 under bare Mac OS 9 on November 19. The timing-title
MacBinary was 592,512 bytes, SHA-256
`d5f9a7a48c41809406dceec1ab795190d6bf604e3faec6d6c26c0647fd652d3c`.
The owner accepted the appearance and anchor placement and reported no
perceptible slowdown. This is qualitative native evidence for that revision;
no numerical E1 comparison was collected. The owner waived that comparison
and the Palatino-only/neither-family font-removal checks for v0.1.2.

The November 20 release-scope decision preserves Helvetica first with
Palatino fallback. #218 is superseded; broader #61 typography stays in v0.2.
Actual Classic glyph ink and shadow bounds, including Palatino overhangs,
remain unverified and are an accepted, documented limitation for this patch.
Host metrics tests and cross-compilation do not establish those bounds.
Integration validation is recorded separately on PR #185 and #119; it does
not imply a new native run or invalidate the accepted historical result.
See the [owner acceptance report](https://github.com/jackwillis/poodle-earth/issues/119#issuecomment-7028370658)
and [release-scope resolution](https://github.com/jackwillis/poodle-earth/issues/119#issuecomment-7058237691).

## Original E1 owner-run Power Mac G4 procedure (historical)

The procedure below is retained for provenance, not as an additional v0.1.2
gate. Its numerical and font-removal steps were waived as recorded above.

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

6. Verify the menu with Helvetica available, with only Palatino available, and
   with neither installed: the last case must be disabled and unchecked. Also
   verify labels are unavailable on an edition/anchor mismatch.
7. Keep the Classic painted-bounds question open: compare actual glyph ink and
   the one-pixel shadow against the TextWidth/FontInfo placement rectangle,
   especially Palatino overhangs, adjacent labels and the globe limb. Record
   clipping or collisions rather than treating host synthetic metrics as proof.
8. E1 starts with one native label against the same settled frame without it;
   separately record the normal capped overlay. Review E2 anchor quality for
   island, multipart and crescent countries (including the cases above).

The historical procedure does not establish numerical timings or actual painted
bounds; use the acceptance and limitations record above for v0.1.2 status.
