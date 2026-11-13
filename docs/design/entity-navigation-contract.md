# Entity and navigation contract — proposal and acceptance journey

**Status:** Design-only, 2026-11-10, revised 2026-11-13. Implements no API. Read [ADR 0005](../adr/0005-entity-identity-and-navigation.md) (draft) for reference semantics and the [product brief](product-direction-2026-11-10.md) for product intent. Every number below is a **provisional prototype constraint**, not a decision.

## Scope and relationship to other work

This document is the single definition of (a) the minimum shared entity/navigation seam and (b) the **geographic navigation acceptance journey**. It does not define a presentation architecture.

- **Geographic navigation (this document):** what an entity reference is, how selection differs from navigation, how history restores state, and the journey *United States → Wisconsin → Madison → Back to Wisconsin*.
- **Frontend presentation parity ([#114](https://github.com/jackwillis/globe/issues/114), [PR #112](https://github.com/jackwillis/globe/pull/112)):** whether a shared presentation model reduces duplicated menu/action/card decisions between frontends. #114's vertical slice reuses the journey below rather than defining a second one, and any presentation snapshot is derived from the resolver results defined here.

No separate tracking issue is proposed. Work is owned by the existing issues: #52/#53 (identity, concordance and data review), #35/#36/#38/#61 (usable native text and label interaction), #114 (presentation parity), #24 (future search), #54 (budgets), and #26/#31/#48/#49 (later resource lifetimes and persistence). Related: #4 (discovery), #23 (interaction parity), #41 (animated recentre, not a prerequisite), #42 (remembered view), #84 (encoding), #87 (copyable card text), #91 (settlement policy).

## Responsibilities

Architectural responsibilities, independent of who implements them:

- **Atlas production and portable core:** catalog publication, value contracts, portable `update()` transitions, placement algorithm and deterministic tests.
- **Native frontends:** controls, information cards, text measurement and drawing adapters, interaction conventions, hardware evidence.
- **Shared boundary:** reference and snapshot types, owned result bounds, publication mapping, target framing, budgets. Review these together before adding shared headers or CMake targets, and avoid simultaneous shared-header edits.

## Minimum prototype seam

Five proposed concepts suffice: `EntityRef`, an owned `Resolution`/`EntitySummary`, a fixed-capacity navigation history, a compact camera snapshot, and `PlacedLabel`. A resolution carries status, correlation token, identity, optional anchor/framing and bounded display text. Keep relationship traversal inside the curated catalog resolver for the tiny path. Defer a general Relationship/Knowledge API; facts and articles remain product direction, not first-header prerequisites.

`SelectEntity`, `NavigateRequested`, `EntityResolved`, Back and Forward extend `Msg`; [ADR 0005](../adr/0005-entity-identity-and-navigation.md) owns their effect, replay and history rules. `EntitySummary` is the one proposed card-data representation; the existing `InfoCard` (borrowed `string_view`s) is what it would replace or wrap.

## Owned results and resource lifetime

No `string_view`, raw pointer or pack-local handle crosses `update` in a resolver result. Use a trivially copyable summary with ref, kind/status, optional quantized anchor and a bounded name with explicit byte length. Measure actual `sizeof`/padding and validate coordinate range and missing-anchor status. Oversized names require explicit failure or a reviewed fallback, never silent corruption.

The sidebar owns bounded copies of the displayed title/body. Present-day `InfoCard` borrows embedded bytes; optional resource support must replace or wrap that assumption before eviction is enabled. For v0.3, propose generation pin/unpin under #26/#57: obtain a view only after successful pinning, copy needed values, then unpin within the event-loop turn. No retained borrowed card after unpin. A missing card resource leaves the selected identity intact.

## Native text and portable placement seam

[Text layout](text-layout.md) remains the baseline: placement is portable, glyph drawing is native. For this slice propose one refinement, pending review by the native-frontend owner: `TextMetrics` keeps width/ascent/descent/leading/`has_glyphs` and adds conservative whole-label ink bounds (or conservatively expanded existing metrics when native ink bounds are unavailable). The cache key includes font, size, style, the actual fallback/encoding result and a `metrics_id`. `text-layout.md` is itself design-only, so this changes no implemented interface.

Measure and draw the same reviewed platform display string, including any Mac Roman fallback; never measure the UTF-8 source and draw a different string. Source/display policy stays with #84/#91. Missing glyphs drop the label while valid city-point and polygon selection remain available.

`PlacedLabel` fields: entity reference, candidate ID, draw origin, conservative whole-label ink rectangle, hit rectangle, face and priority. Text comes from owned/retained placement storage, not an evictable pointer. Clip interaction to both the viewport and the visible globe disk. Draw lower-priority labels first so hit-test order `(priority descending, candidate ID ascending)` matches visual stacking. A miss falls through to visible city-point picking, then polygon picking. The same accepted records drive drawing and picking; stale view/resource/metrics generations are rejected.

Use a fixed-metrics layout golden: alternate anchors, long names, missing glyphs, clipping, stable ranking/caps and overlapping padded hit targets. The first prototype recomputes placements deterministically without hysteresis; if hysteresis is needed later, pass bounded prior state explicitly through replayable messages. Native metrics must be recorded or injected as fixed inputs for any replay that includes placements; only semantic navigation replay is platform-independent by default.

**Glyph-level and bitmap approaches** (per-glyph ink bounds, native occupancy masks, rotated or curved text, rasterized glyph bitmaps) are *alternatives under #36*, evaluated against the native-drawing baseline in [text-layout.md](text-layout.md). They are not part of this slice and not adopted. Preserve authoritative framebuffer/replay goldens.

## Provisional prototype constraints

Starting points to validate or revise by measurement. Record actual ABI sizes and high-water usage; revise a bound before integration if the design exceeds it.

| Area | Provisional constraint | Basis / caveat |
| --- | --- | --- |
| Reference | `EntityRef` = `uint32_t`; zero invalid; high-bit range reserved for synthetic fixture IDs and rejected by production builders | Prototype choice pending format review. Existing `gcat:` allocations are preserved; no QID, source ordinal or polygon index becomes an ID |
| Summary text | Name buffer of 96 bytes plus explicit length | Motivated by the 95-byte legacy GPLC name limit, not a universal name policy. Do not assume an approximate struct size is portable; measure |
| History | 32 entries, at most 2 KiB including metadata | Geographic snapshot fields instead of `Camera` basis vectors and viewport |
| Duplicate suppression | Zoom tolerance 1e-9, pinned in tests | Together with equal identity and `same_orientation` |
| Placement | At most 128 candidates and 64 accepted labels, preallocated flat arrays | |
| Resident storage | At most 16 KiB of new runtime storage outside catalog packs (Model additions, pending results, placement, metrics and UI text buffers) | Catalog bytes, load/decode peaks, font caches and native drawing scratch are reported separately in the memory ledger, never hidden by this ceiling |
| Label hit targets | Padding of at least 3 px where viewport and limb permit | Usability to be confirmed on the target |
| Measurement work | 2 ms per native event-loop turn | About 10% of the earlier 20 ms bare-OS 9 observation; not a proven allowance. If a screenful exceeds it, cache measurement per settle/font change and process bounded batches |
| Memory frame of reference | Classic application 12 MiB minimum / 24 MiB preferred partition, not installed RAM | See [memory research](../research/classic-mac-memory.md). Targets to validate, not measured claims |
| Allocation | None in `update` or per-frame placement | |

## Prerequisites and owner decisions

The current source locks contain no admin-1 polygon layer, Madison is absent from the shipped 304-city pack, and geographic decisions in the research overlay remain unapproved. The source-backed path cannot yet be built from reviewed repository data.

- **P1 (#53):** approve one pinned admin-1 fixture source only after provenance, licence and coverage review. Natural Earth is a candidate, not an approved choice. The source-backed catalog experiment waits for this decision.
- **P2 (#52/#53):** review entity granularity and United States/Wisconsin/Madison identity, parent and anchor assertions through the `basis_sha256`-bound review workflow (`atlas/research/reviews.py`). Extend current kinds/validators explicitly where needed; no invented approvals.
- **P3 (#91):** decide a reviewed Madison fixture/places inclusion route. Do not lower the production one-million-population rule ad hoc or merge the experimental selection policy. A research-only tiny fixture can precede any separately approved production places migration.

Synthetic navigation and fixed-metric placement experiments proceed independently of P1–P3 using test-only references.

## Small experiments and independent progress

1. **Catalog fixture (atlas production):** extend existing research mechanisms with a tiny sourced administrative path after source/claim review. Include a microstate without useful coarse geometry, a non-US hierarchy, ambiguous settlement names, an island/antimeridian case and unresolved parentage. Record provenance; keep fixture ID allocation separate from synthetic IDs.
2. **Synthetic navigation (portable core, native review):** invented test-only references and fixed resolver responses to exercise selection versus navigation, history restoration/branching/capacity, failed/stale resolution, resource loss and viewport changes. No production pack or UI integration is needed. Steps 2 and 3 do not approve geography or production selection.
3. **Native metrics and placement (native adapter plus portable placement):** measured-label contracts, fixed-metric layout tests and one native label. Compare readability, collision rate, CPU, memory and build size. Host figures are not G4 evidence.
4. **Integrated slice:** clickable country/region/city labels, linked sidebar cards and Back/Forward for the journey below. Keep individual cities selectable and polygon picking as fallback; preserve room for the globe at 640×480.
5. **Hardware evidence:** an authorized Classic Mac run records OS/build, installed fonts and fallbacks, screenshot/readability, hit targets, maximum event-loop turn, placement cost, memory and size, plus cancel/restart, resize/expose, zoom-preview and settled-state checks. No run is authorized by this plan alone.

Refine v0.2 tickets from these findings rather than creating speculative implementation tickets now.

## Navigation acceptance journey (authoritative)

**Experience:** select a country label to inspect it without moving the camera; follow a region link and navigate to Wisconsin; inspect Madison; use Back to restore Wisconsin's entity and camera. Cities, countries and regions remain independently selectable.

**Scope boundary:** a tiny curated administrative catalog, a fixed-capacity navigation prototype, a native-label experiment and one integrated journey. Not in scope: production-pack migration, broad encyclopedia import, a general graph runtime, a font renderer, full search/favorites, an external pack loader, or automatic persistence. v0.1 gates and the milestone structure are unchanged; the target theme is v0.2, with its final gate reviewed after prototypes. An always-embedded base catalog serves v0.2; evictable packs and persistence extend the reference contract in v0.3 under existing owners.

**Acceptance:**

- [ ] Reviewed source-backed United States/Wisconsin/Madison identities and relations; unresolved candidates remain separate from published claims.
- [ ] Additional fixtures cover microstates, a non-US administrative hierarchy, ambiguous settlement names and geographic edge cases.
- [ ] Selection without camera motion; navigation, Back/Forward, forward-branch truncation and bounded capacity; relevant entity and camera state restored.
- [ ] Stable references across reordered/detail editions, unavailable packs, unknown/retired IDs, stale results and changed viewport.
- [ ] Fixed-metric placement tests cover deterministic order, collisions, limb/viewport limits and label-level hit targets; native drawing and hit tests use the same accepted placements.
- [ ] Replay-equal intent/result traces with exact parse/format round trips, including failure and stale-result cases; zero allocations in `update`; an explicit history golden for Wisconsin → select Madison → Back → Forward.
- [ ] Prototype ABI sizes measured against the provisional constraints above.
- [ ] Frame/replay goldens, cancellation, responsive progressive rendering and offline operation preserved. Host baseline: `cmake -S . -B build -G Ninja`, `cmake --build build`, `ctest --test-dir build --output-on-failure`; relevant existing targets include `test_app`, `test_projection`, `test_overlays`, `test_countries`, `test_places`, `test_render`, `test_zoom`. Add dedicated navigation/fixed-metrics targets when implementation exists rather than claiming they exist now.
- [ ] `tools/check-policy.sh` and `tools/probe-retro68.sh` run for the implementation; new headers exercised by the probe on both PowerPC and 68k, and every compile result inspected (the script prints `FAIL` without failing the shell). Report compile-only separately from hardware evidence.
- [ ] Actual Classic Mac hardware journey, fonts, readability, responsiveness, memory and build size recorded with OS/build identity, or hardware acceptance explicitly left pending (needs separate authorization).
- [ ] Findings used to scope narrow follow-ups under existing owners; #53 is not closed and no runtime functionality is claimed from research approval.
