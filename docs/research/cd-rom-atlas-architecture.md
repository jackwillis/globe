# Globe — layered atlas and CD-ROM storage research

**Working product/architecture note — 2026-11-09**

**Status:** Owner discussion synthesized; `accepted` below describes product direction, not a format, milestone, source-license or implementation approval. GitHub Issues are the implementation tracker. Do not use this note to silently change production packs or access the G4.

## Product thesis

Build an offline-first, free-software, software-rendered geographic workstation in the *spirit* of 1990s CD-ROM atlases (especially Microsoft Encarta World Atlas / Virtual Globe), for NetBSD/X11 and native Classic Mac OS 9. We are not reproducing Microsoft's code, art or interface. Modern strengths should include transparent data provenance, semantic geographic QA, reproducible atlas editions, source licensing, and long-term maintainability.

The 467 MHz Power Mac G4 with 256 MiB physical RAM remains a critical performance target. **The Classic Mac application's memory partition and peak decoded-resource allocations are generally tighter constraints than the space on a disc.** A conventional data CD offers roughly 650–700 MB of capacity; actual drive speed, seek behavior, disc format and read-only operation must be tested.

**Product principle:** *Globe adapts to the computer, rather than requiring the user to configure the computer for Globe.*

## Accepted product direction

1. **Self-contained Earth:** The application includes a sufficiently complete global atlas to launch, navigate and identify important geography without an external data file or network connection. What *exactly* belongs in the embedded baseline is an atlas editorial/size tradeoff (#52), not a promise that every high-detail feature is in the executable.
2. **Optional external detail:** Compatible data files add finer coastlines, political boundaries, relief, hydrography, places and later regional datasets. The *same* runtime/renderer supports a standalone executable, a larger copied-to-disk distribution, and a read-only volume / possible future CD-R edition. No Full/Compact startup selector.
3. **Progressive resource policy:** Show useful embedded geography immediately; select external refinement from the view when resources permit; pin active resources, validate candidate data, and publish only at safe generation boundaries. Evict unused decoded resources before lowering map detail when practical. Preserve usable lower-detail fallback on any optional load failure (#26, #56, #57).
4. **Automatic memory UX:** Ordinary users never manage a cache to navigate the globe. Memory budget is automatic; an optional Advanced setting caps *managed atlas resources*, not the whole process or Mac OS 9 Finder partition. Loading should not stall a drag, and routine eviction should not produce repeated dialogs.
5. **Read-only by design:** No required writes beside the application. Settings and any optional writable cache follow #49's preference/storage policy and consent constraints. Missing disc/file, unsupported version, truncated/corrupt chunk or allocation failure must not take down the embedded Earth.
6. **Developer diagnostics separate:** #62 owns the optional Activity Log window; #59 owns measurement schema; #63 explores a lazily initialized Forth/Jim Tcl/Lua console. None is a prerequisite to a working atlas.

**Not yet agreed:** a specific spatial index, target compressed chunk size, one-file versus regional pack layout, compression algorithm, a chosen CD product, physical-media release, installer, total data size, or v0.4 milestone.

## Three logical atlas tiers (not user-visible runtime modes)

| Logical tier | Supplied by | Purpose | Memory principle |
| --- | --- | --- | --- |
| Embedded base Earth | Application executable | Always-useful global low-detail map; fallback | Stays usable even without external media |
| Standard world detail | Optional versioned files | Better global outlines/elevation, important places, rivers and lakes | Selectively load and retain within budget |
| Regional/high detail | Optional indexed data | More detail for the current region and scale | Bounded chunk/LOD cache, never require whole-world residency |

A *distribution edition* describes which files are shipped, not a mandatory memory or display-quality preset. Stable geographic identities and editorial completeness remain separate from geometry LOD or label visibility (ADR 0003).

## Candidate disk access / rendering pipeline

```text
Application (embedded base atlas)
    |
    +----> viewport and render scheduler (Model / Msg / update / Effects)
                    |
             lookup best cached data  <----- bounded decoded-resource cache
                    |                          ^
                    v                          |
             optional detail request --> storage adapter --> pack index
                                            |        |
                                      disk / CD    future cache/source
                                            |
                             bounded read -> validate -> decode
                                            |
                                 safe generation publication
```

- Consider a **small resident index** with file ID, typed layer, geographic/LOD bounds, byte offset/length, decoded-byte limit, checksum/version and stable feature identity references.
- Investigate spatially clustered, independently decodable chunks arranged for fewer optical-media seeks; **index locality must also correspond to physical/sequential layout**. Many small individual files or random reads may be expensive on CD.
- Keep `read -> decode -> publish` bounded and measurable. Compressed bytes, decoded data, scratch buffers and temporarily overlapping old/new resource generations all contribute to peaks. `mmap` cannot be assumed on Classic Mac OS.
- A storage adapter should abstract read-only file access from the shared data/resource policy; however, do not introduce a general-purpose async framework without demonstrating need.
- Candidate scheduling: during movement, reuse embedded/cached data; after settling, prioritize useful visible center/horizon/important features; prefetch neighbors only with budget and media-measurement evidence. **A supposedly asynchronous Classic Mac read can still stall on some device drivers; test actual hardware.**
- Persistent caches, if any, are optional and external to the read-only app volume. If a disc disappears, avoid continual retries and retain the last safe image/data; always keep the embedded Earth usable.

### Hypotheses requiring benchmarks

- Index scheme: geographic grid, quadtree, hybrid index or other spatial/LOD partition. Compare actual hemisphere visibility and seam handling; do not adopt an index from a patent uncritically.
- Container granularity: one indexed archive, regional containers or a small number of typed layer files. Avoid accidental dozens-of-seeks-per-frame behavior.
- Chunk size: begin comparisons across several sizes using representative 50m/10m geometry; **no fixed target selected**.
- Compression: compare decode CPU, scratch/peak RAM, checksum behavior and CD read savings; smaller files are not automatically faster.
- Locality/prefetch: sequential region reads versus random tile reads, start/stop/seek behavior, real drive noise/spin-up, and interruption cost.
- Installation: direct read-only run, copying app+Data to disk, mounted disk image. The same compatible file contract should work across sources.

## Historical engineering: research lead, not a copied design

**Primary lead:** Microsoft patent [US 5,781,195, *Method and system for rendering two-dimensional views of a three-dimensional surface*](https://patents.justia.com/patent/5781195), filed **1996-04-16**, granted **1998-07-14**. It discusses Encarta 96 World Atlas and describes reduced-detail data for interaction, spatially contiguous blocks/sections, sequential CD-ROM placement to limit seeks, cached extended fields of view, successive geographic layers and interruption during view changes. It is unusually relevant to Globe's storage and progressive presentation questions.

**Evidence boundary:** A patent describes claimed methods and an identified preferred embodiment; it is *not* evidence that every Encarta product or version shipped exactly those algorithms. Investigate shipping manuals and independently described systems before making historical claims. The ordinary 20-year US patent term would end in 2016 absent exceptional factors; confirm legal status separately if legal reliance matters. Copyright in Microsoft's code, assets and expressive content is distinct from ideas described in patents.

**Further research candidates:** Encarta Virtual Globe/World Atlas product manuals and contemporary technical accounts; 1990s CD-ROM atlas/GIS storage indexes, geographic multiresolution datasets, optical drive caching and retrieval; comparison with Globe's actual C++ software renderer. Record direct citations, date, exact mechanism and whether each statement is documentary or reconstruction.

## Proposed research gates and acceptance evidence

1. **Measure baseline** (#10/#55/#59): Classic OS 9 app partition, startup free heap/largest allocatable block, framebuffer/QuickDraw/cached geometry peaks; keep G4 bare OS9, Tiger Classic and NetBSD/X11 results labeled separately.
2. **One optional pack end to end** (#48): from ordinary read-only file, strict validation and renderer publication, absent/corrupt/removed fallback, cross-platform output evidence; don't wait for a new general indexed format.
3. **Storage-format comparison** (#31/#52): at least two candidate layouts on representative higher-detail geography, including source/provenance handling, spatial queries, compression, total bytes, peak RAM and safe replacement.
4. **Optical-media benchmark** ([#72](https://github.com/jackwillis/globe/issues/72)): sequential and random I/O, cold/open vs warm cache, read sizes, seeks, compression+decode, worst event-loop stall on real G4 CD and hard-drive/SSD; synthetic host figures are not target proof.
5. **Long-travel stress** (#57): navigate repeatedly across continents at constrained budgets; confirm useful base Earth, no drag-time file I/O, bounded allocations, cache eviction/pinning, no resource lifetime bugs and recovery.
6. **Distribution proof** (#49/#51): same app+Data layout from copied disk and read-only image; verify actual G4 Finder/resource forks, packaging, licences, credits and no writes beside executable. Physical CD remains separately optional.

## Release placement (proposal, not additional GitHub commitments)

- **v0.1 — The Classic Atlas:** standalone embedded Earth, native OS9 acceptance, credible RAM requirements, essential UI and digital-first packaging. No dependence on new external-data formats or CD.
- **v0.2 — The Expressive Atlas:** become genuinely useful with labels/search, discoverable controls, diagnostics and automatic memory discipline; prove a basic external-file loading path and read-only-volume behavior with at least one optional pack.
- **v0.3 — The Expandable Atlas:** stable resource ownership, measured pack/LOD contract, regional/data cache research, safe eviction, reproducible richer editions and a distributable disk-image layout. Optical-media-aware design experiments can occur here, but physical CD is not a gate.
- **Potential v0.4 — The Disc Atlas:** *future release concept only*, contingent on v0.3 evidence. Seek-aware indexed layout, data-rich regional edition, real CD access benchmarks, a copied-to-disk option, disc removal and read-only launch tests; only create a milestone after owner review.

## Existing ownership; avoid duplicate epics

- **#5 + #52:** offline-data runtime and the separate data-production/editorial stream.
- **#31:** versioned compressed pack/index format and G4 data/decode/seek tradeoffs.
- **#72:** historical CD-ROM atlas precedents and measured G4 optical-media retrieval research (v0.3 research only, not a release gate).
- **#48:** Classic Mac File Manager path, first optional external pack and source-dependent I/O behavior.
- **#49 / #51:** read-only preference/cache policy and distributable volume layout.
- **#50:** data contents, attribution and size/RAM/latency costs of richer editions.
- **#26 / #54–#57:** resource lifetime, budget, eviction and safe fallback.
- **#35 / #24 / #36–#39:** useful geography, search, cartographic labels and native presentation.
- **#62 / #63:** optional observability and developer scripting, outside the core-data critical path.

## Non-goals for this research pass

No production format migration, new runtime GIS/SQL dependency, forced networking, raster/terrain-engine rewrite, mandatory installation, huge resident global dataset, background optical-drive churn, automatic scripting interpreter startup, or unrequested release/hardware operation.

**Next planning decision:** after a small pack + real media benchmark, decide whether seek-aware indexed data deserves a dedicated post-v0.3 deliverable. Until then keep physical CD release exploratory.
