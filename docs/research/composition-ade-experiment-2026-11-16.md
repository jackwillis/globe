# EXP-ADE-01 — Synthetic atlas composition (A versus D versus E)

**Date:** 2026-11-16  
**Status:** Executed, exploratory research; **not** a Reader contract, accepted ADR, production pack format, platform claim, milestone gate or implementation approval.  
**Ownership/context:** #31 (pack conformance), #52 (atlas publishing); [draft synthesis PR #177](https://github.com/jackwillis/globe/pull/177) §14.  
**Execution:** Host-only. No G4, Classic Mac, Retro68, production pack or renderer change. All generated resources are invented in temporary directories.

## Question and methods

Following `causal-analysis` and `representing-and-intervening` (from `jackwillis/claude-plugins`), model the system, preregister predictions, intervene by substituting one composition policy at a time, observe, and revise. **Frame challenge:** a package manager may not be the problem; perhaps independent *explicit selection* and provenance are the only missing functions. This is a **Rung 2 software intervention** over a controlled synthetic fixture, not an observational estimate of contributor adoption or real Classic performance.

**Estimand:** On six fixed synthetic immutable add-on packs and one common fallback, the difference caused by choosing A, D or E in (i) resolved snapshot correctness, (ii) consumer read operations/bytes, (iii) host Python execution time and traced allocations, compared under the same policy. It does not estimate the effect on real geography or OS 9.

**DAG (engineering causal hypothesis):** `mechanism → lookup/validation/parse work → reads, time, allocations`; `pack count/bytes → lookup work, reads, time`; `conflict policy + selected source set → resolved snapshot`; `host filesystem cache + machine load → observed time`. Treatment is deliberately set; input bytes, conflict policy, interpreter and run environment are fixed. Potential interference through the warmed OS page cache, interpreter state and Python GC makes timing descriptive, not a causal statement about G4. Semantic correctness is proved **only for this toy policy**. No adjustment set is needed for the controlled deterministic comparisons; this does not identify an effect on different platforms.

**Alternatives:**
- **A** discover all supplied mock packs at startup and apply a pure deterministic resolver. No persistent index.
- **D** publisher validates and precomposes the same pack set into one sealed mock file; consumer reads only that result.
- **E** publisher/user prepares a flat manifest with explicit file names, lengths and SHA-256 digests; consumer reads and validates selected packs, then runs **the same resolver as A**.

**Fixture:** six invented packs from four distinct publishers, with relief, hydrography and places slots, one overlapping hydrographic slot with contradictory values, and two different publishers using the local identifier `shared`. Embedded fallback covers the overlapping slot. A conflict reports both candidates and retains the embedded claim. **No spatial union, name/coordinate join, natural-world truth, resolution priority or real public format is implemented.** Byte digest is not geographic entity identity. Both D and E can be independently published in principle; D must be rebuilt to change the selected set.

**Predictions written before execution:** (P1) All 720 orderings of six unchanged inputs yield byte-identical resolved snapshots in A/E and sealed bytes in D; (P2) D requires one consumer file read, A six and E seven in this fixture; (P3) E can explicitly select one overlapping pack while leaving both installed, where A always discovers both; (P4) corruption, incompatible versions, duplicate publisher-qualified IDs, absent files and read-only operation are classified without losing the embedded fallback; (P5) these tiny host timings cannot justify a G4 scanning cache or live generations. **Falsification:** any order-dependent snapshot, silent arbitrary contested source selection, involuntary disk writes, or lost embedded fallback rejects the toy design.

## Reproduce (stdlib Python only)

```sh
python3 -m unittest discover -s atlas/research -p 'test_composition_ade.py' -v
python3 atlas/research/composition_ade.py verify
python3 atlas/research/composition_ade.py bench
```

`verify` prints deterministic JSON and checks 720 permutations for A, E and D, plus a read-only source-fingerprint comparison. The unit tests exercise incompatible versions, corrupt/mutated bytes, missing files, conflicting versus byte-identical duplicate pack identities, no invented upgrade precedence, publisher-qualified local IDs, invalid manifests, explicit selection and fallback. No generated pack or manifest is committed. `bench` reports 80 per-design warm Python trials with `perf_counter_ns` and `tracemalloc`; its measurements are intentionally nondeterministic and must not be used as pinned CI thresholds.

## Observed results

**Host:** Linux 6.18.57 x86_64, Python 3.13.5, 2026-11-16. **Version/context:** source introduced in the separate experiment task branch based on GitHub `main 0b5b16d`. Test script is self-contained and does not compile the repository's C++.

| Check | Observation | Interpretation |
| --- | --- | --- |
| Permutations | 720 input orders; exactly **one** A resolved hash, **one** E resolved hash, **one** D sealed hash | P1 held for this fixture; does not prove a complete generic algorithm |
| Snapshot SHA-256 | `8931145948a671722e6d8afd7dc8ecc27ec6a50a364f4a321cd46f564885900e` | Identical A/E/D *logical* snapshot; the sealed file's own digest differs |
| Conflicting hydro slot | One explicit `ambiguous` record; embedded river retained | No silent precedence or spatial union |
| Publisher-qualified identifiers | `gamma.example:shared` and `delta.example:shared` remain distinct | No accidental ID join |
| E selecting five of six | Hydro conflict removed by **explicitly excluding one pack**, while A still sees both | E offers a functional selection benefit beyond automatic scan; D must republish to achieve it |
| Corrupt/missing/incompatible/duplicate packs | Classified in 13 host unit tests; fallback retained | Only toy error semantics tested; E detects well-formed byte tampering via manifest digest; A lacks external digest commitment |
| Read-only operation | Consumers perform no writes; file hashes unchanged, fixture permissions set read-only | No real Classic volume/file APIs tested |

**Warm Python microbenchmark (80 trials/variant, median; illustrative only):**

| Variant | Consumer file reads | Consumer bytes | Median ms | Median traced Python allocation peak |
| --- | ---: | ---: | ---: | ---: |
| A: scan | 6 | 1,134 | 0.5172 | 11,086 bytes |
| D: sealed | 1 | 792 | 0.0535 | 5,596 bytes |
| E: manifest | 7 | 1,865 | 0.7612 | 14,214 bytes |

The Python timings were not cold-cache controlled, CPU-pinned, or replicated on Classic; they measure JSON parsing, hash computation and Python bookkeeping on tiny fixtures. Traced peaks exclude interpreter/runtime and OS memory and are **not** C++/G4 peak data. D deliberately shifts work to publishing; publishing cost is *excluded* from the consumer columns. File-op counts reflect this fixture and implementation, not actual HFS/CD performance. Source-line counts are crude: shared decode/collect/resolve ~72 source lines, A-specific ~3, D publisher+consumer ~14, E manifest producer+consumer ~22. These are localized function counts, **not** total engineering maintenance complexity.

## Observe → update / decision

**P1–P4 held within the synthetic model.** E adds **selection and byte-pinning semantics**, not a performance benefit: this implementation did more work than A because it verifies six SHA-256 digests and parses a manifest. D is smallest consumer-side but E can change optional selection without rebuilding the seal; D *does not imply* a monopoly publisher and can still be independently produced with FOSS tooling. A is sufficient for deterministic discovery but cannot implement a user-chosen active subset without an additional selection mechanism. A's lack of a pinned manifest means a well-formed changed pack can change its result silently between launches (syntactic validity alone is insufficient authenticity).

**Negative conclusions:** This test demonstrates **no need for B** (a persistent cached catalog) or **C** (transactional live generations); it does not establish that they will never be useful. Neither runtime dependency solving nor a general-package-store model was needed. E's extra code and I/O are justified only if independent, explicit add-on selection is desired.

**Recommended narrow follow-up, not authorized by these results:** retain D for ordinary publisher-sealed distributions and explore a *small explicit active-set manifest as optional Reader metadata* only after #48 demonstrates a safe single external raw pack and #31/#52 define approved conflict/identity contracts. Before shipping a public manifest, decide version/lineage policy, cross-pack geographic semantics, byte authentication, missing-resource status and format compatibility. Do not convert the Python mock JSON to a production format.

## Unresolved / Classic-specific evidence

- Bare Mac OS 9.2.2 File Manager startup seeks, HDD/CD/removable-volume behavior, open/readdir cost, file type/resource forks and error classification (#48/#72).
- Native allocation peaks (source bytes + decoded data + scratch + old/new buffers), `FreeMem`/`MaxBlock`, and quiescence of resource-owning jobs (#10/#26/#55–57/#103).
- Actual read-only HFS behavior, atomic rename/replacement, cache-consent semantics (#49); the toy only reads chmod-restricted files under Linux.
- Actual geospatial identity/contested assertions, seam QA, source rights/provenance and native pixel parity (#52/#53).
- Real publisher usability/adoption or external contributor cost; no human study occurred.
- Existing **24 MiB preferred / 12 MiB minimum** Classic partition remains the baseline; 32 MiB preferred was **not evaluated or adopted**.

**Work boundary:** This report is for review, not a commitment to A, D or E. Nothing touched production GATL/GELV/GPOL/GPLC, Reader behavior, milestones, accepted ADRs, new data licenses, owner hardware or supported platforms. A failed result would have been retained; no thresholds were tuned after observing the data.
