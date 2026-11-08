# Globe — roadmap and issue tracker

> **Active planning now lives in [GitHub Issues](https://github.com/jackwillis/globe/issues).** This document is a lightweight map of the work, not a second backlog. The previous GL-numbered tracker is preserved verbatim in [docs/history/backlog-through-2026-11-05.md](history/backlog-through-2026-11-05.md). Last migration review: **2026-11-05**, source state through `f8b8159`.

## Planning policy

- **Keep Elm-style `Model / Msg / update / Effects`.** Application decisions remain deterministic; small Model state changes in `update()`. Published geographic input and per-job view plans are logically immutable, while framebuffers and rendering workspaces remain mutable.
- **Offline-first, no maintainer-operated services.** A bundled atlas remains fully usable without downloads. Optional fetching awaits vetted hosts, provenance, resource safety and consent.
- **v0.1 does not wait for speculative architecture work.** Hardware acceptance, complete CI verification, requirements, attribution and release packaging (StuffIt archive, MacBinary, disk image) are release priorities.
- **Progressive computation is a core technique; presentation is a UX hypothesis.** Compare current expressive reveal, minimal-delay and adaptive variants with representative users and on the real G4. Do not impose theatrical delays or weaken accessibility.
- **Evidence before optimization:** match view/style/hardware; measure input responsiveness, worst continuation, first useful frame, total completion, copy/upload cost and pixel correctness.
- **Safety:** honor [agent handoff](handoff.md) for protected machines, worktrees and release authorization. The tracker authorizes planning, not unattended device access or publication.

## Release themes

These are the three GitHub milestones (v0.1, v0.2, v0.3), created without due dates. The epics (#1–#6) span releases and carry no milestone.

| Planned milestone | Outcome | Practical gate |
| --- | --- | --- |
| **v0.1 — The Classic Atlas** | A functioning, documented offline Classic Mac application | Native OS 9 acceptance, CI, verified minimum memory, essential Mac polish, fork-preserving release |
| **v0.2 — The Expressive Atlas** | Polished exploration and maintainable rendering | Persona-informed progressive UX, visual stability, accessible settings if justified, smaller coordinator/phase refactors, useful layers/search |
| **v0.3 — The Expandable Atlas** | Safe optional richer geographic data | Resource lifecycle, bounded imports/cache, supported third-party acquisition, conditional regional LOD |

No due dates or fixed feature commitments have been set.

### Editions and distribution (owner decision, 2026-11-07)

- **The 1.44 MB floppy edition is paused.** It stays buildable and its build target stays in CI, but it is no longer a release target and size squeezing stops. Its first bare OS 9 run (2026-11-07) remains the performance baseline.
- **Distribution is digital first:** a StuffIt archive as the headline download, with the MacBinary file and a raw HFS disk image as fallbacks ([#47](https://github.com/jackwillis/globe/issues/47), [#12](https://github.com/jackwillis/globe/issues/12)). Physical media (CD-R, floppy) come later and are optional.
- **A larger edition ships as an application plus a data folder**, laid out like a CD and published as a disk image first: external data packs on Classic Mac OS ([#48](https://github.com/jackwillis/globe/issues/48)), running from a read-only volume ([#49](https://github.com/jackwillis/globe/issues/49)), the full-edition data ([#50](https://github.com/jackwillis/globe/issues/50)) and the volume layout ([#51](https://github.com/jackwillis/globe/issues/51)).

## Epics (parent tracking issues)

| Epic | Outcome | Active issue |
| --- | --- | --- |
| Classic Macintosh experience | Hardware-native, responsive and accessible Classic UI | [#1](https://github.com/jackwillis/globe/issues/1) |
| Responsive progressive rendering | Interruptibility, perceptual performance and purposeful presentation | [#2](https://github.com/jackwillis/globe/issues/2) |
| Portable application architecture | Evolve Elm-style state, immutable data, mutable workspaces | [#3](https://github.com/jackwillis/globe/issues/3) |
| Geographic discovery and visual design | Useful offline geography, search, layers and visual clarity | [#4](https://github.com/jackwillis/globe/issues/4) |
| Offline data and acquisition | Validated packs, safe cache and optional provider ingestion | [#5](https://github.com/jackwillis/globe/issues/5) |
| Atlas production and geographic integrity | Reproducible data pipeline, editorial rules, identity, semantic QA and edition recipes | [#52](https://github.com/jackwillis/globe/issues/52) |
| Release engineering and verification | Reproducible CI and builds, target evidence | [#6](https://github.com/jackwillis/globe/issues/6) |

Each epic includes a linked checklist of its tracked work. These are ordinary GitHub Issues with cross-references; **native GitHub parent/sub-issue relationships have not been configured**. The repository milestones v0.1–v0.3 exist.

## Direction since 2026-11-07: usability

The owner's call after the rendering and responsiveness work: **make Globe an atlas worth looking at
for more than a minute.** Umbrella issue [#35](https://github.com/jackwillis/globe/issues/35)
(milestone v0.2, under epic #4, touching #1). Order: the Classic Mac OS gaps first (cursors #44, looks
palette #45, menu bar #46), then render tiers with a first-run speed test (#34, moved to v0.2), the
help overlay (#40), and the atlas features: text foundation (#36), pointer readout (#37), labels
(#38), scale bar and degree labels (#39), animated zoom (#41), remembered view (#42), image export
(#43); existing #24 (find places), #23 and #25 stay linked. Decisions and evidence:
[ADR 0002](adr/0002-rendering-shared-code-and-usability.md).

## Atlas production as a parallel project (2026-11-07)

The owner approved treating Globe as **two cooperating projects in one repository**: the portable interactive runtime and a reproducible, transparent atlas-production pipeline whose source selection, editorial rules, geographic QA and edition recipes can evolve separately. The interface is versioned, strictly bounded atlas packs; no new runtime GIS dependency or infrastructure is implied. See [ADR 0003](adr/0003-atlas-production-boundary.md).

- [#52 — Atlas production, geographic integrity and edition rules](https://github.com/jackwillis/globe/issues/52) coordinates data-side decisions, alongside existing data epic #5 and discovery epic #4.
- [#53 — Audit atlas coverage and prototype source-driven editions](https://github.com/jackwillis/globe/issues/53) is the first **research-only** assignment: source/provenance and omissions audit, microstates/capitals, rivers/lakes, three candidate edition sizes and a semantic QA plan. No production-format or runtime migration is authorized yet.
- **Working arrangement:** Data-side work proceeds as a separate reviewable research stream, independent of the Classic Mac UI backlog. Any production pack, loader, or renderer integration follows a subsequent reviewed decision; keep source downloads and raw large datasets outside the Git repository unless their use and licensing are explicitly approved.
- Existing [#24](https://github.com/jackwillis/globe/issues/24), [#26](https://github.com/jackwillis/globe/issues/26), [#29–#31](https://github.com/jackwillis/globe/issues/29), [#36–#39](https://github.com/jackwillis/globe/issues/36), [#48](https://github.com/jackwillis/globe/issues/48) and [#50](https://github.com/jackwillis/globe/issues/50) remain the related work; avoid duplicate implementation tickets.

Source decisions, pack types, exact country definitions and release placement remain open pending actual data and memory measurements. Data work can progress independently of Classic Mac UI tasks.

## First release: recommended order

1. [#7 — Verify complete green CI](https://github.com/jackwillis/globe/issues/7): closed 2026-11-06 after a full successful workflow run. Check the Actions tab for the current result before relying on it.
2. [#8 — Native OS 9 hardware acceptance](https://github.com/jackwillis/globe/issues/8): the first run on real OS 9 was recorded on 2026-11-07 ([os9-proof.md](os9-proof.md)); cursors, resizing, exposure and idle behaviour are still unchecked.
3. [#9 — Real G4 X11 and 0.5° benchmarks](https://github.com/jackwillis/globe/issues/9) and [#10 — OS 9 memory requirements](https://github.com/jackwillis/globe/issues/10).
4. [#13 — Precise invalidation](https://github.com/jackwillis/globe/issues/13) and [#14 — Presentation lifecycle regression tests](https://github.com/jackwillis/globe/issues/14) as narrowly scoped v0.1 improvements if warranted.
5. [#11 — Classic Mac UI polish](https://github.com/jackwillis/globe/issues/11), [#47 — Release packaging](https://github.com/jackwillis/globe/issues/47), then [#12 — v0.1 release](https://github.com/jackwillis/globe/issues/12) after verification gates.

## UX and architecture after the initial release

- [#19 — Research refinement presentation modes](https://github.com/jackwillis/globe/issues/19) and [#20 — Reduced-motion and visual-stability policy](https://github.com/jackwillis/globe/issues/20) follow the persona debate. Personas are hypotheses, not research participants; no preference count or default is predetermined.
- [#21 — Adaptive presentation experiment](https://github.com/jackwillis/globe/issues/21) and [#22 — Dirty-region repair benchmark](https://github.com/jackwillis/globe/issues/22) must prove their value on the G4.
- [#15 — Typed refinement phases](https://github.com/jackwillis/globe/issues/15), [#16 — Shared runtime coordinator](https://github.com/jackwillis/globe/issues/16), [#17 — Immutable input / mutable workspace boundaries](https://github.com/jackwillis/globe/issues/17) keep Elm-style design intact.
- [#26 — Safe resource lifecycle](https://github.com/jackwillis/globe/issues/26) precedes [#28 — Optional HTTPS acquisition](https://github.com/jackwillis/globe/issues/28); [#29](https://github.com/jackwillis/globe/issues/29) through [#31](https://github.com/jackwillis/globe/issues/31) concern provider-independent conversion, LOD research, and compressed packs.

## Uncommitted future concepts (do not silently turn into implementation work)

Ruby scripting, fantasy cartography and a separate flat-map editor; native 68k support; arbitrary provider plugins; Perlin decoration; full immutability, universal coroutines, threading and a general-purpose compositor. Research issues should be created only when an objective, owner, scope and testable outcome emerge.

## Current / historical source material

- [Architecture as built](architecture.md), [ADR history](adr/0001-m2-to-m5-decisions.md), [progressive-resolution implementation](progressive-resolution-plan.md), [OS 9 evidence](os9-proof.md), [benchmark history](perf/README.md), and [live agent handoff](handoff.md).
- [Historical GL backlog](history/backlog-through-2026-11-05.md) contains GL-1 through GL-55 and their previous, sometimes superseded statuses; **do not treat it as an active to-do list**. New GitHub issues preserve relevant GL identifiers in their bodies.
- Migration note: one accidentally duplicated resource-lifecycle issue [#27](https://github.com/jackwillis/globe/issues/27) was closed as duplicate of [#26](https://github.com/jackwillis/globe/issues/26).

## Tracker conventions

1. **Milestones:** v0.1, v0.2 and v0.3, with no dates. An issue's milestone follows the **Release theme** line in its body. The original six epics (#1–#6) and the cross-cutting atlas-production epic #52 span releases and carry no milestone. Avoid an evergreen catch-all Future milestone.
2. **Labels in use:** priority `P0`–`P3` (`P3` is reserved and unused so far); area labels `area: classic-mac`, `area: rendering`, `area: architecture`, `area: discovery-design`, `area: data` and `area: release`, used across the original six epic areas and the new data-production epic #52; `epic`. The first research issue #53 is labeled `P1` and `area: data`. Some newer issues also carry `enhancement` or `bug`.
3. **Epics:** each epic's body lists its tracked issues. The nested epic [#35](https://github.com/jackwillis/globe/issues/35) (usability) sits under #4 and is also linked from #1 and #2.
4. Optionally convert epic checklists into native GitHub sub-issue relationships. The issue cross-references already provide usable navigation and will remain stable.

**Source of truth:** GitHub issue state, not copied status tables or old GL notes. Verify current code and Actions before closing a ticket. No release tags, server infrastructure or automated machine actions are authorized by this roadmap.
