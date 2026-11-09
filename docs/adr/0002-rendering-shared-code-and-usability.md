# ADR 0002: Rendering changes, shared front-end code, and the usability direction

Status: accepted, 2026-11-07. The owner (Jack Willis) asked for or approved each user-visible item
below in conversation; items marked "implemented, owner informed" were engineering choices reported
to the owner. Context: `docs/refinement-order.md` (measurements), `docs/handoff.md` (state),
GitHub issues #16, #34 to #46.

## Decisions

| # | Question | Decision |
| --- | --- | --- |
| D1 | When do the lines appear while refining? | **First.** The job draws coastlines, borders and graticule into a private layer (`lines_first`), shows them, then paints the terrain under them, compositing the layer after every pass. Replaces the paced reveal (owner's request). Settled pixels and pins are unchanged. |
| D2 | What does the final terrain pass redraw? | **Only the cells clipped at the horizon**, in index order, after the 16 ring passes (`TerrainRepaint::LimbPass`). The old pass repainted every cell only so the higher index won where a flat limb polygon overlaps a shaded neighbour. The final image is now *defined* as every unclipped cell then every clipped cell in index order; it differs from the old image at a few limb pixels in about 2% of views (19 px in 14 of 600 random views), and removes 47% of the terrain fill work. Implemented, owner informed. |
| D3 | How do the composites touch the line layer? | A list of the layer's line pixels (a few percent of the frame), with a scan fallback if a view has too many. Lines are drawn before the terrain set-up. G4: whole refine 539 ms, first lines at 53 ms. |
| D4 | What happens on zoom? | **Zoom preview.** An orthographic view zoomed about its centre is a scaled picture, so the last finished terrain frame is scaled (nearest neighbour, factors 0.25 to 4, rim holes filled with ocean) with fresh lines on top, and the refinement runs over it without clearing. **The coarse elevation level is skipped** after a pure zoom (the core remembers the last completed view). Works at every refinement level and after a half-finished refine. Owner: "it's amazing". |
| D5 | Progress indication | A 10-pixel opaque bar along the bottom while refining, minimal (no blending, no frame copy), drawn on the shown frame only so pixels and pins are untouched; it is also the "done" signal (owner's request). |
| D6 | How much front-end code is shared? | The refinement lifecycle and presentation building are shared (`RefineRunner`, `ShownFrame`, `ZoomPreview`, `binding_for_char`, `same_orientation`). The event loop, wake timers, how pixels reach the screen and the presentation throttle stay per platform. **No coordinator or host-interface class**: what is left duplicated (about 15 lines of `apply()` and 8 of picking) is not worth the abstraction until a third platform appears (#16). |
| D7 | Next direction | **Usability** (owner, 2026-11-07): make Globe worth looking at for more than a minute. Umbrella #35. First the Classic Mac OS gaps the owner named: cursors (#44), a looks palette (#45), a menu bar (#46); then graphics tiers with a first-run speed test (#34), the help overlay (#40) and the atlas features (#36 to #43). Priorities are proposals. |
| D8 | Render tiers (#34) | Evaluate low (possibly black and white), medium and high strategies per refinement level, chosen by a short timed test at first run with a visible override. A tier is a performance setting, separate from the Map and Relief looks. Needs per-tier golden pins. Recording a choice on disk is a machine change and must be consented to. Moved to v0.2 at the owner's request. |

## Evidence

- Bare Mac OS 9 on the owner's G4 (2026-11-07), floppy edition: Relief 88 bands, 992 ms settle, copy
  150 ms, turn 19 ms, step 8 ms; Map 88 bands, 991 ms, copy 150 ms, turn 20 ms, step 9 ms. Classic
  under Tiger took 2,750 ms and 734 ms of copy. The 8 ms step is probably the 600 KB copy that
  fills the zoom cache in one step (unverified; splitting it into row chunks is the proposed fix).
- G4 compute on NetBSD: `docs/perf/g4-2026-11-06.txt`. Host: `docs/perf/repair-diff-host.txt`.
- 48 host tests pass (`x11_toolbar` is timing-sensitive under sanitizers and is excluded from that
  count); OS 9 floppy build and both-CPU probe pass.

## Consequences and open items

- The OS 9 zoom preview, progress bar, shared runner and key table were checked by the owner on bare
  OS 9 ("it works great") but have no automated OS 9 test; the logic is covered on the host.
- The OS 9 app is the floppy edition only; a full edition needs pack loading through the Toolbox and
  a larger partition than the 24 MiB preferred / 12 MiB minimum now requested.
- The Tiger and OS 9 copies on shared G4 and the NetBSD tips are scratch builds, not releases.
