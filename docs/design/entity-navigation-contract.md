# Minimum entity/navigation interfaces — proposal

**Status:** Design-only, 2026-11-10. Implements no API. Read [ADR 0005](../adr/0005-entity-identity-and-navigation.md) for reference semantics and the [owner brief](product-direction-2026-11-10.md) for product intent.

## Minimum prototype seam for review

Five proposed concepts suffice: `EntityRef`, owned `Resolution`/`EntitySummary`, `NavHistory<32>`, camera snapshot and `PlacedLabel`. Resolution carries status, correlation token, identity, optional anchor/framing and bounded display text. Keep relationship traversal inside the curated catalog resolver for the tiny path. Defer a general Relationship/Knowledge API; facts/articles remain product direction, not first-header prerequisites.

Codex owns catalog publication, value contracts, portable transitions and tests; Claude owns native controls, cards and text adapters. Both review shared types before header/CMake changes. SelectEntity, NavigateRequested, EntityResolved, Back and Forward extend Model/Msg; [ADR 0005](../adr/0005-entity-identity-and-navigation.md) owns their effect/replay/history rules.

Propose `uint32_t` identity, 0 invalid and high-bit IDs for synthetic-only fixtures, rejected by production builders. Preserve existing allocations and reviewed publication mappings; never use QIDs as identity. These numerical choices remain prototype proposals pending format review.

## Owned results and resource lifetime

No string_view, raw pointer or pack-local handle crosses update in a resolver result. Propose a trivially copyable summary with ref, kind/status, optional quantized anchor and `char name[96]` plus explicit byte length. Measure actual `sizeof`/padding and validate coordinate range and missing-anchor status; do not assume an approximate 112-byte layout is portable. The 95-byte legacy name limit motivates a prototype bound, not a universal name policy. Oversized names require explicit failure or reviewed fallback, never silent corruption.

The sidebar owns bounded copies of the displayed title/body. Present-day `InfoCard` borrows embedded bytes; optional resource support must replace or wrap that lifetime assumption before eviction is enabled. For v0.3, propose generation pin/unpin under #26/#57: obtain a view only after successful pinning, copy needed values, then unpin within the event-loop turn. No retained borrowed card after unpin. A missing card resource leaves selected identity intact.

## Provisional memory ceilings

Use the Classic application's 12 MiB minimum / 24 MiB preferred partition, not installed machine RAM (see [memory research](../research/classic-mac-memory.md)). Targets to validate, not measured claims:

- History: fixed 32 entries, at most 2 KiB including metadata; geographic snapshot fields instead of Camera's basis vectors/viewport.
- Placement: at most 128 candidates and 64 accepted labels in preallocated flat arrays.
- Total new resident runtime storage outside catalog packs: at most 16 KiB, including Model additions, pending results, placement/metrics/hysteresis storage and UI-owned text buffers. Record actual ABI sizes and high-water scratch usage; if the design exceeds this, revise bounds before integration.
- No allocation in update or per-frame placement. Catalog bytes, load/decode peaks, native font caches and native drawing scratch are reported separately in the memory ledger, never hidden by this ceiling.

## Native text and portable placement seam

[Existing text-layout stages](text-layout.md) remain the baseline. For this slice propose the following explicit refinement, pending Claude's review: TextMetrics keeps width/ascent/descent/leading/has_glyphs and adds conservative whole-label ink bounds (or conservatively expanded existing metrics when native ink bounds are unavailable). Cache key includes font, size, style, actual fallback/encoding result and `metrics_id`. This proposal does not silently change an implemented interface: text-layout.md is itself design-only.

Measure and draw the same reviewed platform display string, including the Mac Roman fallback, never measure the UTF-8 source and draw a different string. Preserve full-fidelity source names in the builder; source/display policy remains #84/#91. Missing glyphs drop the label while leaving valid city-point/polygon selection available. GetFNum lookup/fallback follows [typography.md](typography.md).

Proposed PlacedLabel fields: EntityRef, candidate ID, draw origin, conservative whole-label ink rectangle, hit rectangle, face and priority. Text comes from the owned/retained placement text storage, not an evictable pointer. Pad hit targets by at least 3 pixels where viewport/limb permit; clip interaction to both viewport and visible globe disk (a rectangle alone cannot represent a curved limb). Draw lower-priority labels first so the hit-test order `(priority descending, candidate ID ascending)` matches visual stacking. A miss falls through to visible city-point picking, then polygon picking. The same accepted records drive drawing and picking; reject stale view/resource/metrics generations.

Use a FixedMetrics layout golden. Test alternate anchors, long names, missing glyphs, clipping, stable ranking/caps and overlapping padded hit targets. First prototype recomputes placements deterministically without hysteresis. If later needed, pass bounded prior placement state explicitly through replayable messages/Model; no hidden temporal state in the placement engine. Native metrics inputs must be recorded or injected as fixed inputs for any replay that includes placements; only semantic navigation replay is platform-independent by default.

Compare per-glyph bounds and native occupancy masks only as later measured experiments, accounting for kerning/overhang. Provisional measurement-work budget: 2 ms per native event-loop turn (10% of the earlier 20 ms bare-OS9 observation, not a proven allowance). If a screenful exceeds it, cache measurement per settle/font change and process bounded batches; measuring once per settle alone must not create one long blocking turn. Preserve authoritative framebuffer/replay goldens. Rotated/curved text and glyph transformations remain research.

## Small experiments and independent progress

1. **Catalog fixture (Codex):** extend existing research mechanisms with a tiny sourced administrative path only after source/claim review. Include a microstate without useful coarse geometry, a non-US hierarchy, ambiguous settlement names, an island/antimeridian case and unresolved parentage. Record exact provenance; keep fixture ID allocation separate from synthetic IDs. Current catalog kinds and sources do not yet prove this path.
Steps 2 and 3 can proceed with synthetic-only identities while source/editorial prerequisites for step 1 remain pending. They do not approve geography or production selection.

2. **Synthetic navigation (Codex, UI review by Claude):** use invented test-only references and fixed resolver responses to exercise selection versus navigation, history restoration/branching/capacity, failed/stale resolution, resource loss and viewport changes. No production pack or UI integration is needed for this test.
3. **Native metrics and placement (Claude adapter, Codex placement):** establish measured-label contracts, fixed-metric layout tests and one native label. Compare readability, collision rate, CPU, memory and build size. Host figures are not G4 evidence.
4. **Integrated slice (joint):** clickable country/region/city labels, linked sidebar cards and Back/Forward for United States → Wisconsin → Madison → Back to Wisconsin. Keep individual cities selectable and polygon picking as fallback; preserve room for the globe at 640×480.
5. **Hardware evidence:** authorized Classic Mac run records OS/build, installed fonts/fallbacks, screenshot/readability, hit targets, maximum event-loop turn, placement cost, memory and size. Include cancel/restart, resize/expose, zoom-preview and settled-state checks. No run is authorized by this plan alone.

Before implementation, review the provisional ID/snapshot types, owned result bounds, publication mapping, target framing and budgets together. Keep native UI work with Claude; do not start simultaneous shared-header edits. Refine v0.2 tickets from these findings rather than creating speculative implementation tickets now.
