# Gemini composable-atlas architecture review — contemporaneous synopsis

**Provenance:** Owner pasted Gemini's response into the ChatGPT architecture discussion on 2026-11-16. This is a **synopsis of the response**, not a verbatim transcript. Unlike the [original Claude review](claude-composable-atlas-review-2026-11-16.md), there was no independent source file attached for archival. Reported mechanisms, historical details and resource estimates were **not independently verified** by this reviewer. See the [Pass 1–3 synthesis](../composable-atlas-bazaar-and-experiments-2026-11-16.md) for the owner's later community/bazaar direction and technical cautions.

## Historical and technical precedents Gemini suggested

- 1990s Microsoft Encarta Virtual Globe and DeLorme Street Atlas / Global Explorer as examples of multiscale geographic browsing and CD-ROM data locality. Exact shipped architectures and RAM claims must be verified against contemporaneous primary evidence; do not infer implementation solely from patents.
- Shapefile, ArcInfo coverage, MBTiles and GeoPackage as GIS/data packaging precedents. These do not themselves solve independent publisher identities or conflicting overlapping representations.
- Debian/rpm for package identities, version constraints, installation and file ownership; Git/Nix/IPFS for immutable/content-hashed storage. **Content hashes are not geographic entity identities.** Hashing is not categorically too expensive on all retro hardware; measure its use at install/check time rather than assuming it runs on the render path.
- Publisher-qualified IDs, copy-on-write overlays, flat spatial indexes, deterministic precedence, and source/provenance metadata.

## Three alternatives proposed by Gemini

1. **Static Stack:** compressed global base plus append-only packs that patch fixed base IDs. Attractive size/simplicity; fragile as base/source rows change.
2. **Namespace-Qualified Package Registry (Gemini's choice):** immutable named packs, small binary indexes/data files, a mount table, compatibility metadata, spatial/scale lookups and deterministic precedence. Can potentially support order independence, but actual costs and geographic continuity are unproven.
3. **Embedded Relational Graph:** runtime SQL/graph reconciliation and fuzzy geographic identity matching. More expressive, but conflicts with the project's runtime-boundedness and editorial authority model.

## Specific proposed behaviors worth testing rather than adopting

- Default regional > national > global precedence; alternative selectable layers when two publishers disagree.
- Explicit reference/alias tables for renamed, split or retired entities.
- Optional dependencies go dormant until installed; a Reader format/version mismatch is explicit.
- Installation order does not affect the resolved atlas.
- A small synthetic overlap benchmark, with current World and Wisconsin mock data, exercises both memory and compatibility.
- User-facing layer browser stays thematic; provenance/dependency details belong in an inspector.
- No runtime polygon-boolean GIS and no full dependency graph solver in the first public format.

## Corrections and limits flagged during our review

- Gemini repeatedly assumed **memory mapping** and **zero heap usage** in the Classic Mac Reader. Classic File Manager reads into owned memory; actual peak and replacement-generation allocations must be measured. A compact index is still plausible.
- A **bounding-box override** does not establish line/ring seam continuity, stable identity or political authority. Source-specific geometry replacement needs explicit validated semantics, probably varying by raster/vector/feature type.
- A deterministic **priority cascade** solves load order, but not correctness or disputed claims; 'newer' or 'more regional' does not imply authoritative.
- The reported "full solution" and 4 MiB/16 MiB performance estimates are hypotheses, not hardware evidence.
- CD, LOD, data-container and package-manager precedents supply *partial patterns*, not a preexisting complete solution to open geographic federation.
- Gemini's previous recommendation did not account for the actual Poodle Earth code and its `gcat:` / ADR 0005 identity research as closely as Claude's repository-specific review.

## Current disposition

Useful for architectural option generation and historical leads. **Not** a specification, approval of exact precedents, performance result, source-license decision or reason to change the runtime. The strongest surviving ideas are independently identified immutable packs, small deterministic resolution, explicit provenance, and deferring automatic geographic reconciliation to modern publishing tools.
