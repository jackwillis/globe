# Geographic workstation and scripting — exploratory direction

**Status:** Owner-directed product aspirations and unvalidated design hypotheses, November 10, 2026. This supplements the [product brief](product-direction-2026-11-10.md); it does not amend [ADR 0005](../adr/0005-entity-identity-and-navigation.md) or the [entity/navigation interface proposal](entity-navigation-contract.md). Nothing below is an implemented capability or a new release gate.

## Established product direction

Globe should grow into a hybrid geographic atlas, encyclopedia and productivity/GIS workstation. Approachable geographic exploration comes first; increasingly capable investigation and spatial computation can follow without making everyday atlas use depend on programming.

The desired editorial family includes curated World, United States and Wisconsin atlas editions. Preserve an embedded global baseline, useful offline without optional files, while allowing higher-detail regional geography and knowledge. Exact contents, sources, licensing, selection rules and packaging need review through the existing atlas-production work; these edition aspirations do not approve a dataset or format.

CD-ROM distribution remains a future experiment. Investigate compact indexed road networks alongside elevation, hydrography, other geography and knowledge content, building on the [layered atlas and optical-media research](../research/cd-rom-atlas-architecture.md). Road coverage, connectivity, indexing, seek behavior, decoding, memory and responsiveness need measured evidence. No data-size estimate, road-routing capability or physical-media release commitment is established here.

## Interaction hypotheses

A potential native Macintosh Tools palette could offer **Explore**, **Inspect** and **Measure**. Explore would support ordinary navigation and selection; Inspect would make overlapping geographic entities discoverable; Measure would expose geographic measurement operations. Tool boundaries, keyboard access and mode feedback need usability experiments.

The desired cursor vocabulary is an open hand by default, a pointing finger over selectable objects and a closed hand while dragging. Preserve double-click-to-center-and-zoom. Investigate Shift-double-click to zoom out, checking conflicts with existing modifier proposals, native conventions and discoverability before adopting it.

A hover experiment would highlight the top geographic candidate without changing semantic selection or camera state. Explicit Inspect would reveal overlapping candidates so the user can choose among a city, administrative region, country or physical feature. Candidate priority, stable hover behavior and the relationship to label/point/polygon hit testing need validation; hover alone should not navigate or execute a script.

Keep the existing main-window Looks/Information sidebar as the baseline. Separate Tools or Listener windows, detached information views and broader multiwindow arrangements remain UX experiments, especially on small displays. This note does not authorize replacing the sidebar or require additional windows for normal exploration.

## Programmable geography hypotheses

A possible lazy-loaded Lisp Listener/command palette could expose geographic operations on demand. Ordinary atlas use should incur no mandatory interpreter startup or dependency. Clicking an entity could insert a structured Lisp expression containing its stable geographic reference into the Listener for inspection or composition; insertion and execution should be distinct actions with visible feedback.

Prepared scripts might temporarily define click behavior, for example collecting places or inspecting selected facts. Such a mode would need clear activation, an obvious exit/reset and predictable interaction with Explore/Inspect/Measure. It must not silently replace normal click behavior outside that mode.

The GUI, geographic services and scripts should share validated operations rather than separate geographic logic. A future scripting adapter would request the same semantic operations and receive owned, bounded results through the existing message/effect boundary. Stable identity, provenance, cancellation, resource lifetimes and deterministic recording still apply. No script directly mutates Model or retains pointers into optional packs; exact APIs await experiments and shared review.

Two illustrative investigations could test usefulness:

- **Population-ranked tour of national capitals:** define the country/capital roster, treatment of alternate or disputed capitals, population unit (city proper versus urban area), source/date and missing values; produce a reproducible order and a cancellable navigation tour.
- **Population-weighted spherical centroid of EU national capitals:** define membership as of a date and reviewed capital/population assertions, form a weighted sum of unit vectors and normalize it only when well-defined. Report missing inputs and degenerate cases; distinguish this result from a planar average or an intrinsic spherical mean. No population figures or computed answer are asserted here.

Tours, queries, measurements, collections and calculation results may eventually become first-class saved geographic artifacts. A research design should preserve input entity references, edition compatibility, parameters, source/date qualifiers and result provenance. Persistence location, compatibility and unavailable-resource behavior remain later decisions under the existing resource/Preferences work; no file writes are introduced by this note.

## Interpreter comparison to investigate

Scheme, Lua and Forth are comparison candidates; **no interpreter is selected**. Treat these as questions for small prototypes, not verified platform claims:

| Candidate | Question to test |
| --- | --- |
| Scheme | Does its Lisp expression model make geographic composition and click-inserted forms approachable, within Classic toolchain, memory and collection-pause constraints? |
| Lua | Would a command palette plus Lua offer a clearer embedding/API boundary, and how would structured entity insertion work without promising Lisp syntax? |
| Forth | Could a compact interactive vocabulary serve geographic investigation as well as developer diagnostics, with understandable errors and discoverable commands? |

Compare actual implementations for license, portable C++20 integration, Retro68 compatibility, startup/resident/peak memory, execution and garbage-collection pauses, cancellation, bounded data exchange and native text editing. Do not infer a winner from language reputation. The Listener concept relates to existing console research (#63), but broader user-facing geographic programming is not automatically part of that issue's scope.

## Evidence before adoption

Begin with a disposable command/interaction mockup and a few curated or synthetic geographic operations. Compare novice exploration with optional expert workflows; test mode exit, overlapping candidates, cancellation and loss of optional data. Interpreter and CD-ROM benchmarks, if later authorized, must identify build, hardware and OS and distinguish host evidence from native Classic Mac results.

This note adds no issue, milestone, runtime dependency, production data migration or implementation assignment. The geographic identity slice remains independently useful without a Listener, GIS workstation features or physical CD distribution.
