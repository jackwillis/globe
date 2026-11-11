# Proposed tracking issue: geographic identity and navigation slice

**Status:** Ready for owner review; not filed. This is a proposed cross-cutting tracker, not another implementation epic or release gate.

## Intended experience

Select a country label to inspect it without moving the camera. Follow a region link, navigate to Wisconsin, inspect Madison, then use Back to restore Wisconsin's entity and camera. Cities, countries and regions remain independently selectable.

## Dependencies and ownership

Reuse #52/#53 for identity/concordance/data review, #35/#36/#38/#61 for usable native text and label interaction. Coordinate with #24 for future search, #54 for budgets and #26/#31/#48/#49 for later resource lifetimes and persistence. Codex owns catalog, portable contracts and deterministic tests; Claude Code owns Macintosh UI, metrics/drawing adapters and cards. Review shared interfaces together.

Also link #4 (discovery), #23 (interaction parity), #41 (animated recentre), #42 (remembered view/camera snapshot), #91 (settlement policy), #87 (copyable card text) and #84 (encoding). Animation is not a prerequisite for correct immediate navigation.

Architecture: [draft ADR 0005](../adr/0005-entity-identity-and-navigation.md). Experiment sequence and API boundary: [shared contract](entity-navigation-contract.md). These documents own semantics; this tracker owns acceptance rather than copying their full content.

## Prerequisites and owner decisions

The current source locks contain no admin-1 layer, Madison is absent from the shipped 304-city pack, and geographic decisions in the research overlay remain unapproved. The source-backed path cannot yet be built from reviewed repository data.

- **P1 (#53):** approve one pinned admin-1 fixture source only after provenance/license/coverage review; Natural Earth is a candidate, not an approved choice. Source-backed catalog experiment 1 waits for this decision.
- **P2 (#52/#53):** review entity granularity and US/Wisconsin/Madison identity/parent/anchor assertions through the basis_sha256-bound review workflow (`atlas/research/reviews.py`). Extend current kinds/validators explicitly where needed; no invented approvals.
- **P3 (#91):** decide a reviewed Madison fixture/places inclusion route. Do not lower the production one-million rule ad hoc or merge the experimental selection policy. A research-only tiny fixture can precede any separately approved production places migration.

Synthetic navigation and fixed-metric placement experiments proceed independently of P1–P3, using test-only references. Native adapter/hardware work retains its own authorization boundaries. Resolve P1 before filing this tracking proposal; no implementation issues are created by this revision.

## Scope boundaries

A tiny curated administrative catalog, fixed-capacity navigation prototype, native-label experiment and one integrated journey. No production-pack migration, broad encyclopedia import, general graph runtime, font renderer, full search/favorites system, external pack loader or automatic persistence. Preserve v0.1 gates and existing milestone structure. Target theme is v0.2, with its final gate reviewed after prototypes; this local proposal does not change GitHub milestone assignments. Use an always-embedded base catalog for v0.2. Evictable packs and persistence extend the reference contract in v0.3, under existing owners.

## Acceptance criteria and tests

- [ ] Reviewed source-backed United States/Wisconsin/Madison identities and relations; unresolved candidates remain separate from published claims.
- [ ] Additional fixtures cover microstates, a non-US administrative hierarchy, ambiguous settlement names and geographic edge cases.
- [ ] Test selection without camera motion; navigation, Back/Forward, forward-branch truncation and bounded capacity; restore relevant entity/camera state.
- [ ] Test stable references across reordered/detail editions, unavailable packs, unknown/retired IDs, stale results and changed viewport.
- [ ] Fixed-metric placement tests cover deterministic order, collisions, limb/viewport limits and label-level hit targets; native draw and hit tests use the same accepted placements.
- [ ] Replay-equal intent/result traces with exact parse/format round trips, including failure and stale-result cases; zero allocations in update; explicit history golden for Wisconsin → select Madison → Back → Forward.
- [ ] Measure prototype ABI sizes against the provisional 32-entry/2 KiB history, 128-candidate/64-label and 16 KiB resident-storage ceilings in the shared contract.
- [ ] Preserve frame/replay goldens, cancellation, responsive progressive rendering and offline operation. Host baseline: `cmake -S . -B build -G Ninja`, `cmake --build build`, `ctest --test-dir build --output-on-failure`; current relevant targets include `test_app`, `test_projection`, `test_overlays`, `test_countries`, `test_places`, `test_render`, `test_zoom`. Add dedicated navigation/FixedMetrics targets when implementation exists, rather than claiming they exist now.
- [ ] Run `tools/check-policy.sh` and `tools/probe-retro68.sh` for the implementation; ensure new headers are exercised by the probe on both PowerPC and 68k, and inspect every compile result (the current script can print FAIL without failing the shell). Report compile-only separately from hardware evidence.
- [ ] Record actual Classic Mac hardware journey, fonts, readability, responsiveness, memory and build size with OS/build identity, or explicitly leave hardware acceptance pending. Hardware use needs separate authorization.
- [ ] Use findings to scope narrow follow-ups under existing owners; do not close #53 or claim runtime functionality from research approval.
