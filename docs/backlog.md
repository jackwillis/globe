# Globe — groomed backlog

**As of 2026-11-03; checked against `main` at `dae1ef3`.** This is the **current planning index**, not a substitute for [design decisions](adr/0001-m2-to-m5-decisions.md), [architecture as built](architecture.md), [performance evidence](perf/README.md), or [agent handoff/safety rules](handoff.md). Historical plans and rejected experiments remain in the ADR and experiment notes, **not** as live instructions here. GL IDs are internal backlog identifiers, not GitHub issue numbers.

## Product and engineering policy

- **Keep the Elm-style Model / Msg / update / Effects architecture.** Mutate small Model state only through `update()`; make published geographic resources and per-job render inputs logically immutable; keep framebuffers and execution workspaces mutable. No functional-programming rewrite.
- **Ship v0.1 on the existing engine.** No large coordinator, tiling, format, or rendering-pass refactor is a prerequisite unless a concrete release defect requires it.
- **No maintained server infrastructure.** Embedded/offline baseline always works; optional later imports/fetching use third-party or user-selected sources with licensing, capacity, and rate-limit checks. No developer-operated relay.
- **Two editions only:** small OS 9 floppy and full X11; no networking requirement on the v0.1 floppy. The existing HFS disk image fits; size is not evidence of runtime compatibility.
- **Measurements before optimization.** Use matched camera/trace, view, style, edition, and platform. Preserve final-frame pins for behavior-preserving refactors; measure worst continuation, responsiveness, presentation cost and end-to-end completion.
- **Constrained code:** portable C++20, no exceptions/RTTI or OS headers in the core, bounded allocations and operations, explicit big-endian verification, Retro68 PowerPC/68k compile probes.
- **Approval boundaries:** follow `docs/handoff.md` for machine access, data provenance, owner binaries and agent push policy; no changes to maul or deployment implied by this backlog.

**Definitions:** P0 = v0.1 blocker / verification; P1 = v0.1 release polish; P2 = focused post-v0.1 architecture and performance; P3 = later features; Parked = research only. **Status:** done, partial, open, proposed, parked. An implementation may be done on X11 and still open on OS 9.

## P0 — prove release candidates, stabilize CI

| ID | Status | Task | Next action / acceptance |
| --- | --- | --- | --- |
| GL-1 | **partial** | Native OS 9 hardware acceptance | **Tiger Classic successfully launched and rendered**, and settling was measured there; this is not native OS 9. On real OS 9 hardware verify launch, drag, zoom, cursors, resize/expose, idle and palette/colors; record results in `os9-proof.md`. |
| GL-2 | **done** | Publish the repository | `main` is public at `jackwillis/globe`; current owner controls publishing. |
| GL-5 | **partial / blocker** | Green GitHub Actions | First run `41576444739` passed cross-compile probes, tests, policy and formatting but failed **clang-tidy in `src/platform/bench/codec_bench.cpp`** (two conversions and `atoi`). Fix narrowly, rerun, then confirm previously skipped full-edition size reporting and artifacts. |
| GL-7 | **open** | Real-display G4 X11 timing | Repeat a fixed interactive script in the actual X11 desktop. Separate compute, packing, XPutImage/XSync, frame/input latency; preserve trace and machine settings. |
| GL-9 | **done, CI proof via GL-5** | Floppy sizing and HFS fork roundtrip | Local 1.44 MB HFS image and MacBinary fork roundtrip verified. Do not duplicate CI work here. |
| GL-10 | **open** | Minimum supported RAM / partition | Measure peak OS 9 memory in real use, choose minimum and preferred SIZE partition, check failure behavior instead of inferring from floppy bytes. |
| GL-54 | **proposed** | Cross-platform orchestration acceptance tests | Simulate refinement/presentation events, paused drag and release, resize, expose and cancellation. Explicitly verify that hidden precomputed frames are never presented before release; same logical results across adapters. |

## P1 — v0.1 shipping scope (no architecture rewrite)

| ID | Status | Task | Next action / acceptance |
| --- | --- | --- | --- |
| GL-8 | open | Native Mac polish | Menu shortcuts, About and data credits, quit, icon; preferences only if small enough for release. |
| GL-11 | open | Publish v0.1 floppy edition | Versioned disk image + MacBinary, screenshots, license/attribution, requirements, hardware-verified notes; depends on GL-1, GL-5, GL-10. |
| GL-46 | **proposed** | Precise command invalidation | `CommandIssued` currently treats every command as view input: even drag-mode toggle or Quit restarts the stage/generation. Distinguish no-op/interaction/overlay/style/camera/geometry changes with small tests. **Do not** redesign all effect types at once; consider as v0.1 bugfix if tests demonstrate avoidable rerender. |
| GL-50 | **proposed** | Quantify worst-case cooperative scheduling | Log per-`step()` worst wall time, max input-to-visible response, first refine, total refine and presentation. Distinguish outer 24-ms slice from inner step bounds; no promise of fixed 8–12 ms until measured on G4. |

## P2 — focused architecture and performance, generally after v0.1

| ID | Status | Task | Next action / acceptance |
| --- | --- | --- | --- |
| GL-12 | proposed | Renderer surface abstraction | Prototype minimal RGB565 surface ops only if necessary for other scoped work. Measure call overhead on G4; don't introduce virtual abstraction by default. |
| GL-13 | proposed | Semantic colors and theme | Move style values from raster operations into lightweight palettes/themes; retain pixel-identical current looks unless explicitly changing art. |
| GL-14 | **open / consolidate** | Refactor refinement passes | Current `RefinementJob::step` uses numeric phases (`-3,-2,-1,0,7,8` etc.) and multiple cursors. Start with **GL-47**; later separate preparation/terrain/lines without changing order or final pixels. GL-6 is already complete, not a new task. |
| GL-15 | proposed | Recording / primitive stream surface | Compare benefit against existing framebuffer golden tests before implementing. |
| GL-16 | **re-scope** | QuickDraw surface experiment | OS 9 already uses GWorld + CopyBits. If profiling justifies it, A/B test direct QuickDraw primitives against current software framebuffer; **not** a prerequisite to the OS 9 edition. |
| GL-18 | proposed | Auto quality and presentation profile | Respect user-selected layer visibility; tune only work/simplification/presentation. Requires reliable GL-7/GL-50 measurements. |
| GL-45 | proposed / experiment | Cache 2–3 settled frames | At 640×480 RGB565 each frame is ~600 KiB, plus presentation buffers; measure benefit under style/layer toggles, memory ceiling, and invalidation complexity **before** adopting. |
| GL-47 | **proposed** | Name refinement phases and extract phase helpers | Replace magic phase numbers with typed states; no algorithm or pacing change; final framebuffer hashes, cancellation tests and G4 timing within noise. First step toward GL-14. |
| GL-48 | **proposed** | Portable application/render coordinator | Remove duplicated creation/cancel/wake/dirty/presentation lifecycle from X11 and OS 9 frontends. Keep platform-specific event loops, QuickDraw/X11 submission, native UI. Extract narrowly after GL-46/47; avoid a generalized framework. |
| GL-49 | **proposed** | Immutable data vs mutable render workspace boundary | Document and test that atlas/elevation inputs and per-job camera/plan snapshots don't change, while `TerrainScene` per-view projection/pass arrays and job cursors remain mutable. Split ownership only where replacement/multiple views require it. |
| GL-53 | **proposed / benchmark-only** | Dirty-region vs canonical final repair | Current circle refinement repaints visible cells and repairs line crossings for canonical pixels. Prototype clipped dirty-region redraw, compare total CPU, CopyBits/X11 uploads, extra memory and pixel equality. **Do not assume faster.** |

## P3 — product features and data roadmap

| ID | Status | Task | Next action / acceptance |
| --- | --- | --- | --- |
| GL-17 | proposed | Classic outline style | Optional style/profile, especially monochrome/low-depth; measure and define visual acceptance. |
| GL-20 | open | Layers palette | X11 first, then OS 9; saved preferences when relevant; do not mix with style palette GL-42. |
| GL-21 | open | Find place, larger city catalog | Search plus bounded offline Wikidata CC0 pack. |
| GL-22 | **X11 done** | Day/night terminator | Time comes through platform-message seam; static/offline behavior remains intact. | Done 2026-11-04 (X11): `D` toggles night shading (`shade_night`, `subsolar_point` in `daynight.hpp`), drawn on the shown copy of the frame so the rendered frame and every pin are untouched; sun from the system clock, refreshed at most every 30 s when an image is shown (no timer). G4 cost of the per-pixel pass is not measured; OS 9 not wired.
| GL-23 | open | Initial view from Mac Map control panel | Confirm Retro68 API availability and user fallback first. |
| GL-24 | open | Copy view as PICT | Begin bitmap; vector export only if useful and tested. |
| GL-25 | **X11 done** | Great-circle distance tool | Pure geometry and tests against known city pairs; independent of online routing. | Done 2026-11-04 (X11): `M` toggles measure mode; the first click marks a point, the second opens a Distance card (km and miles, both ends, initial bearing) and draws the great-circle arc, the third starts over (`great_circle_km`, `initial_bearing_deg`, `draw_measure` in `measure.hpp`; haversine on the mean Earth radius 6371.0088 km, tested against London-New York, Paris-Tokyo, antipodes and pole-to-equator; mutation-checked). Drawn on the shown copy, pins untouched. OS 9 not wired.
| GL-28 | **partial** | Great Lakes and runtime lake/rivers vectors | Lake water-mask builder done; vector layers belong to GL-35, not duplicate implementation. |
| GL-29 | **partial** | Runtime compression and finer packs | G4 codec decode benchmarks now exist (1-degree ~19 ms; 0.25-degree ~301 ms). Remaining: versioned compression container, peak-memory bound, runtime import, packaging/target measurements. |
| GL-30 | proposed | 68k / earlier Mac feasibility | Compile probes exist, but runtime feasibility and FPU cost remain unverified. No promise of 68k compatibility. |
| GL-31 | **design complete; implementation gated** | Full-edition HTTPS pack acquisition | Existing `fetch-design.md` specifies BearSSL/seed/OT/validation/security. First decide legally usable provider/host and pack scope; implement only after resource lifecycle GL-55. No fetch in initial floppy edition. |
| GL-32 | partial / visual review | Make deserts readable | Map look now uses RESOLVE land classes; confirm visually on actual examples before adding a new climate data format. |
| GL-33 | **done** | Map look | Current default Map and Relief styles implemented. |
| GL-34 | **partial / re-scope** | Land class data / Natural look | RESOLVE class grid and Map colors already exist. A distinct Natural style is **not** a release requirement; do not reimplement the grid. |
| GL-35 | open | River and lake vector overlays | Confirm source/license/size, G4 costs, and whether v0.1 genuinely needs it. |
| GL-36 | proposed | Thematic color schemes | Distinct optional palette feature; check ColorBrewer attribution and avoid duplicating GL-13. |
| GL-37 | proposed | Satellite-like precomputed color grid | Offline texture/pack experiment, not live imagery; license, RAM and performance first. |
| GL-38 | proposed | Refine-only antialiasing | Probe cost on G4 and low-depth compatibility; intentional pixel-golden change. |
| GL-39 | proposed | Climate/biome information | User-facing facts and bounded optional data; investigate provenance, licenses and simple UI. |
| GL-40 | **design only** | Glacier/ice bitplane | Requires explicit GELV format-v2 decision before changing builder or decoder. |
| GL-42 | **partial** | Styles palette | X11 `P` palette done; native OS 9 palette later. |
| GL-44 | **partial** | Focused country highlighting | X11 outline done; dithered fill experiment and OS 9 picking not done. |
| GL-51 | **proposed / design** | Offline-first acquisition/conversion pipeline | No developer-hosted infrastructure: immutable embedded atlas, user imports, cache, provider adapters, bounded conversion to native packs, attribution and rate limits. Distinguish public source formats from ready-to-use Globe packs. Build on GL-31/55; avoid unsupported arbitrary URLs. |
| GL-52 | **proposed / research** | Screen-space local LOD | Keep existing global elevation threshold for now. Once local detail packs exist, benchmark projected-error prioritization, central-view bias, horizon silhouette protection and coarse-parent fallback. Not a v0.1 requirement. |
| GL-55 | **proposed / prerequisite to GL-31 implementation** | Resource ownership and install lifecycle | Stable typed handles; immutable published pack storage; explicit disk-cache versus resident-memory states; staged candidate validation and failure-safe replacement; cancel/quiesce jobs borrowing old scenes; platform crash/relaunch tests. Avoid shared_ptr everywhere. |

## Completed work retained as history (no further tickets implied)

- **GL-3/4/6/9:** click rules, documentation refresh, line culling optimization, and floppy reports done.
- **GL-19:** alternative refinement orders measured and rejected; **circle-only from view center** is the current decision.
- **GL-26:** focus/choice experiment concluded; view-center focus retained. No choice UI is needed for removed orders.
- **GL-27:** terrain rendering and current Map/Relief appearance implemented, performance still worth measurement.
- **GL-41:** paced circle-outward progressive lines, canonical repair, 15-ms scheduled pauses implemented.
- **GL-43:** 400-ms delayed single-click picking implemented in core/X11; not OS 9 picking.
- **GL-2/GL-33:** GitHub publication and Map look complete.

## Parked ideas — not implementation authorization

| Idea | Why not now | Revisit |
| --- | --- | --- |
| Ruby scripting and fantasy cartography / separate map editor | Requires stable document model and user-facing commands; not a globe-release dependency | After native layers and file/document model |
| Separate detailed flat map / routing app | Different scale, tile/provider constraints; don't force current globe into a street-level app | After data store and LOD are tested |
| Perlin noise | Fine for restrained decoration, not a substitute for real Earth elevation | After accurate terrain and consistent performance |
| Multithreading / C++ coroutines / universal scheduler | Cooperative jobs are proven; added complexity without a demonstrated bottleneck | Only after profiling a specific use case |
| Fully immutable Model or Elm rewrite | Small in-place `update(Model&, Msg)` is deterministic and appropriate | Not planned |
| Large-pass architecture or generalized UI framework before v0.1 | High churn, little release value | After shipped acceptance |
| Server/relay owned by developer | Conflicts with product goal | Not planned |

## Next recommended development slices

1. **GL-5:** fix the three clang-tidy findings and obtain a green complete CI run.
2. **GL-1 + GL-7 + GL-10:** hardware acceptance, real X11 timing and OS 9 memory ceiling.
3. **GL-46 + GL-54:** test and narrowly fix unnecessary invalidation; verify pause-drag/presentation lifecycle.
4. **GL-8 + GL-11:** polish and publish the floppy release.
5. **GL-47**, then optionally **GL-48/49/50:** incremental architecture improvements **after** release, preserving pixels and G4 performance.
6. **GL-55 → GL-31/51 → GL-52:** resource-lifecycle contract first, then safe optional fetch/conversion, then localized LOD.

**Status caution:** Local/CI documentation and historical addenda may lag later commits. Recheck current code and actual CI before claiming a ticket is finished; this backlog never authorizes destructive machine operations or an agent-initiated deployment.
