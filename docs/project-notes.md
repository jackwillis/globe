# Project notes — 2026-11-02

Start with [handoff.md](handoff.md), [backlog.md](backlog.md) and the [ADR](adr/0001-m2-to-m5-decisions.md).
The historical v2/unmerged-branch state is preserved in [the archive](history/handoff-20261101.md).

## Current state

Main is the active branch; the owner renamed v2 and configured origin. All five earlier side
branches and GL-6 are merged. Agents do not push, tag, force or merge remote branches. The handoff
and packaging work are committed; there is no pending merge of those original side branches.

Implemented: strict pack formats/builders, bounded cancellable terrain, GL-6 band rejection, core
click/present-only rules, local elevation imports, X11 toolbar/cursors/country and city windows,
OS 9 proof and two edition reports. GL-27 adds the gray base, palettes, smoothing and Antarctic
white; GL-28 adds the lake-aware one-degree pack. Fetch is [design only](fetch-design.md).

Host GCC sanitizer and clang suites pass 35/35. G4 app tests include click rules and reproduce
9b4a458633a32d9d; terrain tests pass and combined lake-aware replay agrees with the host at
efa030af1a10f887. These are NetBSD headless checks, not OS 9 execution or current GUI acceptance.
Owner builds tip3–tip7 remain theirs and must not be deleted, overwritten or killed.

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

Surface/pass/theme refactors, dissolve ordering, desert color and runtime compression are later
scopes. The maps app is separate; there is no relay. In-app fetching is now wanted, but its design
needs review, hosting decisions and seed/OT acceptance before implementation. Keep that future
library independent of Globe's renderer and never damage immutable baked data on a failed import.

The product ADR now calls this an atlas. Map is the intended floppy default; Natural needs a
land-class source, and other looks/overlays have separate tickets. No fetch on the v0.1 floppy;
full edition first. The owner/Claude retains the smoothing cost/tuning decision. Their current
review note is preserved verbatim in the handoff; no further terrain tuning was done here.
