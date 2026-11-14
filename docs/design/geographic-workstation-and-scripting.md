# Geographic workstation and scripting — exploratory direction

**Status:** Owner-directed aspirations and unvalidated hypotheses, 2026-11-10, condensed 2026-11-13. Most items here remain unscheduled research, **except the optional detached information-window first slice now targeted as a pre-v0.2 patch candidate in [#148](https://github.com/jackwillis/globe/issues/148)**. Nothing in this note independently authorizes implementation, hardware access or a release gate. It supplements the [product brief](product-direction-2026-11-10.md) and does not amend [ADR 0005](../adr/0005-entity-identity-and-navigation.md) or the [entity/navigation contract](entity-navigation-contract.md). The optional console and Listener are owned by [#63](https://github.com/jackwillis/globe/issues/63).

## Product direction

Globe may grow from an atlas toward an encyclopedia and light GIS workstation, with approachable exploration first and increasingly capable investigation after it; everyday use must never depend on programming. Aspirations: curated World, United States and Wisconsin editions on top of the embedded global baseline (useful offline without optional files), and, as a later experiment, compact indexed road networks beside elevation and hydrography on optical media (see [layered atlas and CD-ROM research](../research/cd-rom-atlas-architecture.md)). Contents, sources, licences, selection rules and packaging go through existing atlas-production review; no dataset, format, size estimate, routing capability or physical-media commitment is made here.

## Interaction hypotheses (need usability experiments)

- A native Tools palette with **Explore**, **Inspect** and **Measure**. Inspect would make overlapping entities (city, region, country, physical feature) discoverable. Mode feedback, keyboard access and tool boundaries are untested.
- Cursors: open hand by default, pointing finger over selectable objects, closed hand while dragging. Keep double-click-to-centre-and-zoom. Shift-double-click to zoom out needs a check against modifier conventions and discoverability.
- Hover highlights the top candidate without changing selection or camera; hover alone never navigates or runs a script.
- **Pre-v0.2 experiment (#148):** retain the native one-window globe + Looks/Information sidebar as the default, but provide an optional, reversible detached information window first. Make it a candidate for an independently usable v0.1.x patch, before v0.2.0; no exact patch has been selected. X11 catch-up follows the [parity policy](../backlog.md#incremental-releases-and-milestone-graduation-owner-decision-2026-11-13). Broader palettes, arbitrary docking and multiple independent globe viewports remain uncommitted UX experiments, especially on small displays.

## Programmable-geography hypotheses

**Owner refinement (2026-11-12, recorded on #63): GUI-first, progressively programmable.** The first slice would be *semantic command observation*, not interpreter execution: while a Listener is visible, accepted GUI actions appear as copyable Lisp forms such as `(show (place "Madison, Wisconsin"))`. Later, users might replay or edit commands, and in an explicit composition mode a click could insert a structured `(place "...")` argument into a partly typed form. Ordinary atlas clicks stay ordinary clicks. Command formatting and transcripts should work without initializing any VM.

Constraints that apply to any such feature:
- `Model / Msg / update / Effects` remains the only command execution path. Scripts and the GUI request the same validated semantic operations and receive owned, bounded results; nothing mutates the Model directly or retains pointers into optional packs.
- Stable identity, provenance, cancellation, resource lifetimes and deterministic recording still apply. Echo/duplicate suppression and minimally distinguishing human-readable references need their own prototypes.
- Ordinary atlas use incurs no interpreter startup or dependency. Insertion and execution are separate, visibly distinct actions.
- Saved tours, queries, measurements and results might later become geographic artifacts with input references, edition compatibility, parameters, source/date qualifiers and provenance. Persistence location belongs to the existing Preferences/resource work; no file writes are introduced here.

Illustrative investigations (definitions, not results): a population-ranked tour of national capitals (needs a reviewed roster, capital and population definitions, source/date, missing-value handling) and a population-weighted spherical centroid of EU national capitals (membership as of a date; weighted sum of unit vectors, normalized only when well defined; distinct from a planar average or intrinsic mean). No figures or answers are asserted.

## Interpreter candidates (none selected)

Scheme, Lua and Forth remain comparison candidates; earlier Forth exploration is not a commitment, and the Scheme/Lisp runtime is unselected. Compare actual implementations for licence, portable C++20 integration, Retro68 compatibility, startup/resident/peak memory, execution and collection pauses, cancellation, bounded data exchange and native text editing. Do not infer a winner from reputation.

## Evidence before adoption

Start with a disposable command-observation mockup and a few curated or synthetic operations. Compare novice exploration with optional expert workflows, and test mode exit, overlapping candidates, cancellation and loss of optional data. Any interpreter or optical-media benchmark must identify build, hardware and OS and keep host evidence separate from native Classic Mac results. The geographic identity slice remains independently useful without a Listener, workstation features or physical CD distribution.
