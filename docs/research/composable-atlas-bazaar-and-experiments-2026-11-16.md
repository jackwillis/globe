# Composable atlas editions, open publishing, and experimental platforms

**Research synthesis / product-planning proposal — 2026-11-16**  
**Status: non-normative.** This note preserves owner discussion, independent reviews, the Pass 1 decision audit, and the Pass 2 causal research plan. It does **not** accept an ADR, establish a public pack contract, change any issue's owner/priority/milestone, approve a new source/license, authorize hardware access, or add release gates. GitHub Issues remain the live tracker. This note deliberately distinguishes **accepted project principles**, **current implementation**, **owner's emerging product direction**, **provisional hypotheses**, and **open decisions**.

**Source context:** [ADR 0003](../adr/0003-atlas-production-boundary.md) (accepted boundary, formats not accepted), [draft ADR 0005](../adr/0005-entity-identity-and-navigation.md), [CD-ROM atlas research](cd-rom-atlas-architecture.md), [catalog prototype](../../atlas/catalog/README.md), [backlog](../backlog.md), the current issue bodies, and two independent reviews (Claude's read-only repository audit at commit `1f2dfe3`, dated 2026-11-14, supplied by the owner; a Gemini prior-art/design response supplied by the owner). The two reviews are **advisory**, not approval or hardware evidence. Preserved sources: [Claude original review](reviews/claude-composable-atlas-review-2026-11-16.md) and [Gemini synopsis with corrections](reviews/gemini-composable-atlas-synopsis-2026-11-16.md). This note also incorporates the later discussion of Python/GIS publishing, WebAssembly, WebGL, and cathedral versus bazaar. Exact issue and code details should be rechecked against `main` before assigning an implementation task.

## 1. Product thesis: a bazaar of atlases, a small dependable Reader

The aspiration is not merely to reproduce a sealed 1990s CD atlas. It is an **offline, free-software geographic publishing ecosystem** that may have curated official publications *and* independent community editions. A third party should eventually be able to author, validate, distribute, and inspect a compatible pack using documented FOSS tools, without recompiling the Reader or needing a maintainer-operated approval service. This is a **design goal**, not current behavior and not a guarantee that arbitrary datasets can be combined safely.

The quality boundary is intentionally stricter than "everyone's version of truth wins": publishing independence must not imply silent source conflation, undocumented geopolitical decisions, or arbitrary last-installed precedence. Official curation and independent publishing coexist. An installed pack's *presence*, *validity*, *publisher*, *provenance*, *editorial status*, and *selection for display* are different facts.

AI agents are potential accelerants for source processing, documentation, QA and contributor support, **not required infrastructure or geographic authorities**. We should measure whether they reduce work or transfer work to reviewers. Native historical platforms remain first-class; the web can be an additional access and development platform, not a reason to compromise Classic Macintosh resource discipline.

### Proposed vocabulary (not yet a public contract)

| Term | Suggested meaning | Example |
| --- | --- | --- |
| **Reader release** | Version of application/runtime and supported decoders | Poodle Earth 0.2.0 |
| **Data publication** | Dated snapshot/release of geographic data, independently versioned from the app | Atlas `2027.02` |
| **Edition** | Curated human-facing product identity, not a binary serialization unit | Wisconsin Edition |
| **Distribution** | Packaging/delivery of Reader + optional data for platform/media | Classic Mac CD, `.sit`, pkgsrc `.tgz`, web |
| **Pack** | Immutable, versioned, identifiable data artifact | Wisconsin Rivers pack v3 |
| **Layer** | User-viewable type/theme of content | Hydrography, places, elevation |
| **Source dataset** | Identified/pinned upstream evidence with distinct license and publication details | A particular Natural Earth extract |
| **Entity** | Durable geographic identity, independent of geometry and record placement | A river, a city |
| **Representation/assertion** | A particular geometry, fact, name, or relationship, qualified by source/viewpoint/time | One publisher's river path |
| **Catalog** | Identity concordances and/or the small derived index of eligible pack resources; distinguish the two meanings explicitly | An identity authority table vs a resolved-resource table |
| **Manifest** | Authored or compiled description of a publication/pack and compatibility | Text publishing input; compact runtime metadata |

A calendar-style data publication name `YYYY.MM` is appealing but has *no special semantic authority*: later dates do not mean higher geographic truth, compatible data, or automatic precedence. Multiple releases in a month and backwards-compatible reader constraints remain unsettled. The existing `atlas/editions/recipes.json` compact/standard/expanded terms are **research recipes**, not mandatory user modes, product branding, or committed releases.

## 2. What is settled, and what is not

**Accepted/strong direction:**
1. Embedded, useful offline Earth and safe fallback without external media.
2. Builder/runtime separation (accepted ADR 0003): use modern data tools *at build time*, not as Classic runtime dependencies.
3. Stable entity identity is distinct from source record, geometry, name, label, and visibility. The precise production encoding remains draft.
4. Versioned strict, bounded readers; deterministic core behavior; no unmeasured migration to universal GIS/SQL.
5. Provenance, data license review, reproducibility, semantic QA and explicitly documented omissions.
6. Adaptive memory and safe replacement; no drag-time I/O, no assumption of `mmap` on Classic Mac.
7. Independent, community-friendly publishing is an owner *product aspiration*, not an already-approved protocol.
8. Pack availability must not be confused with thematic layer visibility or user memory-quality preferences.

**Leading technical hypotheses, not decisions:**
- Immutable self-describing packs with a small envelope around existing payload bytes rather than rewriting GATL/GELV/GPOL/GPLC immediately.
- A deterministic resolver deriving a small active resource table from the valid pack set; optional, recoverable installation-time index cache, not an authoritative mutable database.
- Publisher-qualified external identities with explicit concordances into existing project `gcat:` IDs, resolved into bounded compact handles; no blind equivalence by name/coordinate/QID.
- Human-readable publishing manifests (TOML or another simple format) compiled into bounded runtime metadata.
- Mostly builder/install-time semantic validation; runtime performs small typed selection/lookup with safe failure statuses.
- Additive packs before arbitrary overlapping-geometry replacements.
- Python library/CLI first for general GIS interchange; browser preview, native GUI, JavaScript/Rust/WASM converter options evaluated later.
- An Emscripten/WebAssembly software Reader as a third frontend and possible future WebGL/native accelerated rendering laboratory.

**High-irreversibility unresolved decisions:**
1. Persistent `EntityRef` representation; project IDs vs community authority/local IDs; redirects, splits, retirement and namespace collision strategy.
2. Location/evolution of the canonical identity catalog: embedded, external versioned catalog, or hybrid; avoid confusing with derived pack mount index.
3. Pack IDs, versions, compatibility (Reader API, payload schema, identity contract, coverage and lineage); unsupported unknown reader-format versions must be rejected safely.
4. Geometry refinement semantics by type: feature, cell, segment, published composite; seam/topology validation; disputed sources and competing claims.
5. Spatial indexing, coverage grid, chunk size, compression, CD locality and peak decoded+scratch+old/new memory: **measure** before standardizing any fixed global cell grid.
6. Community authenticity, redistribution/licensing policy, trust labels, politically contested claims and official editorial governance.
7. Optional installation and crash recovery under Classic filesystem semantics, removal of mounted media, optional cache locations, write consent and direct read-only use.
8. Publisher ergonomics/accessible formats, browser tooling and GPU backend investment: no commitment yet.

### Current implementation boundary (review claims; verify before coding)

- The repo has strict little-endian, versioned GATL/GELV/GPOL/GPLC readers with checksums and bounds; none is a general independently identified community pack.
- X11 imports some external data (notably elevation) via whole-file loading, whereas the Classic Mac frontend still uses embedded data. The initial Classic external-file path is #48.
- `ElevationPacks::add` appends resolution levels in registration order; `for_model` resolves by index. This is a candidate correctness test/fix, **not** evidence that generic multi-pack composition already works.
- Regional GELV coverage may be representable at the binary level but is rejected by the current pack-registration policy.
- The `atlas/` research inventory contains 808 allocated *source concepts*, not 808 adjudicated geographic identities. `atlas/catalog/review-decisions.json` had no approved individual decisions at the reviewed checkpoint. No runtime canonical catalog is integrated.
- The current app has no runtime hydrography layer, so the Mississippi/Wisconsin overlap is initially a synthetic *thought experiment*, not an existing end-to-end feature.
- Existing `MemoryLedger`, generation/invalidation and typed resource patterns provide useful starting seams; whole-file reads and lifetime/low-memory behavior still require evidence.

**Review caution:** Claude's audit was at `1f2dfe3` and could not read issue comments; Gemini provided broad precedents but made unsupported claims about zero-RAM/memory-mapped Classic I/O, deterministic regional override solving geometry, and exact historical implementations. Neither is a production specification. PR #175 was merged later and concerns Classic QuickDraw cursors, not pack semantics.

## 3. Alternative architectures worth keeping visible

| Design | Benefits | Main limitation |
| --- | --- | --- |
| **Sealed full editions** | Strong publisher control; precomputed geometry; tiny runtime | Weak independent composition, large replacement cost |
| **Simple overlay stack** | Easy pack activation and thematic additions | Load-order/priority can silently create incorrect or broken geography |
| **Immutable packs + derived resolved catalog** | Order-independent, auditable source files, separable identity/representation, small Reader possible | Needs explicit conflict, compatibility and lifetime contracts |
| **Materialized installed atlas/index** | Potentially fast startup and access | Rebuild/recovery complexity; read-only media and optional installation |
| **Full dynamic GIS/graph/database resolver** | Expressive runtime joins/queries | Excessive complexity for current scope and Classic constraints |

A promising **hybrid** is immutable packs (authority), small deterministic runtime resolution, and an *optional recoverable* prepared catalog/index. A richer publishing pipeline can build composites and validate seams ahead of time. This is a hypothesis; a pure derived table may or may not be cheap on slow media at high pack counts.

Raster levels, places, lines, boundaries and factual assertions require **different composition semantics**. Do not make a universal source-precedence cascade decide semantic truth. Never use install order, modification time, or newness of unrelated publishers as geographic authority. When representations genuinely conflict, preserve attribution and make unresolved status explicit; "degrade on tie" is only a candidate display policy and needs coverage/continuity tests.

## 4. Publisher journey: interoperability is a product feature

**Long-term contributor goal:** Without writing C++ or using AI, a publisher takes an appropriately licensed dataset, imports/normalizes it, describes its meaning and provenance, validates it, builds a portable pack, previews it, and distributes it.

Illustrative, non-existing CLI:

```sh
poodle-atlas import wisconsin.geojson
poodle-atlas validate wisconsin/
poodle-atlas build wisconsin/
poodle-atlas inspect wisconsin.pack
```

Candidate pipeline:

```text
GeoJSON / other GIS input
  → parsing and CRS/axis normalization
  → declared target feature/layer semantics
  → source, rights, citation, temporal/viewpoint metadata
  → identity allocation + explicit concordance/review queue
  → geometry/topology/semantic QA + machine-readable diagnostics
  → deterministic versioned pack build + compatibility report
  → same-renderer preview on desktop/browser
  → user-controlled publication/distribution
```

**Format input priorities are experiments, not approvals:** GeoJSON is the easiest point/line starter. Shapefile, GeoPackage, GeoTIFF, OSM-family extracts and others may be valuable later, each with dependencies, coordinate/encoding quirks, topology concerns and *individual* source-data licensing. Support for a file format never implies permission to redistribute the contained data.

Prefer a Python API and a small CLI initially because `atlas/` already uses Python for reproducible builder research and because it supports automation, packaging, and AI agents. Benchmark dependency footprint and modern OS usability. Future options: one reusable Rust/C++ core for validation or codec reuse if measured, JS browser editor, WASM compiler, a local GUI. Do not implement duplicate divergent pack validators merely to support several languages.

Separate **syntactic import** (coordinates/fields can be parsed) from **semantic publishing** (correct feature identity, claims, time, source, license, topology). Require explicit unavailable/unknown statuses, not guessed identity matches. Provide actionable diagnostics with stable codes, traceable source rows, negative fixtures and deterministic output. AI suggestions are attributed, reviewable proposals, never silent accepted assertions.

For community adoption, the pathway from *first import to first valid preview* may matter more than a theoretically universal format; observe actual user friction instead of treating pack count or imported feature count as success.

## 5. Browser strategy: access, QA workbench and renderer bootstrap

There are **two separate hypotheses**:
- A WebAssembly browser Reader reduces access friction and permits community pack preview using the same portable engine.
- Developing a WebGL backend in that browser can bootstrap a portable native accelerated renderer.

Recommended *experimental* order:
1. Emscripten compile of the existing platform-neutral C++20 `globe_core`, using embedded atlas and existing deterministic `Model/Msg/update/Effects`.
2. Minimal browser event-loop adaptation; mouse/keyboard/touch where feasible; software-produced framebuffer displayed with Canvas. Compare output/trace parity and code size/memory to native.
3. Optional browser local-file pack load/preview and inspection with controlled resource bounds; do not require cloud hosting or upload a user's GIS data to a server.
4. Distinguish **GPU presentation** (uploading a CPU-rendered framebuffer as a texture) from **actual GPU geographic rendering** (projecting and drawing geographic primitives via shader/buffers).
5. WebGL2 / OpenGL ES-compatible prototype of one primitive; compare geometry processing, scene contract, picking, labels, performance and native portability. WebGL support does not guarantee legacy desktop OpenGL or native GLES compatibility; measure.
6. Only if justified, an interactive browser Atlas Toolkit/editor with the same validators and published pack semantics.

The web frontend is not a portable copy of native QuickDraw/X11 widgets. Keep platform-specific event loops, fonts, windows, browser permissions and presentation in adapters; avoid adding a universal UI framework without measured payoff. An accelerated renderer might consume the same geographic scene and identity semantics as the software renderer, but need not mirror every pixel. Preserve Classic software rendering and native acceptance independently.

Strategic analogy: **NetBSD/X11 was a productive bootstrap for Classic Mac; WebAssembly/WebGL could be a bootstrap for future accelerated platforms.** This is a promising research proposition, not an approved supported-platform matrix or release gate.

## 6. The causal research question and inference limits

**Rung 2 (intervention) question:** For prospective independent atlas publishers, what is the causal effect of a supported CLI/toolkit versus the current bespoke-script workflow on the probability of completing a *semantically reviewed, reproducible, redistribution-compliant* pack within a specified task window?

```text
Contributor skill ─→ toolkit adoption
             └────→ semantic correctness ───→ useful pack
Dataset difficulty ─→ toolkit adoption
              └────→ completion ────────────→ useful pack
Motivation (partly unobserved) ─→ adoption, completion, useful pack
Toolkit adoption ─→ import effort ─→ completion
Toolkit adoption ─→ validation feedback ─→ correctness
Source rights/licensing ─→ publishability
```

**Causal caveat:** Toolkit users self-select. Contributor skill, motivation and dataset difficulty can cause both adoption and outcomes. Counting only submitted packs also conditions on selection influenced by completion and motivation; it can bias comparisons. A graph/backdoor check is valid only under its assumed edges; it doesn't prove the edges. Observational correlations should not be promoted to causal effects. To estimate broader contributor impact, use task assignment or randomization where possible; single-owner/agent pilot results mainly establish feasibility, not community adoption.

For AI assistance later, a **2×2 factorial** is conceptually useful: baseline vs new toolkit × no AI vs AI, using comparable datasets, blind independent reviews where feasible, and measuring authoring *plus review* cost. Avoid asserting precise effects from tiny samples.

**Represent → Predict → Intervene → Observe → Update:** Record a competing explanation and falsifiable prediction *before* changing the system. Failure should prompt structural model revision if the mechanism was wrong, rather than reflexively adding another compatibility setting or resolver rule.

**Graph-reduction distinction:** A transitive reduction of an engineering **dependency** DAG removes redundant prerequisite links. Do *not* transitive-reduce a **causal** DAG merely because there are indirect paths; direct and indirect causal effects are different mechanisms. A derived engineering graph is not experimental evidence.

## 7. Experiment portfolio (proposals, not authorized implementation)

All tests require predeclared baseline, task/data fixture, expected observable result, explicit negative cases and decision gate. The order below is based on **cost and information gain**, not a numerical RICE/ICE score.

| ID | Question/intervention | Predictions and falsifiers | Evidence/decision gate | Existing home |
| --- | --- | --- | --- | --- |
| **A** | Minimal synthetic multi-pack resolver; same pack set in varied orders | Identical active table, safe tie/unknown handling; falsifier: order-dependent or implicit geographic decisions | Permutations, duplicate installs, version upgrades, corrupt pack, removal; choose narrower contract if needed | #31, #52, #26 |
| **B** | Python GeoJSON-to-pack CLI/library against current production readers | Mechanical authoring time decreases without hiding semantic uncertainty; falsifier: review dominates or importer output is not valid/traceable | Source/rights/CRS controls; machine-readable QA; independently reviewed fixture; no new source license | #29, #52/#53 |
| **C** | Synthetic two-publisher identity/concordance fixture | Scoped IDs + explicit links work without runtime reconciliation; falsifier: routine updates force ambiguous unbounded joins | Rename/split/retire, forward refs, unknown, conflicting assertions; decide identity contract *before shipping public refs* | #52/#53, draft ADR 0005 |
| **D** | WASM CPU Reader with embedded atlas, same trace | Core ports with small adapter; falsifier: invasive core rewrites, broken parity, unacceptable web footprint | CMake/Emscripten build, trace/pixel/interaction comparison, code-size/heap metrics | New research issue (if approved) + #114 |
| **E** | Storage and failure/resource benchmarks | Selective loading stays bounded/usable; falsifier: decode/I/O/scratch/old+new peaks exceed target | Host + separate G4 authorization; CD/HDD/SSD seeks, drag stalls, corruption, low memory | #48/#26/#31/#54-59/#72 |
| **F** | Controlled toolkit × AI publishing comparison | Correct reviewed packs per total author+review effort increase; falsifier: reviewer burden rises faster than author productivity | Task/dataset matching, independent semantic reviewer, license check, report selection/confounding limitations | #29/#52, future study |
| **G** | WebGL primitive against CPU and texture presentation | Native-sharable scene semantics and useful speedup; falsifier: incompatibility, costly scene transfer, fragile parity | One coastline primitive, picking/LOD, actual GPU time/resource cost; not a general native OpenGL support claim | New research issue (if approved) + #114 |

**Prioritize informative negative results.** A minimal synthetic fixture can disprove an overbuilt resolver quickly. GeoJSON import can reveal whether meaning and review rather than encoding are the real bottleneck. WASM CPU rendering can test portability without obligating GPU development.

**Outcome measures:** reproducible pack completion; independent geographic and topology errors; unreviewed claim count; import and review time; diagnostic clarity; setup cost for *new* contributors; permutation/upgrade/unknown behavior; peak decoded+scratch+replacement RAM; worst I/O stall; CPU/GPU geometry cost; code/maintenance delta; tested native platform/version.

## 8. Pass 3 product-management mapping: reuse issues, protect milestones

**Planning rule:** Preserve the present milestone and issue ownership, do not create a parallel backlog, and do not upgrade a research hypothesis to `status: ready` or a release gate without the owner's later explicit decision. The existing `docs/backlog.md` and some issue bodies use differing *historic release-theme descriptions* (e.g. #48/#49 vs the newer "v0.3 Memory and Packs" roadmap). Treat live milestone metadata and owner decisions as authoritative; report this drift, do not silently retarget.

### Work packages and decision gates

| Track | Small separately reviewable slice | Output and stop/go condition | Reuse / priority |
| --- | --- | --- | --- |
| **Pack foundation** | Establish order-independent elevation registration + property test | Correct equivalent output for input permutations; don't broaden into generic resolver | #26/#31/#48; engineering correction candidate |
| **Pack foundation** | One optional raw external pack on Classic, strict fallback | Host/Retro68 evidence, later authorized G4 tests; not blocked on new manifest | #48/#49; existing owner scope |
| **Pack foundation** | Synthetic, format-agnostic resolver/conformance harness | Deterministic install order, corrupt/removal/upgrade behavior; may falsify envelope hypothesis | #31/#52/#26; research-only |
| **Publishing** | Reusable Python import/normalization API + CLI with one simple GIS fixture | Valid current-pack output, documented rights and semantic status; stop if meaningful reviews dominate | #29, #53; research/prototype |
| **Publishing** | Two-publisher identity/concordance stress case | Explicit unknown/ambiguous outcome; no accidental joins | #52/#53, draft ADR 0005 |
| **Publishing** | Publisher-facing manifest, diagnostics and docs | First-time "source → valid pack" repeatable without editing Reader | #29/#52; after format evidence |
| **Web** | Embedded WASM CPU Reader prototype | Draw, interaction, trace parity, measured adapter cost | A new **unmilestoned research issue**; #114 for parity |
| **Web** | Browser local-file pack preview | Same validation/selection as CLI/Reader; no mandatory server upload | After pack compatibility prototype, no release commitment |
| **Web/GPU** | One direct WebGL geographic primitive | Native GLES portability hypothesis and performance measurement, may be negative | Research only; no GPU release gate |
| **Ecosystem** | One independent contributor end-to-end demonstration | Import, QA, build, preview and open in native Reader; independent reproducibility | Integrates tracks only *after* component experiments |
| **Ecosystem** | Optional AI assistance experiment | Joint author+review cost, errors and user effort; not output volume | Research only, after baseline toolkit |

### Suggested grouping, without creating new release milestones

1. **Near-term enabling work already owned:** #48 external files; #26 safe resource lifetime; #31 bounded format experiments; #29 source conversion; #52/#53 atlas production and identity. #54–#57, #49, #72 retain safety/measurement ownership. Work on one narrow independently testable slice at a time.
2. **Unmilestoned research queue:** multi-pack conformance hypothesis; importer usability; identity authority stress; WebAssembly CPU browser frontend. The first three may be recorded as scoped updates to existing owning issues; WebAssembly genuinely lacks a dedicated issue.
3. **Future product options, not gates:** public pack publishing protocol, community/official trust model, full browser editor, WebGL/native acceleration, physical CD as a product, cross-platform package matrix, alternative geometric worldviews. Do not attach them to v0.1.x/v0.2/v0.3/v0.4 merely because they are attractive.

**Existing overlap to avoid:** #29 owns source adapters/conversion, #31 the format/index, #52 the atlas publishing epic, #53 geographic QA, #48 Classic external reading, #49/#51 media/preferences/layout, #54–#57 memory, #72 optical research, #122 Unix/pkgsrc packaging research, #165 proposed parser fuzzing/schema pilot, #114 portable presentation semantics, #23 user-facing layers, #63 optional scripting. A separate general "community publishing epic" would duplicate #52 today.

### Candidate milestone gates (not changes to the repository)

- **First external data proof:** one existing pack loads and falls back safely. Do *not* gate on composition, identity federation, browser, or disc image.
- **First multi-pack proof:** permutation-invariant synthetic resources and explicit incompatible/ambiguous statuses. No public format version freeze.
- **First authoring proof:** one documented importer builds one valid previewable pack; establish what remains editorial.
- **First ecosystem proof:** an independent publisher builds a pack and it works in a native Reader and, if proved feasible, browser Reader; only then consider publishing-protocol stability.
- **Future accelerated proof:** GPU path beats relevant CPU baseline while preserving geographic behavior; does not change Classic engine commitments.

### Decision log to revisit after research

| Decision | Evidence needed | Who must approve |
| --- | --- | --- |
| Public identity scheme, catalog placement, and pack compatibility | Synthetic update/split/unknown references and measured memory | Owner; separate accepted ADR/protocol review |
| Pack envelope/index/chunk contract | Parser/conformance suite, G4 resource and media comparisons | Owner; #31 |
| First community-publishable layer scope | Importer and editorial/rights evidence | Owner; #52/#29 |
| Browser as supported platform | WASM prototype, maintenance and parity cost | Owner; platform review |
| WebGL/native GPU backend | Prototype and performance/correctness data | Owner; no current milestone |
| Data publication CalVer and marketing "edition" | Real publication workflow; date and compatibility semantics | Owner; docs before external publishing |
| Official/community source authority and trust | Real conflicting-publisher fixtures, license policy, attribution and safety | Owner; editorial policy |

## 9. Items explicitly preserved as research, not inadvertently approved

- The social value of community editions and curated official publications together; "cathedral" quality control can coexist with "bazaar" participation.
- A *distribution* (CD, ZIP, pkgsrc) is not an *edition*, a *pack*, a *data release*, a *layer*, or a Reader version.
- Reference identities are not merely hashes of bytes; content hashes, publisher/pack IDs, and geographic entity identities solve different problems.
- Geographic knowledge remains source/viewpoint/time qualified. "More local" and "newest" are not proxies for truth. Community disagreement should remain inspectable.
- A user may hide a thematic layer without uninstalling a pack; package inspection can group by publisher, geography, size, source and theme.
- Publisher/build-time workflows can afford Python and GIS libraries; Classic runtime may not. Same contract and conformance suite should span CLI/browser/native readers.
- Support for normal GIS interchange is necessary to make the bazaar accessible; mere byte conversion cannot guarantee correctness, identity, provenance or licensing.
- Browser-local preview should be possible without forcing a server account or network service. WebAssembly may bootstrap both future pack QA UI and a GPU renderer.
- WebGL2-oriented work is a possible OpenGL ES portability route, but Canvas CPU output, WebGL texture presentation, and GPU geographic rendering are distinct accomplishments.
- A fixed cell grid, global priority ladder, universal overlay solver, mandatory installer or hosted registry are **not** accepted foundations.
- A failed experiment is a legitimate decision gate to **remove complexity**, not an instruction to add a generic framework.
- Current GitHub release milestones and accepted ADRs remain unchanged by this research.

## 10. Follow-up and preservation policy

Keep this document as the **research synthesis**, not a shadow issue tracker. Link to it from #52 and relevant ownership issues. Future implementation proposals should include a specific falsifiable hypothesis, tiny fixture, resource/parity constraints, test plan, explicit owner authorization, and the actual issue owning the slice. Revise this note with dated addenda if major assumptions change; do not silently rewrite old experiment results. Preserve the independent reviewers' limitations and distinguish literature precedent from demonstrated Poodle Earth behavior.

**The central experiment:** Can an independent contributor transform a licensed GIS dataset into a semantically validated, reproducible Poodle Earth pack, inspect it in a WebAssembly Reader, and load it on a native historical platform **without** turning the native Reader into a GIS/database engine? The journey is an eventual integration test, not a prerequisite for earlier small experiments.


## 11. Research portfolio housekeeping: separate learning from shipping (2026-11-16 follow-up)

**Owner idea / proposal, not an adopted workflow or label mutation.** Release milestones describe *shippable outcomes and verification gates*. Research tracks describe *uncertainties, learning goals and candidate experiments* that can proceed without changing release scope. A single issue may contribute evidence to a future milestone, but research work should not automatically block that release.

**Why the bazaar matters without external community adoption:** Today's atlas already combines independently sourced geography, different layers, conventions, names, resolutions and provenance. A clean third-party interoperability contract could help **our own internal production** even if no community pack is ever published. The value hypothesis is reduced coupling between source adapters, curated identities, produced packs and the Reader; community use is an additional upside.

### Minimal proposed labeling taxonomy

Preserve existing **P0–P3**, **area:**, **status:**, **needs:**, **idea**, and milestone authority. Don't use labels to redefine release priority or allow an agent to self-authorize work. Existing repository already has a research issue template; reuse it.

- **One type marker:** `kind: research`, distinct from `idea` (an unpromoted suggestion) and `status: ready` (owner-authorized work). An idea may be research in topic but not approved to execute.
- **Three cross-cutting optional tracks:** `track: composition` (identity, format, runtime resources), `track: publishing` (GIS imports, QA, licenses, community workflow), `track: platform-lab` (WASM, WebGL, cross-platform feasibility). Existing `area:` still describes where code belongs.
- **Priority without another P0–P3 series:** Put a single `Research horizon: now | next | later` field **in the research issue body at first**, with *decision enabled*, uncertainty, smallest discriminating test, expected effort, stop condition, and owner. Introduce `research: now` or a Project field only if query/filter pain proves a need. Don't make three overlapping priority systems.
- **Keep stages in issue text:** question → model/alternatives → predicted observation → experiment/evidence → decision. Research findings are evidence, not automatic authorization to implement.

Suggested review rhythm: a **brief optional research-lane review** at release planning or when an experiment finishes, not an additional mandatory release ceremony. Limit simultaneous deep research to one or two independent experiments while milestone issues progress; let a finished result free a slot. An experiment is independently reviewable if it states a bounded question, fixture, prediction, objective observation and decision gate.

**Proposed organization without a second backlog:**

| Track | Existing home(s) | First question to investigate | Horizon proposal |
| --- | --- | --- | --- |
| Composition | #26, #31, #48, #52 | Does permutation-invariant pack composition require anything larger than a small resolver? | Now |
| Publishing | #29, #52/#53 | Can a Python GeoJSON importer produce a traceable validated pack without a C++ edit? | Now |
| Platform lab | #178, #114 | Can existing `globe_core` produce a draggable WebAssembly CPU Reader with minimal native impact? | Next/parallel |
| Storage/media (part of composition) | #49/#54–57/#72 | What happens to read, decode and replacement peaks on real media? | Before container freeze |
| Nix-like publication model (part of composition) | #31/#52, possibly #26 | Can immutable pack sets, activation generations and rollback stay simple/read-only? | Next |

**No new epic tree**: #52 is the existing atlas-production research epic; #166 already covers tracker hygiene automation and #121 the project-state workflow. Only create new research issues where the question has no existing owner and can be tested independently. Always preserve current milestone metadata even when older issue body text conflicts with the current roadmap.

### What to record on each research issue (template proposal)

- **Question / product decision affected:** one sentence.
- **Two competing models:** what would each predict differently?
- **Smallest falsifiable intervention:** controlled fixture, comparison and negative cases.
- **Evidence:** code revision, data/source hashes, OS/CPU, memory/time measurements, test commands and limitations.
- **Decision gate:** adopt / revise / abandon / defer; owner accepts any consequential architecture or scope change.
- **Research horizon:** now / next / later, distinct from P priority and release milestone.
- **Links:** one owning issue, related work and current research documents; avoid duplicate backlog entries.

## 12. Nix / NixOS as a precedent for an open atlas (2026-11-16 follow-up)

**Worth studying; do not adopt Nix as an on-device runtime or publishing requirement.** Distinguish Nix (functional package manager/build/store ideas), Nixpkgs (package collection), and NixOS (declarative OS distribution). The architecture of the *Nix store*, derivations and profiles is more relevant than NixOS configuration modules.

Primary reading:
- [Nix language and derivations — nix.dev](https://nix.dev/tutorials/nix-language)
- [Store derivations — Nix manual](https://nix.dev/manual/nix/2.28/store/derivation/)
- [Store closures and references — Nix manual](https://nix.dev/manual/nix/2.33/glossary.html)
- [Profiles / generations / atomic activation — Nix manual](https://nix.dev/manual/nix/2.31/package-management/profiles)
- [Reproducible-build caveats — NixOS](https://reproducible.nixos.org/)

**Potential transfer patterns (analogies, not protocol requirements):**

| Nix concept | Possible atlas analogy | Limits |
| --- | --- | --- |
| Derivation / precisely declared inputs | Locked source/version/license + builder configuration + QA decisions + toolchain revision produce one edition/pack | Build reproducibility is not geographic semantic correctness; geographic source acquisition can have external terms and ambiguities |
| Immutable store object | Immutable validated pack bytes, with digest separate from logical pack name/version | Do **not** use a byte hash as a durable geographic entity ID |
| Store references and dependency closure | Explicit small set of pack/identity-contract dependencies for a published edition | Full transitive solver unnecessary until real data needs it; on Classic CPU/RAM keep closure calculation at publishing/install time |
| Profile | One selected active pack set / small derived resolved-catalog recipe | Profile is a *selection*, not source truth or a user-visible thematic layer |
| Generation and atomic switch | Candidate pack set validated first, then active catalog switched; prior set retained for rollback | Classic Mac File Manager has different rename/atomicity/symlink guarantees; use appropriate tested filesystem primitives rather than copying Nix mechanics |
| Garbage collection | Removing no-longer-used inactive pack versions | Deletion/retention requires explicit consent; read-only CDs and external volumes cannot be cleaned |
| Reproducible dev shell / pinned Nixpkgs | Optional modern-host build/development environment for atlas publisher | Do not require Nix, Linux, store paths, daemon or Nix language for ordinary Python/CLI/community publishing |

**Crucial separation of three identities:**
1. **Artifact digest** — exact published bytes / integrity.
2. **Publisher + pack ID + version** — distributable lineage and compatibility.
3. **Geographic entity reference** — durable concept surviving changed bytes and publication revisions.

An installable *edition* is closer to a Nix **profile** selecting a set of immutable inputs than to a mutable overwritten database, while a data-publication `YYYY.MM` is a human release name rather than a content hash. A specific distribution may include the Reader, pack files, profiles, attribution and user-consented installer. An index may be regenerated from active packs; installing Wisconsin then USA should yield the same active catalog as the reverse order under identical explicit policies.

**Proposed discriminating experiment (research-only):** Implement a toy `packs/` collection of immutable synthetic artifacts and a declarative active-set file. Evaluate three variants on the host only: (A) scan valid files and resolve on startup, (B) derive and cache a compiled active catalog, (C) prepare new active generations with reversible activation. Test partial write, duplicate version, corrupt manifest/index, missing external volume, removal, upgrade/rollback, and no-writable-storage mode. Measure startup, peak memory, file operations, recovery behavior and implementation/code complexity. **Compare before selecting a generation mechanism.**

**What not to copy:** Nix daemon, requirement for `/nix/store`, Unix symlinks/atomic profile switching, derivation evaluator, transitive repository/dependency solver, mandatory hashed path layout, GC semantics, user-facing Nix syntax, or a hosted binary-cache requirement. The Reader should not know *why* a GIS publication was built, only how to validate and use the resulting bounded resource.

**Research implication:** Nix gives an architectural precedent for immutable inputs and reversible activation, but it does not solve geographic identity equivalence, geopolitical assertions, geometry seam reconciliation or the governance of conflicting publisher claims. Those require their own source-aware decisions. Nix itself also does not automatically guarantee bit-identical outputs: nondeterministic builders still need independent checks.


## 13. Independent review protocol and next-session handoff (2026-11-16)

**Purpose:** Close this long exploratory conversation cleanly and open a focused experiment/research session. No implementation authorization or milestone change is implied.

### Comparative precedents to investigate (beyond Nix)

| Precedent | Testable mechanism relevant to Poodle Earth | Boundary |
| --- | --- | --- |
| **GNU Guix** | Declarative package manifests, profiles, reversible generations; compare whether composition recipes make research reproducible | The Scheme language, daemon and full Guix package model are **not** Classic requirements |
| **OSTree / libostree** | Immutable precomposed filesystem trees, atomic deployment and rollback; a useful **contrast** with Nix's component-level composition | OS filesystem deployment is not a spatial/semantic merge algorithm; copying hardlinks/Unix semantics is inappropriate for Classic |
| **OCI image/artifact descriptors** | Media type, digest, byte length and a manifest referencing independently identified immutable objects | Container images/registry/overlayfs are overkill; do not make historical Reader fetch image layers |
| **The Update Framework (TUF)** | Role/delegation and snapshot consistency ideas for *future optional network updates* and publisher authority | Not relevant to offline-only pack discovery yet; signatures never establish geographic truth |
| **Existing atlas formats (PMTiles, FlatGeobuf, GeoPackage)** | Compare geographic spatial indexing, independent data blocks and GIS interchange against pack access costs | A geospatial file format does not define authority, revisions, or cross-publisher identity |

Grounding references for reviewers (read directly; verify present content before claiming specific support):
- [GNU Guix manual, profiles/generations](https://guix.gnu.org/manual/en/html_node/Invoking-guix-package.html)
- [OSTree introduction](https://ostreedev.github.io/ostree/introduction/)
- [OCI content descriptors](https://specs.opencontainers.org/image-spec/descriptor/)
- [TUF metadata and roles](https://theupdateframework.org/docs/metadata/)
- [Nix profiles](https://nix.dev/manual/nix/2.34/package-management/profiles)

**Scope discipline:** This is a shortlist for focused comparative reading, not a demand to investigate six ecosystems before running a tiny experiment. Prioritize **Nix vs Guix vs OSTree** for immutable composition/activation; OCI is a later metadata comparison; TUF only if optional authenticated distribution becomes real.

### A fourth comparator for the A/B/C experiment

Previously proposed:
- **A:** Scan immutable pack files and derive active table at startup.
- **B:** Cache a derived active catalog/index.
- **C:** Validate/install new immutable pack-set generations with rollback.

Add a control:
- **D:** Precompose a sealed edition entirely at publishing time and ship one manifest plus ready-to-read files, no installer-level composition.

D may outperform the others when distribution is CD/read-only and bundles are curated. A normal installer/activation model is only needed if optional writable pack-set changes actually pay for the extra complexity. If running on an unwritable Classic volume, generations may need to be built elsewhere (modern host, user-owned disk) or not exist at all.

**Initial prediction to preregister:** D/A should cover a large share of current requirements with lowest complexity; B becomes attractive if many-pack discovery is measurably costly; C becomes attractive if users frequently change pack sets and want reversible activation. Explicitly seek evidence **against** those expectations. Measure peak memory, startup time, read/write operations, corrupt/missing file response, install-order invariance, rollback, resource-lifetime safety and implementation LOC/maintainability. Keep the identity catalog and geometric assertions synthetic.

### Protocol for two independent advisory reviewers

**Recommended sequence in a fresh ChatGPT session:**
1. Re-read live **AGENTS.md**, ADR 0003, draft ADR 0005, #26/#29/#31/#48/#52/#72, current PR #177 and this file. Check whether the PR has been merged or changed. Do not assume summaries are current.
2. **Have ChatGPT write two *different*, read-only prompts**, one for Claude and one for Gemini, plus an explicit synthesis rubric. Do not send either reviewer's conclusions to the other before the independent pass. Each gets the same product requirements and A/B/C/D experiment question so results are comparable.
3. **Claude: repository-specific feasibility review.** Verify current code, file/volume behaviors, resource accounting, pack readers and tests; identify the smallest host-only A/B/C/D experiment, exact affected APIs, traps and acceptance/falsification gates. No branch, implementation, GitHub edits, Classic access or source changes.
4. **Gemini: independent systems/prior-art review.** Use primary documentation for Nix, Guix, OSTree, OCI and (only if useful) TUF; distinguish mechanisms and limitations; challenge the need for a package-manager-like layer; propose at least one simpler architecture and an experiment that distinguishes models. Cite direct sources, record unknowns and negative findings; no repository modifications.
5. **Synthesis only after both:** disagreement matrix (claim, exact evidence, conflicting assumption, cheapest discriminating experiment, decision owner), update the issue/research note on a draft branch if needed. Do not seek fake consensus or treat research advice as format approval.
6. If more experiments remain, break them into one-question issues **using existing owners first**; separate planning from authorized implementation.

Possible counter-hypotheses:
- Our small precomposed data **distribution** does not need an installer/activation mechanism at all.
- Stable geographic identity and source semantics dominate complexity; package managers illuminate only byte provenance.
- A simple explicit *pack-set manifest* yields most of the benefits of generations without storing multiple physical installations.
- Read-only media and limited Classic RAM make a host-side catalog compiler more useful than an in-app cache.

### Suggested new-chat opening prompt

> Continue the Poodle Earth composable-atlas research. Repository: https://github.com/jackwillis/globe. Start by reading AGENTS.md, draft research PR #177 (including the archived Claude review and Gemini synopsis), and existing issues #26, #29, #31, #48, #52, #72 and #178. The earlier conversation completed Pass 1 (settled/open decisions), Pass 2 (causal experiments), Pass 3 (issue mapping) and proposed a research-label taxonomy; none of that accepted a new format or milestone. We want to test whether Nix-inspired immutable packs and reversible active atlas configurations are genuinely worth adopting, comparing A: scan-on-startup, B: derived cache, C: validated generations, and D: precomposed sealed edition. Also consider Guix, OSTree, OCI and TUF selectively. First generate two complementary **read-only advisory prompts** for Claude (repository-grounded implementation feasibility) and Gemini (independent systems/prior-art research), with explicit assumptions, source/evidence rules, falsification criteria and a planned synthesis rubric. Do not run experiments, change labels/milestones, implement a store, accept an ADR, or merge PRs unless I authorize those actions. Keep the Classic offline fallback, small memory partition, read-only media and reproducible Python/GIS publishing workflow central.

**Open state at handoff (verify afresh):** draft PR #177 is unmerged; research issue #178 remains exploratory; proposed research labels have **not** been created or applied; owner must decide whether and when to merge research documents, implement experiments or promote research labels. Avoid turning the four presentation/elevator-pitch ideas into new release blockers; website and Infinite Mac demos are parallel product opportunities, not a prerequisite for composition research.


## 14. Independent Nix/Guix/OSTree/OCI reconciliation and experiment preregistration (2026-11-16)

**Non-normative owner-requested research addendum, not an ADR or pack specification.** Two further independent reviews were supplied in conversation by the owner on 2026-11-16: Claude's repository-specific read-only audit at `main 0b5b16d` and PR #177 head `78f3f1c`, and Gemini's systems/prior-art report. This section is a **faithful synthesis of owner-provided reports**, not a verbatim archival copy or fresh hardware measurement. Earlier archived Claude/Gemini review files under `reviews/` document their *previous* reviews; do not confuse these later reports with those files. Both assessments are advisory. Code findings were partly cross-checked against current `main` by ChatGPT; no application code changed.

### Five alternatives (not mutually exclusive)

| ID | Mechanism | Value hypothesis | Main additional cost / uncertainty |
| --- | --- | --- | --- |
| **A** | Stateless discovery of valid packs and deterministic resolution | Drop-in independent packs, no persistent index or writes | Header scans, explicit overlap/compatibility rules, media latency |
| **B** | Derived, disposable cached resource catalog | Avoid repeat scanning for many packs | Staleness, verification, cache lifecycle; could be *publisher-generated and read-only*, not necessarily a Reader write |
| **C** | Validated selected-set generations and rollback | Persisted prior selection and recovery | Requires defining activation semantics; **restart-time manifest selection** is distinct from *live replacement of decoded scenes* |
| **D** | Publisher-composed sealed edition | Tiny Reader, prevalidated assets, natural read-only/CD distribution | Adding an external pack requires producing/selecting another edition |
| **E** | Sealed embedded baseline plus explicit manifest-selected add-ons | Independent publishers and explicit activation without general dependency solving | Manifest integrity/compatibility, missing refs, version/identity conflict handling; not inherently signed |

A sealed **distribution** need not be a monolithic file or single-publisher monopoly. An independent publisher can produce a sealed multi-file edition with FOSS tools. E is not automatically superior to D. Conversely, if A and E use the same selected set and explicit policy, neither should silently choose a disputed geographic representation.

### Claude's repository-specific review (owner-supplied)

- `ElevationPacks::add` admits duplicate same-resolution packs until the resource budget/capacity refuses more. Width-based quality selection makes distinct-width behavior more stable than the previous archived review suggested, **but equal-width ties and capacity rejection can depend on arrival order**. Existing test `tests/test_elevation_packs.cpp` explicitly imports the same fine file twice. This motivates an isolated determinism test, not a new Reader framework.
- X11 accepts `--atlas FILE` for direct external atlas replacement as well as `--elevation FILE` for the elevation collection; Classic OS 9 currently parses embedded data. `read_pack_file` performs whole-file reads to owned bytes; it does not establish streamed/CD access.
- `Model.generation` invalidates rendering work; it is **not a pack-set generation or safe resource-lifetime protocol**. `RefinementJob` can reference `TerrainScene` through a pointer; retaining/cancelling/retiring scenes safely remains #26 work.
- The memory ledger's `OwnedPack` category exists without complete production accounting. The `kTerrainCacheBudget` constant is 32 MiB, while the Classic app's `SIZE` has 24 MiB preferred and 12 MiB minimum; this does **not** demonstrate that today's OS 9 app actually allocates 32 MiB of terrain. Claude's ~14.3 MiB (0.5°) and ~3.85 MiB (1°) scene figures are **derived estimates**, not peak measurements.
- Correction to the reviewer's claim that the partition was never tested: the **bare OS 9.2.2 G4 hardware acceptance** in [#95](https://github.com/jackwillis/globe/issues/95#issuecomment-6840673231) records a memory-workout pass and a 12 MiB partition pass. Exact `FreeMem`, `MaxBlock`, and transient heap peaks remain unknown. The 24/12 MiB shipping baseline stays; 32 MiB preferred remains only a separately measurable proposal, not a scope change.
- Claude recommends **D as default, A or an explicit active-set manifest if useful, B only on measured scanning cost, and no Reader package manager or C-style live activation now**. Defer public ID width, schema, cell grid, source authority, format and Classic filesystem atomicity.

### Gemini's independent comparative review (owner-supplied)

- Recommended a fifth option **E**, sealed embedded baseline plus flat manifest-selected independently published packs, as a way to keep consumer-side selection simple.
- Cited Nix profiles/derivations, Guix manifests/generations, OSTree complete-tree deployments, OCI content descriptors and TUF role/delegation as **mechanism precedents**. Strongest transferable lessons: reproducible declared inputs, immutable bytes, explicit selection, checksummed descriptors, precomposed read-only distributions.
- **Corrections, not implementation evidence:** A does not inherently require graph solving or high RAM; B can use a precompiled read-only index; C need not use Unix symlinks or simultaneous decoded scenes for restart-time selection; D does not prohibit independent publishers; E need not require publisher signatures; runtime composition is not a generic deterministic *spatial union*. Signed bytes do not assert geographic truth.
- The claim that Nix *necessarily* needs a multi-user daemon is too strong (single-user operation exists). The claim that Classic Mac hardware or HFS performance is established by a simulated 2× CD is unsupported. Gemini's high confidence is design opinion, not measured Reader feasibility.

**Primary references supporting *the external mechanisms*, not Poodle Earth performance:**
- [Nix profiles/generations/rollback (Nix manual)](https://nix.dev/manual/nix/2.31/package-management/profiles)
- [GNU Guix profiles and generations (Guix manual)](https://guix.gnu.org/manual/en/html_node/Invoking-guix-package.html) (source link; no independent on-device assessment)
- [OSTree introduction and package-manager comparison](https://ostreedev.github.io/ostree/introduction/)
- [OCI content descriptors: media type, digest and size](https://github.com/opencontainers/image-spec/blob/main/descriptor.md)
- [TUF roles, snapshot consistency and target checks](https://theupdateframework.org/docs/metadata/)

### Reviewer agreement, disagreement and unresolved facts

| Topic | Agreement | Real disagreement / research implication |
| --- | --- | --- |
| Full Nix/Guix/OCI/TUF runtime | Both reject it | The exact subset of metadata/selection mechanics remains open |
| Reader/builder boundary | Complex work belongs with the publisher | E emphasizes independent add-on selection; D emphasizes publisher-composed control |
| Scanning | Requires bounded parsing and explicit conflicts | Claude treats A as plausible; Gemini assumed unmeasured prohibitive cost |
| Manifest | Explicit list can improve reproducibility | Must compare with stateless A, and distinguish editable publishing input from compact Reader contract |
| Rollback | No adoption of Unix profile symlinks | Restart-time selection may supply the useful guarantee without runtime generations |
| Geographic provenance/identity | Package bytes cannot establish geographic truth | Conflicts and publisher-qualified IDs need separate semantics |
| Classic resource budget | No mandatory writable cache, bounded RAM, embedded fallback | Native optical seek latency, peak allocations, activation atomicity and removed-volume semantics remain **unmeasured** |

**Internal value even without community adoption:** immutable source snapshots, editorial decisions and publication recipes allow the project itself to reconcile disparate providers and editions reproducibly. A pack/manifest can keep source rights, identities, representation policy and published bytes inspectable. Community publishers are upside, not the sole value case; published artifact checksums are **not** canonical entity identities.

### Preregistered controlled synthetic experiment (research only)

**Question (Pearl Rung 2 / intervention):** Holding synthetic pack bytes, declared active pack IDs, version support and a single explicit conflict policy constant, how does choosing **A versus D versus E** affect deterministic outcomes, recovery classification, read operations, measured host time/memory and code complexity?

**Represent:** Host Python scripts model six invented, immutable add-on resources, a sealed embedded fallback, publisher-scoped identity, and an explicit ambiguous-overlap policy. The resource set and rule remain constant. **Alternative explanation** for any speed difference: one design reads a precomposed result while another validates individual components; report this as the real mechanism, not a generalized platform speedup.

**Predict before intervention:**
1. On the same six valid add-ons, **all 720 permutations** yield byte-identical resolved output and explicit ambiguity; a violation falsifies the proposed rule.
2. D uses one ready-to-read published object; E uses a manifest plus selected packs, and A discovers candidate packs. D should have fewer consumer read operations; whether that affects G4 startup is **not identified** by host Python.
3. E can exclude a conflicting add-on by explicit selection without deleting it from storage; A with automatic discovery will see both. D can do likewise only by publishing another sealed edition.
4. Missing, corrupt, incompatible and duplicate identities must be classified without hiding a conflict, crashing, writing into a read-only volume or invalidating the fallback.

**Intervene:** Randomize/balance model execution and enumerate input permutations; run checks with a fixed dataset/host/platform and separately report deterministic conformance from noisy microbenchmarks. Outcome selection is predeclared (not chosen from whichever design looks fastest). DAG: design → selection/verification work → file reads and parse time; input size, pack count, filesystem cache and machine load → time; explicit policy and input bytes → semantic outcome. Controlled paired fixtures block cross-design dataset confounding. Host results do not identify Classic optical performance or correctness on real geography.

**Success / stop:** If D and E meet all invariants and E uniquely permits independent *explicit* add-on selection, retain E as a candidate publication/selection model; do not standardize a manifest. If A matches correctness and setup cost with acceptable operations, do not add cache B. If D alone satisfies the intended product scope, a package subsystem is unnecessary. Do not infer C/rollback need without an installation/activation failure case. Retain negative results.

**Owned within existing work:** #31 (synthetic pack conformance), #52 (publication/identity semantics), #26/#48/#72 (later resource, Classic I/O, media checks). No new epic. No data-source/license change. Any executable work requires a separate authorized task branch, isolated host test, independent evidence and review, **not implementation via this documentation PR**.



### Completed A/D/E experiment and independent code review (2026-11-16)

**Evidence and status:** [draft, unmerged PR #179](https://github.com/jackwillis/globe/pull/179) contains the [single experiment report](../../docs/research/composition-ade-experiment-2026-11-16.md) (link will resolve on `main` once the independent experiment PR is merged; [branch report](https://github.com/jackwillis/globe/blob/research/composition-ade-host-20261116/docs/research/composition-ade-experiment-2026-11-16.md) available meanwhile). **This synthesis is not an adopted runtime contract.** `main` was `0b5b16d` during the experiment.

- The controlled synthetic six-pack fixture, using one shared conservative resolver and an embedded base, produced **one identical logical snapshot for all 720 pack-list orders** for A, D and E. An independent expected-output assertion protects against some common-mode resolver errors. An independent review discovered that E's published *manifest bytes* were initially order-dependent: the corrected builder now sorts and produces **one byte-identical manifest across 720 orders**. The original result proved logical invariance only.
- Original optional file-boundary behavior was incomplete: missing discovery folders, missing manifests and missing sealed files could throw. Follow-up host-only corrections now classify those omissions and preserve the embedded-only result. Current corrected revision passes **20 host Python tests** (before any future new design work); no native Mac acceptance is inferred.
- **D — publisher-sealed edition is the simplest baseline** for the Reader. A sealed edition can consist of multiple files and can be authored by independent publishers using reproducible FOSS tools. A small embedded baseline remains mandatory; a publisher's edition is never a required launch mode.
- **E — manifest-selected add-ons** have a distinct **functional** advantage when people need to activate a subset of independently supplied packs without republishing a complete edition. D and E could coexist as *sealed baseline plus optional manifest-selected additions*. A single mock manifest is not a public format or a required OS 9 feature.
- **A — stateless discovery** remains a viable alternative in the toy fixture, not a mandatory mechanism. It offers drop-in installation but must resolve duplicate and contested representations explicitly. E's benefit was **selection and artifact pinning**, not measured speed. D shifted composition work to the publisher; D/A/E Python microbenchmarks are not equivalent whole-system costs.
- **B — persistent derived cache, C — transactional generations, runtime dependency solving, and a Nix-like package manager** address **no demonstrated requirement** in this experiment. A future read-only precomputed index or restart-time saved manifest may be useful if measured needs arise; neither is approved here.
- **Do not conflate three identities:** immutable byte digest, publisher/pack/version identity, and persistent geographic entity reference. Metadata integrity or cryptographic authentication does not validate entity equivalence, polygon/river seam continuity, provenance, rights/attribution, time/viewpoint, or political interpretation. All geographical overlaps were invented mock resources.
- Even without any third-party pack ecosystem, clear producer/Reader boundaries, immutable source snapshots, pinned QA decisions and reproducible publication recipes help **the project itself** reconcile multiple upstream geographic data sources.
- **Next implementation boundary:** [#48](https://github.com/jackwillis/globe/issues/48) should first prove a **single, existing, unmodified optional external pack read on Classic Mac OS** with validation, stable caller-owned bytes, and embedded fallback; do **not** gate it on A/E, a manifest, or changes to GATL/GELV/GPOL/GPLC. Reserve #26 for safe live replacement, #55–57 for memory policy, #72 for measured media layout. The existing 24 MiB preferred / 12 MiB minimum Classic partition remains unchanged; 32 MiB preferred is an unapproved measurement candidate.

**Limits:** Python/Linux synthetic host tests (and GitHub CI on the corresponding PR revision) are not OS 9 execution, HFS read-only acceptance, verified filesystem atomicity, optical timings, native peak memory or real geographic QA. A future agent should treat §13's previously proposed prompts and research program as historical planning, not as current open implementation requirements.
