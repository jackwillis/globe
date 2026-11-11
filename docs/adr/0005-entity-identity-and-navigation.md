# ADR 0005 — Entity identity, concordances and navigation references

**Status:** Draft, 2026-11-10. Owner principles come from the [product brief](../design/product-direction-2026-11-10.md); concrete runtime representation and migration require prototype evidence and joint interface review. No runtime behavior is implemented by this ADR.

## Context

[ADR 0003](0003-atlas-production-boundary.md) separates atlas production from runtime. The existing [research catalog](../../atlas/catalog/README.md) already allocates opaque project IDs, preserves source evidence and requires explicit review. Its 808 proposed source concepts are not an approved canonical political inventory. Existing GPOL/GPLC row indices support current picking but cannot identify a place across editions.

The next coherent experience is United States → Wisconsin → Madison → Back to Wisconsin. Selection should reveal information without moving the camera; explicit navigation should move the view and create history. Names, geometry and optional resource residency must not determine identity.

## Owner-directed principles

- Globe owns stable entity IDs. External identifiers are scoped concordances, never universal identities. Preserve existing allocations; do not renumber them to fit a runtime experiment.
- Entity, source record, name, geometry, attributed assertion, knowledge and presentation remain distinct. Names, coordinates and proximity cannot authorize a join.
- Preserve unresolved/disputed candidates and source viewpoints. Acceptance of a research checkpoint does not accept its geographic assertions. Reuse the existing evidence-bound review overlay.
- A durable reference identifies an entity independently of a pack or memory address. Geometry detail and pack eviction do not change it. Large immutable data stays outside the small mutable Model.
- User intent enters Model / Msg / update / Effects independently of native controls. Preserve portable C++20, bounded operations and offline embedded fallback. No embedded browser, script engine, graph database or runtime GIS prerequisite.

## Proposed reference and resolution semantics

For the prototype, propose `EntityRef = uint32_t`, zero invalid, with the high-bit range reserved for synthetic IDs and rejected by production publication. Persisted numeric mappings must preserve existing `gcat:` allocations; no QID, source ordinal or polygon index becomes a project ID. This is a prototype choice pending format review, not an approved production encoding. A future binary resource should explicitly byte-assemble little-endian fields like the legacy readers, independent of host endianness.

The builder decides real-world entity granularity and reviewed equivalence before publication. Country and map-unit source concepts remain separate unless an explicit evidence-bound decision identifies them; bookkeeping allocations are not approved joins. Published active records use canonical references. The builder resolves approved redirect chains to terminal active IDs, rejects cycles/dangling replacements, and emits a flat sorted retired-ID table with status and either one canonical successor or an explicit ambiguous/split marker. No arbitrary successor is chosen for a split. Runtime performs one bounded binary search and at most one direct canonical lookup, never redirect-chain traversal. Builder input can preserve historical chains; published output cannot contain them. Retirement/publication are new prototype work, not existing validator features.

Never reuse a retired ID. Source updates require explicit reconciliation rather than row-ordinal carryover. Resolvers distinguish available, detail-unavailable, unknown and retired/ambiguous references. Absence of an optional pack is not proof an entity does not exist. In v0.2 the fixture base catalog stays embedded; v0.3 extends the same identity contract to evictable resources under #26/#54/#48. Cross-edition persistence needs a version/compatibility stamp and retired-ID handling; no future binary format is promised unchanged before validation.

No resolver result crosses a message/update boundary by borrowed reference. Results carry bounded owned value fields; native card text is copied into UI-owned storage before releasing any catalog view. Pack-local views are separate from durable identity and are valid only while the owning generation is retained. Proposed v0.3 pin/unpin semantics belong to #26/#57 and are not implemented today.

An immutable edition snapshot publishes a reviewed subset of relationships with explicit viewpoint/time policy. Administrative parentage is not necessarily a universal tree. Keep multiple or contested relationships representable; the tiny UI fixture may expose one explicitly curated path without asserting global uniqueness. Reject malformed references and bound traversal even when the research validator permits global cycles.

## Proposed navigation semantics

Today selection is outside `Model`: `Effects::pick` is handled by the front ends, `Overlays::focused_` holds country focus, and cards contain borrowed string views. The slice explicitly proposes migrating semantic selection and bounded history into Model, while leaving native card presentation outside it. This extends the existing message architecture rather than assuming it already exists; it does not imply removing all platform card code.

Picking emits `SelectEntity(ref)`; update changes the selected identity and requests bounded summary resolution without moving the camera or adding history. Navigation follows an effect round trip even for a synchronous embedded resolver:

- `NavigateRequested{ref, intent}` records a pending navigation token and returns `Effects.resolve{ref, intent, token}`.
- The effect handler resolves against the catalog and emits `EntityResolved{token, ref, status, owned summary, anchor, framing}`.
- update commits selection, camera and history together only for a successful result matching the current token, requested reference and request purpose. Failure preserves the current view/history and produces a bounded diagnostic. New intent or Back/Forward invalidates pending work; stale completions are ignored.

Selection-summary requests use the same explicit correlation discipline and cannot move the camera. Resolver residency, I/O and lookup never occur inside update. Extend trace parsing/formatting to include every new intent and resolved-result field with exact numeric round trips, including failures, so replay depends solely on Model and recorded Msg values. The review's abbreviated `resolved TOKEN REF STATUS LAT LON FRAME` is illustrative, not a sufficient final grammar. Token width, wrap handling and maximum in-flight work need tests before asynchronous loading; initially allow one pending navigation.

Back/Forward restore stored selection and camera together without recomputing old views from changed anchors. Immediately before a successful push or valid history traversal, overwrite `entry[cursor]` with the live selection/camera snapshot. Selection alone creates no entry; boundary Back/Forward is a no-op. Failed navigation does not alter history. After Back, a new navigation truncates the forward branch. Duplicate suppression requires equal entity identity, reconstructed cameras satisfying existing `same_orientation`, and normalized zoom within a named prototype tolerance (propose 1e-9); pin that tolerance in tests. A fixed 32-entry array drops the oldest entry predictably when full.

Store center latitude/longitude, heading and `radius_px / min(viewport_width, viewport_height)` in a compact camera snapshot, not `Camera` or the existing viewport-bearing `CameraParams` verbatim. Reconstruct CameraParams for the current viewport, clamp through existing camera limits, and invalidate obsolete render work via generation semantics. Round-trip arbitrary trackball orientation, including poles, in tests before accepting this representation; precision and ABI size require evidence.

If a history entity is unavailable, retain its ID, restore its camera and show unavailable detail. Back never silently skips entries or substitutes a nearby place. Test Wisconsin → select Madison without navigating → Back → Forward, including the selection overwritten into the departing entry. Favorites may eventually reuse the reference/snapshot contract, with disk persistence separately scoped under #49.

## Contracts and validation gates

The [shared interface proposal](../design/entity-navigation-contract.md) defines the minimum seam and prototype matrix. Review it between Codex (catalog, portable core, tests) and Claude Code (native UI, text adapters, interaction) before adding shared headers or CMake targets.

Prototype in isolation, then integrate only the tiny journey. Test identity across reordered/alternate-detail editions, unresolved joins, removed resources, retired/split IDs, deterministic replay, history branching/capacity, viewport changes and cancellation. Test fixed-metric label placement separately from native glyph pixels. Native Classic Mac acceptance and latency/memory evidence require a separately authorized G4 run.

## Consequences and migration

Continue supporting GPOL/GPLC. A reviewed builder-emitted `(pack-local index → EntityRef)` mapping must be bound to exact pack bytes/version. This requires a versioned side resource or a pack-version change, with #26/#31 review; it is not free compatibility. Never infer that mapping from names at runtime or reinterpret GPLC QIDs (including its QID ordering invariant) as project identity. No production pack replacement or new decoder follows automatically from this draft.

Versioned, typed, bounded binary resources remain preferred; Production ID encoding, schema, pack dependencies, index layout and resource budgets await evidence under #52/#53, #26/#31, #54 and #48/#49. Knowledge may begin with curated facts and article examples retaining qualifiers, sources and dates. Source licensing and update suitability must be evaluated individually; this ADR approves no new acquisition or source policy.

v0.1 acceptance and packaging remain unchanged. The integrated navigation slice informs v0.2 scope; full search, favorites, articles and external-resource support are not automatic gates.
