# Globe — roadmap and issue tracker

> **Active planning now lives in [GitHub Issues](https://github.com/jackwillis/globe/issues).** This document is a lightweight map of the work, not a second backlog. The previous GL-numbered tracker is preserved verbatim in [docs/history/backlog-through-2026-11-05.md](history/backlog-through-2026-11-05.md). Last migration review: **2026-11-05**, source state through `f8b8159`.

## Planning policy

- **Keep Elm-style `Model / Msg / update / Effects`.** Application decisions remain deterministic; small Model state changes in `update()`. Published geographic input and per-job view plans are logically immutable, while framebuffers and rendering workspaces remain mutable.
- **Offline-first, no maintainer-operated services.** A bundled atlas remains fully usable without downloads. Optional fetching awaits vetted hosts, provenance, resource safety and consent.
- **v0.1 does not wait for speculative architecture work.** Hardware acceptance, complete CI verification, requirements, attribution and release packaging (StuffIt archive, MacBinary, disk image) are release priorities.
- **Adaptive by default:** Globe adapts to the computer, rather than requiring the user to configure the computer for Globe. Memory and geographic detail are managed automatically; advanced resource controls and developer diagnostics remain optional.
- **Progressive computation is a core technique; presentation is a UX hypothesis.** Compare current expressive reveal, minimal-delay and adaptive variants with representative users and on the real G4. Do not impose theatrical delays or weaken accessibility.
- **Evidence before optimization:** match view/style/hardware; measure input responsiveness, worst continuation, first useful frame, total completion, copy/upload cost and pixel correctness.
- **Safety:** honor [agent contract](../AGENTS.md) for protected machines, worktrees and release authorization. The tracker authorizes planning, not unattended device access or publication.

## Product direction — 2026-11-10

The [owner product brief](design/product-direction-2026-11-10.md) establishes Globe as an offline-first, free-software geographic hypermedia atlas. The next development emphasis is one coherent identity → administrative regions → interactive labels → linked cards/navigation slice, exemplified by United States → Wisconsin → Madison → Back to Wisconsin. This is planning direction, not an expansion of v0.1 acceptance.

[Draft ADR 0005](adr/0005-entity-identity-and-navigation.md) records identity/concordance and reference semantics; the [minimum shared interface proposal](design/entity-navigation-contract.md) defines research steps and responsibility boundaries. The journey's acceptance criteria are defined once, in that contract; no separate tracker is proposed. Frontend presentation parity is the distinct question tracked in #114. Existing issues retain implementation ownership and the live queue remains authoritative. The v0.2 milestone row above is unchanged; this direction does not alter any milestone or gate.

Structured facts and readable linked articles are both eventual goals. Search, favorites, richer geography, naturalistic land cover/urban rendering, native speech and Earth-and-sky context remain future themes; no new milestone commitment or approved source acquisition follows. Preserve native single-window/sidebar usability at 640×480, installed Helvetica/Palatino with fallbacks, and the embedded offline atlas.

## Release themes

These are the four **thematic** milestones (v0.1 through v0.4), established without due dates and restructured on 2026-11-10 (owner decision). Separate **patch-release** milestones v0.1.1 and v0.1.2 now capture incremental work; they do not replace the broader v0.2 outcome. v0.1.0 was published on 2026-11-13. The epics (#1–#6) span releases and carry no milestone. Parked experiments (#15, #16, #19, #21, #22) have no milestone.

| Milestone | Outcome | Practical gate |
| --- | --- | --- |
| **v0.1 — The Classic Atlas** | The application as built on `main`: single window with sidebar, menus, palette, About and Preferences | [Hardware acceptance: v0.1](https://github.com/jackwillis/globe/issues/95), CI, verified minimum memory (#10), essential Mac polish (#11), fork-preserving release (#47, #12) |
| **v0.2 — The Atlas on the Map** | Text, labels, pointer readout, scale bar and compass, city symbols, help overlay, render performance | Placement function and label rules (#36, #38), measured cost on the G4, hardware acceptance for the release |
| **v0.3 — Memory and Packs** | Memory ledger and budget, low-memory handling, stable resource handles, versioned packs, external packs on Classic Mac OS, Preferences handling | Memory regression gate, tested degradation ladder, safe pack lifecycle (#55, #56, #58, #26, #31, #48, #49) |
| **v0.4 — The Expandable Atlas** | Richer optional data and editions | Full-edition data and volume layout (#50, #51), a third look (#75), optional acquisition (#28), CD-ROM research (#72) |

### Incremental releases and milestone graduation (owner decision, 2026-11-13)

**Ship useful slices frequently; graduate milestones deliberately.** While Poodle Earth is pre-1.0, `0.1.x` releases may include compatible new features as well as fixes, documentation and polish. Each patch must be independently usable, versioned and packaged, with applicable CI, native-hardware acceptance, documentation, attribution and explicit owner publication approval. This is the project's documented pre-1.0 convention, **not a claim of strict post-1.0 Semantic Versioning patch semantics**.

A **minor release** (`0.2.0`, `0.3.0`) means that its broader product milestone's *agreed* acceptance criteria work together and have been verified, not that a large number of new features must first land in that release. Completing or deferring scope is an owner decision with an issue record; merely exhausting a planned patch sequence does not authorize a minor release. Keep hardware acceptance and release scope explicit for **every published version**, including patches.

| Version | Incremental outcome | Status and ownership |
| --- | --- | --- |
| **v0.1.0** | First public Classic Mac atlas | Published 2026-11-13; preserve its historical claims and artifacts |
| **v0.1.1** | Public-facing polish: README, real OS 9 images and guide, project website, changelog, agent cold-start validation and repo rename; visible low-memory resize feedback | Existing patch milestone, issues [#118](https://github.com/jackwillis/globe/issues/118), [#129](https://github.com/jackwillis/globe/issues/129), [#125](https://github.com/jackwillis/globe/issues/125), [#121](https://github.com/jackwillis/globe/issues/121), [#124](https://github.com/jackwillis/globe/issues/124) and [#116](https://github.com/jackwillis/globe/issues/116) |
| **v0.1.2** | Text/layout foundation, installed-font roles and **settled-frame country labels** | Existing patch milestone, issues [#36](https://github.com/jackwillis/globe/issues/36), [#61](https://github.com/jackwillis/globe/issues/61) and [#119](https://github.com/jackwillis/globe/issues/119) |
| **v0.1.3+** | Candidate slices: city labels/symbol hierarchy, pointer readout, map scale and compass, help, cards, Preferences, accessibility and frontend coherence | **Provisional examples only**—no versions, milestones, issue moves or feature gates approved; choose slices from measured progress after v0.1.2 |
| **v0.2.0** | **The Atlas on the Map**: integrated, understandable geographic UI with accepted v0.2 requirements, performance and real-hardware evidence | Thematic milestone graduation, tracked through [#35](https://github.com/jackwillis/globe/issues/35) and the v0.2 milestone; not merely another arbitrary patch |

After v0.2.0, **v0.2.x** patches can incrementally build toward **v0.3.0 — Memory and Packs** under the same policy; do not promise the number or content of those future patches.

**Platform feature-parity policy (owner decision, 2026-11-13).** Feature development and *published guarantees* have different cadences:

- **Patch releases** (`x.y.z` with `z > 0`) may ship new functionality on just one supported frontend, with the other frontend(s) catching up later. Release notes and feature issues must name exactly which frontends support the feature and which do not. This is intentional, documented parity debt, not an implicit promise of a matching UI in that patch.
- **Minor and major releases** (`x.y.0`, including `x.0.0`) are **functional-parity checkpoints** for the **owner-approved core feature set** across the frontends supported by that release (currently Classic Mac OS and X11). Equivalent user outcomes are required, **not pixel-identical widgets or identical native interaction conventions**. Only explicit owner-approved platform-specific exceptions, recorded with rationale in the release issue and notes, may remain.
- **At every release**, portable core/build compatibility, regression avoidance, safety, data integrity and the already-required testing/acceptance for artifacts actually shipped remain mandatory. An OS 9-first feature must not break the X11 build or existing behavior; cross-compilation alone is not native acceptance.
- **Track parity debt as features land:** each platform-specific issue/PR identifies frontend coverage, missing equivalents and a linked catch-up task or acceptance item. Plan and integrate catch-up throughout the cycle when practical; verify and close or expressly waive the remaining debt at the minor/major graduation gate. See [#114](https://github.com/jackwillis/globe/issues/114) for exploratory shared-presentation research; parity does not require adopting a shared UI abstraction.
- This policy applies **prospectively**; it does not reinterpret historical v0.1.0 claims, change supported-platform declarations, automatically move issues/milestones, or authorize release-scope changes, hardware access or publication.

**GitHub tracking:** An issue can have only one milestone. Assign concrete deliverables to the *specific approved release milestone* when ready; use the cross-release epic ([#35](https://github.com/jackwillis/globe/issues/35) for v0.2 usability), issue links and the thematic milestone's remaining integration/acceptance work to track the full outcome. Avoid double-counting issues or insisting that every exploratory idea must ship before graduation. The release roadmap is explanatory; GitHub issues, labels and milestones are the live state. **This policy does not assign work, set `status: ready`, move issues, change release scope, create release tags or authorize hardware access.**

**Hardware acceptance per release (owner decision, 2026-11-10).** Each release gets one "Hardware acceptance: vX.Y" issue with a fixed checklist, a run record (hardware, OS, build, StuffIt Expander version) and evidence per item. #8 was closed and its open items carried into the v0.1 issue. #9 and #10 stay as measurement issues that feed it. Proposal, not adopted: use the Classic Mac `vers` resource stages (for example 0.2b1) to label candidate builds, and run the acceptance issue against each candidate.

No due dates or fixed feature commitments have been set.

### Editions and distribution (owner decision, 2026-11-07)

- **The 1.44 MB floppy edition is paused.** It stays buildable and its build target stays in CI, but it is no longer a release target and size squeezing stops. Its first bare OS 9 run (2026-11-07) remains the performance baseline.
- **Distribution is digital first:** a StuffIt archive as the headline download, with the MacBinary file and a raw HFS disk image as fallbacks ([#47](https://github.com/jackwillis/globe/issues/47), [#12](https://github.com/jackwillis/globe/issues/12)). Physical media (CD-R, floppy) come later and are optional.
- **A larger edition ships as an application plus a data folder**, laid out like a CD and published as a disk image first: external data packs on Classic Mac OS ([#48](https://github.com/jackwillis/globe/issues/48)), running from a read-only volume ([#49](https://github.com/jackwillis/globe/issues/49)), the full-edition data ([#50](https://github.com/jackwillis/globe/issues/50)) and the volume layout ([#51](https://github.com/jackwillis/globe/issues/51)).

## Layered atlas and optical media — working strategy (2026-11-09)

Full synthesis: [Layered atlas and CD-ROM research](research/cd-rom-atlas-architecture.md). Inspiration: 1990s CD-ROM geographic software, especially Microsoft Encarta World Atlas, approached as a **FOSS contemporary** with reproducible data, documented provenance and semantic integrity—not a reproduction of proprietary assets or code.

**Agreed product direction:** an immediately usable Earth bundled into the executable, with optional external finer world/regional data. The same renderer and automatic managed-RAM policy work with standalone, copied-to-disk, read-only image or eventual CD distributions. There is **no Full/Compact startup chooser**. The embedded atlas is the reliable fallback; optional resources are validated, loaded/evicted safely, and must not block active navigation.

**Research, not decided:** index design (grid/quadtree/hybrid), regional versus typed containers, chunk dimensions, compression, read-ahead, CD seeking, disc removal, installation UX and the exact atlas contents. Favor representative benchmarks before format changes. See [#31](https://github.com/jackwillis/globe/issues/31), [#48](https://github.com/jackwillis/globe/issues/48), [#50](https://github.com/jackwillis/globe/issues/50), [#51](https://github.com/jackwillis/globe/issues/51), [#57](https://github.com/jackwillis/globe/issues/57), and new [#72 — CD-ROM atlas history / G4 optical benchmarks](https://github.com/jackwillis/globe/issues/72).

**Proposed progression (not new release gates):**
- **v0.1 — Classic Atlas:** a standalone, useful embedded globe on actual Classic Mac OS, verified memory and digital distribution.
- **v0.2 — The Atlas on the Map:** labels, readout, scale bar and compass, city symbols, help overlay and render performance (usability core).
- **v0.3 — Memory and Packs:** the memory ledger and budget, safe resource lifecycle, versioned packs, external packs and read-only loading proof, Preferences handling.
- **v0.4 — Expandable Atlas:** measured rich editions, regional loading/eviction, distributable read-only disk image; optical retrieval research can happen but no mandatory physical CD.
- **Possible later — Disc Atlas:** contingent future concept for seek-aware indexed geography, CD/HDD comparison, larger curated packs, optical-media compatibility and graceful removal. **No milestone for it exists or is authorized by this note.**

The atlas builder (#52) owns source curation, geographic identity, editorial completeness and packaged datasets; the runtime (#5/#54) owns storage adapters, bounds, resource lifetime, responsive refinement and memory management. The same small indexed pack must not be assumed to have a good CD physical layout until measured. The Classic Mac application must not write beside itself on read-only volumes (#49). Preserve v0.1's release focus.

## Epics (parent tracking issues)

| Epic | Outcome | Active issue |
| --- | --- | --- |
| Classic Macintosh experience | Hardware-native, responsive and accessible Classic UI | [#1](https://github.com/jackwillis/globe/issues/1) |
| Responsive progressive rendering | Interruptibility, perceptual performance and purposeful presentation | [#2](https://github.com/jackwillis/globe/issues/2) |
| Portable application architecture | Evolve Elm-style state, immutable data, mutable workspaces | [#3](https://github.com/jackwillis/globe/issues/3) |
| Geographic discovery and visual design | Useful offline geography, search, layers and visual clarity | [#4](https://github.com/jackwillis/globe/issues/4) |
| Offline data and acquisition | Validated packs, safe cache and optional provider ingestion | [#5](https://github.com/jackwillis/globe/issues/5) |
| Memory budgets and runtime resource management | One memory ledger and budget, a tested degradation ladder, honest metrics, Classic Mac Memory Manager handling | [#54](https://github.com/jackwillis/globe/issues/54) |
| Atlas production and geographic integrity | Reproducible data pipeline, editorial rules, identity, semantic QA and edition recipes | [#52](https://github.com/jackwillis/globe/issues/52) |
| Release engineering and verification | Reproducible CI and builds, target evidence | [#6](https://github.com/jackwillis/globe/issues/6) |

Each epic includes a linked checklist of its tracked work. These are ordinary GitHub Issues with cross-references; **native GitHub parent/sub-issue relationships have not been configured**. The repository milestones v0.1–v0.4 exist.

## Direction since 2026-11-07: usability

The owner's call after the rendering and responsiveness work: **make Globe an atlas worth looking at
for more than a minute.** Umbrella issue [#35](https://github.com/jackwillis/globe/issues/35)
(milestone v0.2, under epic #4, touching #1). Order: the Classic Mac OS gaps first (cursors #44, looks
palette #45, menu bar #46), then render tiers with a first-run speed test (#34, moved to v0.2), the
help overlay (#40), and the atlas features: text foundation (#36), pointer readout (#37), labels
(#38), scale bar and degree labels (#39), animated zoom (#41), remembered view (#42), image export
(#43); existing #24 (find places), #23 and #25 stay linked. Decisions and evidence:
[ADR 0002](adr/0002-rendering-shared-code-and-usability.md).

**Added 2026-11-09 (owner testing on the G4 and a UI audit):**

- **Layout:** a deliberate Classic Mac workspace ([#77](https://github.com/jackwillis/globe/issues/77)), then one window with a globe pane and a sidebar ([#79](https://github.com/jackwillis/globe/issues/79)), and the same on X11 ([#83](https://github.com/jackwillis/globe/issues/83)). Wrapped information-card text ([#80](https://github.com/jackwillis/globe/issues/80)), About in its own window ([#82](https://github.com/jackwillis/globe/issues/82)) and selectable card text ([#87](https://github.com/jackwillis/globe/issues/87); the TextEdit swap is paused, a Copy action comes first). The 2026-11-09 UI audit is [docs/ui-audit-2026-11-09.md](ui-audit-2026-11-09.md) ([#90](https://github.com/jackwillis/globe/issues/90), a proposal, not an authorization).
- **Preferences and language:** a Preferences window ([#89](https://github.com/jackwillis/globe/issues/89)) with units ([#85](https://github.com/jackwillis/globe/issues/85)) and live language switching from string files ([#86](https://github.com/jackwillis/globe/issues/86); strategy question [#84](https://github.com/jackwillis/globe/issues/84)). Settings are per-session until [#49](https://github.com/jackwillis/globe/issues/49) settles Preferences handling. These landed on `main` with host tests; the issues note they have not all been seen on a real display.
- **Atlas furniture:** scale bar and compass rose ([#81](https://github.com/jackwillis/globe/issues/81), needs a design decision for an orthographic globe) and city symbols by size class with a distinct capital symbol ([#88](https://github.com/jackwillis/globe/issues/88)). Wake-from-sleep redraw measurement is [#78](https://github.com/jackwillis/globe/issues/78).
- **Typography:** installed Helvetica and Palatino, no bundled fonts; see [design/typography.md](design/typography.md) and [#61](https://github.com/jackwillis/globe/issues/61). The mouse proposal (Option-drag momentary trackball, Option-double-click zoom out, a menu item for every gesture) is recorded on [#44](https://github.com/jackwillis/globe/issues/44) and [#46](https://github.com/jackwillis/globe/issues/46) and is not yet assigned.
- **Render performance:** [#65](https://github.com/jackwillis/globe/issues/65) to [#67](https://github.com/jackwillis/globe/issues/67) are open as PRs #69 to #71 (host measurements only; unmerged); the limb completion bug is [#74](https://github.com/jackwillis/globe/issues/74), which did not reproduce on the host.
- **A third look:** [#75](https://github.com/jackwillis/globe/issues/75), a Terrain Atlas design proposal, not an approval.

## Adaptive memory and developer diagnostics (owner decision, 2026-11-08)

The proposed **Full/Compact atlas selection screen at startup is superseded**. Globe launches directly into the embedded, offline-capable atlas; geographic resource loading, retention and eviction adapt to the actual available budget. Disk-distributed atlas editions and optional packs remain distinct from automatic *runtime* memory management.

- **Minimal memory UX:** [#56](https://github.com/jackwillis/globe/issues/56) owns Automatic as the default and a single optional Advanced preference for a maximum *managed atlas memory* budget (not total app memory or the Classic Mac Finder partition). [#57](https://github.com/jackwillis/globe/issues/57) owns quality-preserving cache eviction and safe fallbacks. Routine low-memory behavior is silent; only visibly unavailable detail merits a restrained, nonmodal notice.
- **Detailed logs:** [#62](https://github.com/jackwillis/globe/issues/62) adds a separate optional activity log window on Classic Mac OS and X11, informed by WhatItBeLike's existing diagnostics. [#59](https://github.com/jackwillis/globe/issues/59) remains the owner of underlying metrics and measurement semantics.
- **Developer console:** [#63](https://github.com/jackwillis/globe/issues/63) explores an optional Forth REPL for observation and controlled navigation/debugging commands. It is developer tooling, not a prerequisite for the log window, normal atlas operation or a general third-party scripting API.

Keep these as separate tracked changes rather than expanding the memory-management epic to own the console and log UI. No new v0.1 release gate or unapproved runtime implementation is implied.

## Atlas production as a parallel project (2026-11-07)

The owner approved treating Globe as **two cooperating projects in one repository**: the portable interactive runtime and a reproducible, transparent atlas-production pipeline whose source selection, editorial rules, geographic QA and edition recipes can evolve separately. The interface is versioned, strictly bounded atlas packs; no new runtime GIS dependency or infrastructure is implied. See [ADR 0003](adr/0003-atlas-production-boundary.md).

- [#52 — Atlas production, geographic integrity and edition rules](https://github.com/jackwillis/globe/issues/52) coordinates data-side decisions, alongside existing data epic #5 and discovery epic #4.
- [#53 — Audit atlas coverage and prototype source-driven editions](https://github.com/jackwillis/globe/issues/53) is the first **research-only** assignment: source/provenance and omissions audit, microstates/capitals, rivers/lakes, three candidate edition sizes and a semantic QA plan. No production-format or runtime migration is authorized yet.
- **Working arrangement:** Data-side work proceeds as a separate reviewable research stream, independent of the Classic Mac UI backlog. Any production pack, loader, or renderer integration follows a subsequent reviewed decision; keep source downloads and raw large datasets outside the Git repository unless their use and licensing are explicitly approved.
- Existing [#24](https://github.com/jackwillis/globe/issues/24), [#26](https://github.com/jackwillis/globe/issues/26), [#29–#31](https://github.com/jackwillis/globe/issues/29), [#36–#39](https://github.com/jackwillis/globe/issues/36), [#48](https://github.com/jackwillis/globe/issues/48) and [#50](https://github.com/jackwillis/globe/issues/50) remain the related work; avoid duplicate implementation tickets.

- Research follow-ons under #52: country-aware capital-first settlement selection and duplicate QA ([#91](https://github.com/jackwillis/globe/issues/91)) and curated ocean, sea, gulf and bay labels ([#92](https://github.com/jackwillis/globe/issues/92)). Both are research only. The research tree is `atlas/` (see `atlas/reports/recommendation.md`); it changes no production pack.

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

- [Architecture as built](architecture.md), [ADR history](adr/0001-m2-to-m5-decisions.md), [progressive-resolution implementation](progressive-resolution-plan.md), [OS 9 evidence](os9-proof.md), [benchmark history](perf/README.md), and [live agent handoff](history/handoff.md).
- [Historical GL backlog](history/backlog-through-2026-11-05.md) contains GL-1 through GL-55 and their previous, sometimes superseded statuses; **do not treat it as an active to-do list**. New GitHub issues preserve relevant GL identifiers in their bodies.
- Migration note: one accidentally duplicated resource-lifecycle issue [#27](https://github.com/jackwillis/globe/issues/27) was closed as duplicate of [#26](https://github.com/jackwillis/globe/issues/26).

## Tracker conventions

1. **Milestones:** v0.1 to v0.4 are the thematic outcomes, with patch milestones (currently v0.1.1 and v0.1.2) for deliverable increments; no dates. An issue belongs to only one release milestone, and the epic/issue links retain its broader theme. The original six epics (#1–#6) and the cross-cutting atlas-production epic #52 span releases and carry no milestone. Avoid an evergreen catch-all Future milestone.
2. **Labels in use:** priority `P0`–`P3` (`P3` is reserved and unused so far); area labels `area: classic-mac`, `area: rendering`, `area: architecture`, `area: discovery-design`, `area: data` and `area: release`, used across the original six epic areas and the new data-production epic #52; `epic`. The first research issue #53 is labeled `P1` and `area: data`. Some newer issues also carry `enhancement` or `bug`.
3. **Epics:** each epic's body lists its tracked issues. The nested epic [#35](https://github.com/jackwillis/globe/issues/35) (usability) sits under #4 and is also linked from #1 and #2.
4. Optionally convert epic checklists into native GitHub sub-issue relationships. The issue cross-references already provide usable navigation and will remain stable.

**Source of truth:** GitHub issue state, not copied status tables or old GL notes. Verify current code and Actions before closing a ticket. No release tags, server infrastructure or automated machine actions are authorized by this roadmap.
