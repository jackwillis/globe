# Project notes — 2026-11-02 (historical)

> Dated 2026-11-02. For the current state read [handoff.md](handoff.md) and [ADR 0002](adr/0002-rendering-shared-code-and-usability.md). Corrections since: refinement is circle-out only (rows, center-out and dissolve orders and the `order` trace verb were removed); lines now appear first through a line layer; the final terrain pass redraws only limb cells.

Start with [handoff.md](handoff.md), [backlog.md](backlog.md) and the [ADR](adr/0001-m2-to-m5-decisions.md).
The historical v2/unmerged-branch state is preserved in [the archive](history/handoff-20261101.md).

## Current state

Main is the active branch; the owner renamed v2 and configured origin. All five earlier side
branches and GL-6 are merged. Agents do not push, tag, force or merge remote branches. The handoff
and packaging work are committed; there is no pending merge of those original side branches.

Implemented: strict pack formats/builders, bounded cancellable terrain, core click/present-only
rules, local elevation imports, X11 toolbar/cursors/country and city windows, OS 9 proof and two
edition reports. Terrain: Map (default, biome-class land from the licensed RESOLVE class grid) and
Relief looks, Antarctic white, lake-aware one-degree pack, circle-outward refinement from the view
center (every other order was tried and removed), plain vectors while dragging, a paced progressive
line reveal after the terrain (GL-41), a click that waits out the double-click window before picking
(GL-43), an orange country outline (GL-44) and a looks palette window (GL-42, X11). Fetch is
[design only](fetch-design.md). Owner decisions are in the ADR addendum of 2026-11-02 (night).

Host suite 40/40; refined pins Map `e86d02e2424d2a47`, Relief `1c3eac3bcc2d3b8f`, replay hash
`9b4a458633a32d9d`, all equal on the G4 (headless). The Retro68 probe is clean and the OS 9 floppy
build packages and runs under Tiger Classic (2026-11-02, emulated; the owner confirmed drag, zoom and keys; not real OS 9 hardware), CI never run, real-display timing (GL-7) unmeasured.
Owner builds tip3-tip17 on maul remain theirs and must not be deleted, overwritten or killed.

## Measurements and limits

GL-6's matched old-profile before/after is about 786 to 380 ms at 1x. Adding GL-27 appearance gives
about 555/693 ms at the two profiler views. Gray base alone is about 4.3/5.3 ms; the standard replay's
final view measured about 802 ms with a longest 120-ms continuation. See [terrain.md](terrain.md)
for the actual views and raw data. View/quality differences explain why reports are not interchangeable.

Compression experiments are recorded in the ADR; there is no runtime container/decoder choice yet.
The fetch sizing component adds 104,206 OS 9 data-fork bytes, excluding a complete fetch feature.
The two-edition build reports are [here](floppy-size.md). No CI, current real-display timing, native
OS 9 execution or live TLS evidence is claimed for Globe.

## Carry-forward constraints

Never compile on maul or change desktop access controls. Use new temporary binary names and
restore their executable bit after transfer. ETOPO and Natural Earth source files live in
`~/scratch/globe-data/`, not Git; elevation rows are south-first. Keep no clock/OS headers,
exceptions/RTTI or frame allocations in the portable core. Retro68 printf lacks %ll/%z. Use
meaningful tests, deliberately reject mutants, run both portability probes after core changes,
and distinguish bounded work counts from wall-clock guarantees.

Surface/pass/theme refactors, desert color and runtime compression are later scopes. Dissolve
ordering was an opt-in experiment (GL-19) and has since been removed; refinement is circle-out only. The maps app is separate; there
is no relay. In-app fetching is now wanted, but its design needs review, hosting decisions and seed/OT acceptance before implementation. Keep that future
library independent of Globe's renderer and never damage immutable baked data on a failed import.

The product ADR now calls this an atlas. Map is the intended floppy default; Natural needs a
land-class source, and other looks/overlays have separate tickets. No fetch on the v0.1 floppy;
full edition first. The owner/Claude retains the smoothing cost/tuning decision. Their current
review note is preserved verbatim in the handoff; no further terrain tuning was done here.

The 2026-11-02 continuation implements GL-19 and the focus part of GL-26: trace-selectable row,
center-out and Gaussian orders with unchanged settled pins, G4 progress hashes and measured costs.
See [refinement-order.md](refinement-order.md); dissolve costs about 14–15% more overall and is not
a proven responsiveness improvement over the matched baseline. Smoothing remains unchanged.
GL-40 is [an ADR proposal](adr/0001-m2-to-m5-decisions.md) only: v1 has no lossless ice bit, and the
proposed v2 plane needs review before builder/decoder/data work. The dated handoff records failures,
verification and stopping point.
