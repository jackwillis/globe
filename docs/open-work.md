# Execution status — 2026-11-02 (historical)

> Superseded: this is the state of 2026-11-02. Current plans are in [backlog.md](backlog.md) and GitHub Issues; decisions since are in [ADR 0002](adr/0002-rendering-shared-code-and-usability.md). Several items below (the gray moving base as a default, dissolve ordering, trace-selectable orders) no longer exist.

The live prioritized list is [backlog.md](backlog.md); the current operating instructions and dated
agent notes are in [handoff.md](handoff.md). Work is on `main`; the owner renamed `v2`, retained tag
`v1`, and configured origin. The owner handles pushes. The handoff is now committed.

## Current pass

- GL-27 implemented: gray moving base, CycleStyle in model/traces/toolbar, hypsometric/cel palettes,
  Antarctic white and shared-corner shaded terrain. G4 before/after and isolated base costs are in
  [terrain.md](terrain.md). The longest measured continuation is a responsiveness limitation, not
  evidence of a hard frame-time bound.
- GL-28 builder scope implemented: pinned Natural Earth lakes, cell-center even-odd water mask,
  unchanged GELV v1 layout, twice-reproduced one-degree pack. [Rule and limits](elevation-format.md).
  Runtime lake outlines and finer data remain later work.
- GL-4 documentation refreshed: README, handoff, architecture, toolchain and project notes describe
  current code; superseded instructions and original bootstrap measurements are explicitly archived.
- GL-31 design written: [fetch-design.md](fetch-design.md), including weather reuse, licensing,
  seed/transport boundaries, candidate validation and measured TLS link cost. No networking code
  or third-party dependency was added to the application.

## Previously completed

The original ordered handoff items 1–9 and independent tasks 1–5 are implemented: commands and
scheduler, bounded cancellable refinement, GELV builder/decoder, terrain, local import, X11 toolbar
and cursor feedback, country/city packs and information windows, OS 9 proof of life, two-edition
packaging and four-size host/G4 measurements. Click/expose rules and GL-6 band skipping were merged
before this pass. Historical pass notes are [archived](history/handoff-20261101.md).

## Still requires acceptance or another scope

OS 9 execution, real-display G4 timing, origin publication and CI execution are owner gates.
The surface/pass/theme refactors, center-biased dissolve, runtime compression, larger layers,
network implementation and maps app are not part of this pass. The fetch design needs review and
hosting/provisioning decisions before implementation. No source ZIPs or private seeds are committed.
