<!-- Archival copy of the owner-supplied Claude architecture review, 2026-11-16. Reviewer worked from 1f2dfe3 (2026-11-14), could not inspect issue bodies/comments or hardware. This is evidence and advisory analysis, not accepted architecture or up-to-date implementation truth. See ../composable-atlas-bazaar-and-experiments-2026-11-16.md for subsequent synthesis and corrections. -->

# Poodle Earth — Composable Atlas Editions: Architecture Review

Read-only review of `jackwillis/globe` at commit `1f2dfe3` (2026-11-14, shallow clone). No repository changes were made.

**Evidence labels.** CONFIRMED = read in code or tests. ACCEPTED = stated as owner-approved in a repo document. DRAFT/RESEARCH = proposed or experimental in the repo. REC = my recommendation.

**Limits of this review.** I could not read the GitHub issue bodies or comments: the API is blocked for an unattached repo, and I did not request write-capable credentials for a read-only task. Issue ownership below is taken from `docs/backlog.md`, `docs/research/cd-rom-atlas-architecture.md` and the ADRs. Issue #137 appears in none of the documents I read, so I say nothing about it. No hardware, Classic Mac or G4 behavior was observed; everything about it is cited from the repo's own notes.

## Bottom line

The smallest credible step is four things. Nothing here changes an existing format.

1. Load existing packs from the Classic Mac file system. This is the confirmed gap, owned by #48.
2. Wrap each existing payload, byte-identical, in a small self-describing envelope with a pack ID, version, identity-contract number, layer kind and coverage.
3. Define the resolved atlas as a pure function of the *set* of valid packs. Ties degrade to a lower-detail source and are never broken by install order or recency. Prove this with a permutation-invariance test.
4. Put a read-only storage seam under pack reads, so fault injection and low-memory tests become possible.

Defer the overlay graph, dependency solver, on-device manifest parsing and per-object provenance.

The main risk is upstream of all this. The identity catalog that composition depends on has no reviewed assertions yet (`atlas/catalog/review-decisions.json` has `"decisions": []`), and the Wisconsin and Mississippi examples depend on data and layers that do not yet exist (see G).

## A. Current repository assessment

### Confirmed implementation

- **Strict pack readers.** GATL, GELV, GPOL and GPLC are all version 1 and little-endian, with explicit bounds and CRC. Unknown versions are rejected (`docs/atlas-format.md`, `docs/elevation-format.md`, `docs/country-format.md`, `docs/place-format.md`; `src/data/*.cpp`). Readers take caller-owned `(pointer, size)` views, so they already work on a sub-range of a larger buffer (`include/globe/atlas.hpp` header comment). This is what makes a byte-identical wrapper possible.
- **No pack identity or dependency fields.** Headers carry magic, version, flags, CRC and counts. Only GATL has a provenance text block (at most 4096 bytes). GELV, GPOL and GPLC carry none. Nothing carries a pack ID, data version, dependency, coverage or supersession field.
- **External loading exists only on X11, and only for two flags.** `--atlas FILE` and `--elevation FILE` (`src/platform/x11/main.cpp` \~1049–1061, 1097, 1118–1129) go through `read_pack_file`, which does `fopen`, reads the whole file into a `std::vector`, and rejects anything over a size cap (`src/platform/common/elevation_packs.cpp`). Up to `kElevationLevels = 4` levels are accepted. `tests/test_elevation_packs.cpp` covers missing, corrupt, over-budget and too-many cases.
- **Classic Mac OS has no external-pack path.** `src/platform/os9/main.cpp` \~101–120 parses only `kEmbedded*` arrays. There is no File Manager read code. The SIZE resource asks for 24 MiB preferred and 12 MiB minimum (`docs/os9-proof.md:37`).
- **Elevation packs are order-dependent today.** `ElevationPacks::add` appends in call order, and `for_model` selects `scenes_[level]` by index. As I read `add`, it checks global coverage and equal cell sizes but neither sorts nor checks ascending resolution. Install order therefore already determines level mapping, which violates requirement 2 for the one pack kind that loads externally.
- **Regional elevation packs are rejected.** `GELV` can describe regional extents (`docs/elevation-format.md`), but `ElevationPacks::add` requires whole-globe extents.
- **Elm-style core with precedent for resource registration.** `Msg` includes `RegisterElevation`, `Refine` and `RefinementFinished`. `Model` holds only grid dimensions and a `generation` counter that invalidates in-flight work (`include/globe/app.hpp` \~50–143). Selection is outside the Model: `Effects::pick` is handled by the front ends.
- **Memory accounting.** `include/globe/memory_ledger.hpp` has an `OwnedPack` category and per-category peaks.

### Accepted (owner-stated in repo documents)

- ADR 0003 (`docs/adr/0003-atlas-production-boundary.md`): the two-project boundary (runtime and atlas builder) is accepted in principle, with strict, bounded pack contracts between them and no runtime GIS, SQL or Python. Source policy, formats and edition composition are explicitly *proposals pending experiments*.
- Product direction (`docs/research/cd-rom-atlas-architecture.md`): self-contained embedded Earth, optional external detail, no Full/Compact startup selector, read-only by design, and automatic memory UX. The same note lists as **not yet agreed**: spatial index, chunk size, one-file versus regional layout, compression, installer, and a v0.4 milestone.
- AGENTS.md §5: never write preferences or caches silently (#49); no new data sources or licences without Jack's approval.

### Draft or research only

- ADR 0005 and `docs/design/entity-navigation-contract.md`: a draft `EntityRef` (`uint32`, zero invalid, synthetic range rejected by production builders), flat retired-ID table, effect round-trip navigation and pack-local-index→`EntityRef` side resource. All of it is explicitly unimplemented; numbers are "provisional prototype constraints".
- `atlas/` is Python-stdlib, build-time only, and consumed by no runtime code. It has 20 locked Natural Earth archives (`atlas/sources.lock.json`), an ID allocation registry of 808 source concepts (`atlas/catalog/allocations.json`; `gcat:NNNNNN`, never recycled), a 35-entity fixture, and a review overlay bound to `basis_sha256` (`atlas/research/reviews.py`). `atlas/editions/recipes.json` defines compact, standard and expanded recipes, including `coverage_cell_degrees` of 30, 15 and 15.
- `docs/fetch-design.md` (GL-31) is a design-for-review for a fetch library and pack installation. Nothing is implemented. I read only its header.

### Missing

Pack identity and version metadata; any dependency, coverage or supersession field; a pack scanner or registry; a deterministic resolution rule; the pack-local-index→`EntityRef` mapping; a Classic Mac pack reader; generation pin/unpin; install or recovery logic; any multi-pack test. Also: the reviewed catalog decisions, an admin-1 source (the contract's P1 notes none is locked), and Madison in the places pack. There is also no rivers layer in the runtime (ADR 0003 context).

### Reusable as-is

All four readers, `bytes.hpp` and `crc32.hpp`, the `MemoryLedger`, the generation counter, the `RegisterElevation` registration pattern, the `gcat:` allocation discipline, the review overlay, `atlas/sources.lock.json`, and the build-twice-and-diff reproducibility check described in `atlas/README.md`.

## B. Architectural alternatives

|  | Approach | Strength | Failure modes |
| --- | --- | --- | --- |
| **0 (minimal)** | Drop-in raw packs. Reader scans a folder for known magics. Order is fixed by sorting on resolution or filename. No metadata. | Zero new format. Works today on X11 and needs only a Classic file reader. | No identity, no coverage, no provenance. Overlaps are undefined, upgrades are ambiguous, and filename becomes the identity. Cannot support requirements 4–6. |
| **1 (recommended)** | Existing payloads wrapped in a small self-describing envelope. The resolved atlas is a pure function of the valid pack set. Unit of refinement is *(entity or layer, global coverage cell, scale band)*. A derived index is only a cache. | Keeps formats unchanged. Order-independent by construction. Installs are crash-safe because the valid files *are* the truth. | Needs a fixed global cell grid and scale bands, which are irreversible once published. Needs pack-ID governance without a central registry. Launch cost of scanning on slow media (mitigated by the cache index). |
| **2** | Sealed editions. The builder emits one complete edition per CD. Community additions are separate editions, never merged. | Simplest runtime and the easiest conformance story. Matches the accepted "edition = which files are shipped" idea. | Fails requirement 2 (composable independently installed packs) and the Wisconsin-plus-US case. Every combination is a new build. |
| **3 (strawman)** | General overlay graph with `requires`, `provides`, `supersedes` and an on-device solver. | Maximum expressiveness. | Runtime complexity, memory and test surface far beyond the Classic constraints. Solver behavior becomes de facto truth. No evidence yet that any real case needs it. The brief itself asks to be skeptical of this. |

Approach 1 is Approach 0 plus metadata and a determinism rule. It is the smallest step that makes requirements 2, 4, 5, 6 and 9 achievable. Approach 2 remains the fallback if community composition is dropped.

## C. Recommended minimal architecture (REC)

### Vocabulary: keep these five separate

- **Entity**: a stable geographic identity, `gcat:` for project-owned. Not a polygon, row or name.
- **Source dataset**: a locked upstream archive. Builder-only.
- **Layer**: a typed content kind (coastline, border, places, relief, hydro).
- **Pack**: one distributed file holding one or more layers. It is a container, not a theme.
- **Edition**: a published list of packs. A CD describes an edition; it does not change what a pack is.

### Three states: use the vocabulary, build two objects

The *physical set* (valid pack files in a folder) and the *visible atlas* (what the current `Model` generation plus memory budget can show) are both useful. The *resolved atlas* is just a small derived table: for each (layer, cell, band), which pack and chunk win. Treat it as a pure function that is cheap to recompute. Do not build it as a subsystem with its own lifecycle.

### Identity contract

- Project IDs stay exactly as ADR 0005 and the registry define them. Do not create a competing registry.
- **Community IDs**: add an *authority* concept. The durable reference is `(authority, local)`, where authority 0 is project `gcat` and any other authority is a publisher-chosen namespace string stored in the pack header, with a 32-bit hash used at runtime. The reader rejects a pack whose namespace hashes to the same value as a different loaded namespace. Namespaces are claimed by convention (publisher domain) with no central server. If community IDs are deferred, a bare `uint32` per ADR 0005 is sufficient through stages 1–3.
- Community packs may *mint* only in their own namespace and *reference* `gcat` IDs. They can never redefine a project entity. Adoption of a community entity into the project catalog happens in the project's catalog, through the existing retired/redirect table. Installing a pack never changes canonical identity.
- **Forward references** to an unknown `gcat` ID resolve to ADR 0005's existing statuses (unknown or detail-unavailable) and never fail the load. v1 allows no cross-pack references between community namespaces.

### Pack metadata

A fixed little-endian binary header in the existing house style (magic, version, CRC):

- pack ID (namespace plus name) and a monotonic integer pack version
- layer kind, plus the inner format magic and version of the wrapped payload
- identity-contract number (hard compatibility gate)
- coverage as a set of global grid cells, and a scale band
- payload offset, length and CRC; offset to a provenance block that is never read on the render path
- licence tag

**What a pack references.** Only the identity-contract number is a hard requirement. Add an optional minimum catalog high-water mark as a hint: because `gcat` IDs are never recycled, an older catalog simply yields unknown references. Do **not** pin a base-atlas version; that defeats requirement 3. Do **not** pin specific resource revisions; the one exception is a newer pack declaring lineage over its own earlier versions.

### Resolution rule

For a given (layer, cell, band), consider all valid packs. Select by this total order:

1. finest band that fits the requested detail
2. most specific coverage
3. if still tied, **degrade** to the next-lower representation not involved in the tie, and record the conflict

Never pick by install order, mtime or version recency across *different* packs. Higher version of the *same* pack ID replaces lower, subject to contract match. An explicit user "prefer this source" setting is a later layer of preference above truth, not part of resolution.

### Unit of refinement

Use **(entity or layer feature class, fixed global cell, scale band)**.

- Entity-level replacement forces a pack to ship the whole river, or the base to omit it.
- Segment-level replacement has no stable identity.
- Source-record replacement belongs to provenance, not runtime.
- Cells are the natural chunk and also the natural CD-locality unit. The recipe's `coverage_cell_degrees` is existing evidence that this idea has already been explored.
- Seams are a builder and QA responsibility, tested by comparing border vertices at cell edges.
- Raster layers (elevation, land classes) are a different problem: today they refine by grid level. Do not force them into the entity model.

### Where conflict work happens

- **Atlas production**: all semantic work. This covers ID allocation, entity equivalence, seam stitching, precedence within an edition, worldview and viewpoint policy, and contradictory sourced facts.
- **Install time**: structural validation only. Check magic, bounds, CRC, contract, namespace collision and duplicate (pack ID, version). Report overlaps. Make no geographic judgement.
- **Runtime**: apply the total order above. Nothing mutates.

### Manifest and index

- A human-editable TOML manifest is fine as a **publisher-side input** to the build tool. It should never be parsed on device.
- The pack is self-describing; scanning headers is the truth.
- A generated binary index is an optional **cache**, whose absence or corruption costs only time. This is the simpler alternative to a required index, and it is what makes interrupted installs safe.

### Provenance

One provenance block per pack, kept out of the render path, plus a one- or two-byte source index per chunk. "Which packs contributed to this object" is answered by the resolved table plus those blocks, on demand. No per-vertex or per-feature provenance in v1.

### Layers, packs and UI

Theme (relief, borders, rivers) is the default browsing axis in menus. Pack, source, size and geography belong in an inspector view and are never conflated with themes. Build no UI now.

### Elm-style architecture

No structural change. Follow the existing precedent:

- The platform scans and validates, then sends a `PacksPublished{generation, small summary}` message, analogous to `RegisterElevation`.
- `Model` stores the generation and a small per-layer summary only. The immutable resolved table stays outside the Model, consistent with ADR 0005.
- A loss or corruption event becomes a `PackLost` message. It bumps the existing generation, which cancels in-flight work, and redraws from the embedded Earth.
- Scanning, reading and resolution happen in the platform layer. Nothing in `update` does I/O.

### Install and recovery

- Pack files are immutable and named by pack ID plus version.
- Install = write a temporary name, verify, rename; then rewrite the derived index the same way. Keep prior versions until the new set validates.
- A partial file fails length or CRC and is ignored. No journal is needed because correctness never depends on the index.
- Installation is an explicit user action, consistent with AGENTS.md §5 and #49. Run-from-media needs no write at all.
- Atomic replacement primitives on Classic Mac OS are not verified in this repo and need a hardware check.

## Thought experiments

| # | Case | Expected behavior | Required metadata | Cost |
| --- | --- | --- | --- | --- |
| 1 | Wisconsin then US, and the reverse | Identical resolved table both ways. Wisconsin wins inside its cells by specificity and band, not by order. | Pack ID, coverage, band, contract | Low once the rule exists. Test by permuting all orders. Depends on admin-1 data that is not yet locked. |
| 2 | Two packs overlap on the Mississippi | Different bands: the finer is used when zoom demands it. Same band and cell: a tie, so degrade to the lower-detail source and show the conflict in the inspector. | Band, coverage, entity or feature class | Low. Needs a hydro layer that does not exist in the runtime yet. |
| 3 | Upgrade World Detail, keep Wisconsin | Higher version of the same pack ID replaces the old. Wisconsin's cell keys stay valid. Retired IDs resolve through the flat retired table, and a split or ambiguous retirement shows detail-unavailable. | Pack version, contract, retired-ID table | Medium. Open decision: where the catalog and retired table live (see F). |
| 4 | Remove a CD or pack while running | Reads fail. Pin and keep the last good decoded data. Mark the pack unavailable, do not retry repeatedly, and fall back to the embedded Earth. References report detail-unavailable. | None new | Medium. Needs error classification and the `PackLost` message. Classic volume-removal behavior must be tested on hardware. |
| 5 | Community pack with new IDs referencing canonical ones | Loads under its own namespace. Unknown `gcat` references become unknown, not errors. It cannot override project entities and is labeled as community-sourced. | Namespace, licence tag, provenance | Low to medium. Governance of namespaces is the real cost. |
| 6 | Contradictory sourced facts about one entity | Facts are assertions with viewpoint (the existing catalog schema). The build-time policy picks the displayed primary. A community pack cannot override a project value; it appears as an attributed secondary. | Assertion source, viewpoint, review status (exist in `atlas/catalog`) | Zero now: there is no runtime facts layer. A disputed flag later is small. |
| 7 | Interrupt install halfway | Next launch sees a partial file, which fails length or CRC and is ignored. The previous set is untouched. The stale index is rebuilt or bypassed. | Length and CRC (exist) | Low. Needs fault injection at every write and rename boundary. |
| 8 | Classic launch with too little memory | Load the finest level that fits the budget. On allocation failure, fall back one level at a time down to the embedded Earth. Show at most a minimal "detail limited" note. | Per-chunk decoded-size bound | Medium. Today `add()` refuses over-budget packs outright, which is safe but not graceful. Chunked loading needs the storage seam, which does not exist. |

## D. Incremental implementation path

Each stage is independently testable. Existing owners are from repo documents, not from the issues themselves. All ownership claims are therefore *unverified against issue text*.

1. **Classic file path for existing raw packs** (owner #48, with #49 and #26). No new format. Add a File Manager reader behind a small read-only storage interface, and cover absent, corrupt and removed cases with the same fallback tests X11 already has. This is the simplest useful external pack: the 0.5° `GELV` already works on X11. Fix elevation ordering explicitly so level mapping does not depend on registration order.
2. **Envelope plus deterministic resolved set** (owner #31 for the format; #52/#53 for identity; #26 for lifecycle). Wrap existing payloads. Add the scanner, the pure resolution function and the permutation-invariance test on synthetic packs. Existing readers are untouched.
3. **Identity binding** (owners #52/#53, ADR 0005, entity-navigation contract; #114 for the presentation journey). Emit the pack-local-index→`EntityRef` side resource bound to the exact pack bytes. Embed one reviewed catalog slice. This stage is gated by reviewed geographic decisions (the contract's P1–P3). Decide the authority/namespace question here.
4. **First real refinement and conflicts** (owners #31, #54, #72; #50 for data). One overlap fixture (synthetic first), tie-degrade behavior, cell-seam QA, and an optical-media benchmark to fix cell and chunk size.
5. **Transactional install and provenance inspector** (#49/#51 for layout and read-only policy; #50 for edition data). Write-then-verify-then-rename, retention of prior versions, and the provenance view.

**Likely-missing issues** (could not confirm none exists):

- A pack envelope and resolution-contract issue. It is distinct from #31's compression and seek focus.
- A multi-pack conformance-suite issue.
- Possibly a transactional-install issue, since #49/#51 appear to cover policy and layout rather than recovery.

## E. Compatibility and correctness tests

A stdlib-Python generator (matching `atlas/`'s no-dependency stance) builds synthetic packs from a toy planet with invented entities. Use the reserved synthetic ID range from ADR 0005. No proprietary guest software, no large datasets.

**Fixtures.** Toy World, Toy Region, a nested US/Wisconsin pair, two overlapping river packs at the same and at different bands, a community pack with its own namespace and a forward reference, and a catalog update that retires an ID.

**Invalid variants.** Bit flip, truncation, unknown inner version, contract mismatch, duplicate pack ID and version, namespace hash collision, and CRC-valid but semantically invalid.

**Properties:**

- **P1** Permutation invariance. Every install order yields byte-identical resolved tables.
- **P2** Idempotence. Reinstalling the same pack changes nothing.
- **P3** Monotone degradation. Removing any optional pack leaves a valid atlas that is never worse than embedded-only.
- **P4** Embedded isolation. With no packs, frames match the existing goldens in `tests/golden`.
- **P5** Ties degrade. They never depend on order.
- **P6** Upgrade lineage. Highest compatible version of one pack ID wins; other packs are unaffected.
- **P7** Retired-ID resolution. One bounded lookup; split IDs report ambiguous.
- **P8** Crash safety. A simulated read-write file system fails at every write and rename step; the result is always a valid prior or new set, never a mixed one.
- **P9** Low memory. A failing allocator, hooked through the existing `MemoryLedger`, always falls back without a crash.
- **P10** Seam consistency. Border vertices agree at cell edges to quantization.
- **P11** Reproducible build. Build twice and diff, as `atlas/README.md` already does.
- **P12** Endianness. The Power Mac target is big-endian, but the usual dev host is little-endian. Keep byte-assembled parsing tests and, if available, run them under an open PPC emulator.

Use an open Classic cross-toolchain (Retro68) for compile checks. Host tests are not hardware acceptance (AGENTS.md §5).

## F. Major risks and open decisions

Ranked by irreversibility first, then product importance, then uncertainty.

| # | Decision | Irreversible? | Importance | Uncertainty | When to decide |
| --- | --- | --- | --- | --- | --- |
| 1 | Durable reference width and authority scheme (`uint32` versus authority + local) | Very high: persists in favorites, history and published packs | High | Medium | Before any shipped pack carries entity references (stage 3) |
| 2 | Global cell grid and scale-band definitions | Very high: every pack must align | High | High (the research note lists index and chunk size as unagreed) | After the optical-media benchmark (stage 4) |
| 3 | Pack ID, namespace and version semantics | High | High | Medium | Stage 2 |
| 4 | Where the identity catalog and retired table live (embedded in the Reader versus a catalog pack with monotone-append revisions) | High | High | High | Before stage 3; this is the biggest design hole |
| 5 | Tie rule: degrade versus a declared rank | Medium | Medium | Low | Stage 4 |
| 6 | Envelope wrapper versus per-format v2 headers | Medium: the wrapper preserves reversibility | Medium | Low | Stage 2 |
| 7 | Classic file-system conventions (folder, type codes, resource forks, atomic replace) | Medium | High for Classic | High: needs hardware | Stage 1 and stage 5 |
| 8 | Governance of community licences and authenticity (AGENTS.md §2 requires owner approval for new sources; `docs/fetch-design.md` mentions a possible signed manifest) | Medium | High | High | Before any public community-pack story |
| 9 | Worldview and viewpoint selection exposed at runtime | Low | Medium | High | Not now |

## G. Adversarial review

1. **The identity system the vision depends on does not exist yet.** There is no runtime ID. Reviewed decisions are empty (`atlas/catalog/review-decisions.json`). The places pack is keyed by QID, which ADR 0005 says must not be identity. Building overlay machinery before P1–P3 in the entity-navigation contract risks designing around guesses.
2. **The headline examples outrun the data.** The runtime has no rivers layer (ADR 0003). No admin-1 source is locked, and Madison is not in the 304-city pack (contract, "Prerequisites"). The first refinement that actually exists is raster elevation by grid level, not entity geometry. Vector overlap handling is currently hypothetical. Prove the envelope and determinism rule on elevation first.
3. **Requirement 3 is weaker than it reads.** Every reader rejects unknown versions by design (`docs/atlas-format.md`). A Reader can load new *data* only within the format versions it already knows. A new layer kind or format needs a new Reader. State that boundary explicitly rather than implying open-ended forward compatibility.
4. **Order dependence already exists in code** (`ElevationPacks`, above). Requirement 2 is untrue of the one pack kind that loads externally today, and silent. It needs a test, not just a design.
5. **Raster and vector are different composition problems.** Elevation and land classes have no entity identity. One model for both would be overbuilt.
6. **"Bounded allocations" and `read_pack_file`.** Whole-file reads into a vector contradict chunked loading on a 12–24 MiB partition. The storage adapter the research note calls for does not exist, and chunk-level resolution depends on it.
7. **Read-only versus install.** "Read-only by design" and "installing enriches the user's atlas" are two modes. Run-from-media writes nothing. Install needs a defined user folder under #49's consent rules. Say so up front.
8. **"No central infrastructure" versus `docs/fetch-design.md`.** That design assumes hosted editions and a fetch library, with hosting an owner decision. Decentralized packs need an authenticity answer (signing, or none with an explicit trust label) that nothing yet provides.
9. **Pack-ID governance is itself infrastructure.** Namespace by publisher domain plus content hash is the least central option, but it is still a convention someone must document and police.
10. **Do not build three runtime objects.** Physical set, resolved atlas and visible atlas are useful vocabulary, but the resolved atlas is a derived table and the visible atlas is the existing generation and quality logic.
11. **Scope and sequencing.** The repo's own plan places packs in v0.3 and keeps physical CD as exploratory. v0.1 acceptance (native hardware, memory) is still open. Stage 1 is low-risk and independent. Everything from stage 2 on should not compete with v0.1 or v0.2 gates.
12. **Provenance can grow without bound.** Per-feature provenance would break the Classic memory budget and complicate rendering. Pack-level plus per-chunk source index meets the transparency requirement.

**Answer to the main question.** The least you can build now is: the Classic file path for existing packs, a \~64-byte wrapper envelope with pack ID, version, contract and coverage, a pure resolution function with a permutation-invariance test, and a storage seam for fault and low-memory testing. That preserves a credible path to independently published editions. Every decision that is hard to reverse (reference width, cell grid, catalog location) is deferred to the stage where evidence exists to make it.