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

`verify` prints deterministic JSON and checks 720 permutations for A, E and D, plus a read-only source-fingerprint comparison. The unit tests exercise incompatible versions, corrupt/mutated bytes, missing files, conflicting versus byte-identical duplicate pack identities, no invented upgrade precedence, publisher-qualified local IDs, invalid manifests, explicit selection and fallback. No generated pack or manifest is committed. `bench` now reports 84 per-design warm Python trials balanced across all six A/D/E execution orders, with `perf_counter_ns` and `tracemalloc`; its measurements are intentionally nondeterministic and must not be used as pinned CI thresholds.

## Observed results

**Host:** Linux 6.18.57 x86_64, Python 3.13.5, 2026-11-16. **Version/context:** remote experiment task branch was created directly from GitHub `main 0b5b16d`; an isolated local fixture-only Git worktree was used because direct `git clone` was unavailable in the runner (DNS blocked). It was **not** a complete local `origin/main` checkout. Published code/test/report blob hashes were checked byte-for-byte against the tested local files. Test script is self-contained and does not compile the repository's C++.

| Check | Observation | Interpretation |
| --- | --- | --- |
| Permutations | 720 input orders; exactly **one** A resolved hash, **one** E resolved hash, **one** D sealed hash | P1 held for this fixture; does not prove a complete generic algorithm |
| Snapshot SHA-256 | `8931145948a671722e6d8afd7dc8ecc27ec6a50a364f4a321cd46f564885900e` | Identical A/E/D *logical* snapshot; the sealed file's own digest differs |
| Conflicting hydro slot | One explicit `ambiguous` record; embedded river retained | No silent precedence or spatial union |
| Publisher-qualified identifiers | `gamma.example:shared` and `delta.example:shared` remain distinct | No accidental ID join |
| E selecting five of six | Hydro conflict removed by **explicitly excluding one pack**, while A still sees both | E offers a functional selection benefit beyond automatic scan; D must republish to achieve it |
| Corrupt/missing/incompatible/duplicate packs | Classified in 16 host unit tests **at this historical revision** (20 in the corrected review revision below); fallback retained | Only toy error semantics tested; E detects well-formed byte tampering via manifest digest; A lacks external digest commitment |
| Read-only operation | Consumers perform no writes; file hashes unchanged, fixture permissions set read-only | No real Classic volume/file APIs tested |

**Initial warm Python microbenchmark, before a sealed-file integrity control was added (80 trials/variant, median; retained as historical evidence, not the current executable version):**

| Variant | Consumer file reads | Consumer bytes | Median ms | Median traced Python allocation peak |
| --- | ---: | ---: | ---: | ---: |
| A: scan | 6 (+1 directory enumeration) | 1,134 | 0.5638 | 14,008 bytes |
| D: sealed | 1 | 792 | 0.0517 | 5,596 bytes |
| E: manifest | 7 | 1,865 | 0.6293 | 14,214 bytes |

**Integrity-control follow-up, same day (current executable revision):** A reviewer could otherwise mistake D's very fast, shape-only read for equal validation. The D publisher now records SHA-256 of the resolved atlas and the D consumer checks it; a 15th test mutates valid sealed JSON without updating that checksum and verifies safe fallback. These are **internal integrity checks, not publisher authentication**: an attacker with write access could change bytes *and* the untrusted checksum. The earlier D result must not be used as an equivalent-integrity comparison. A separate run of 80 warm trials on this revision gave:

| Variant | Consumer file reads | Consumer bytes | Median ms | Median traced Python allocation peak |
| --- | ---: | ---: | ---: | ---: |
| A: scan | 6 (+1 directory enumeration) | 1,134 | 0.9289 | 14,008 bytes |
| D: sealed + SHA-256 | 1 | 874 | 0.1725 | 5,678 bytes |
| E: manifest | 7 | 1,865 | 1.0782 | 14,214 bytes |

The shifted A/E medians across host runs (A 0.5638→0.9289 ms, E 0.6293→1.0782 ms) demonstrate sensitivity to shared host load/cache and instrumentation. **Do not infer a stable speed ratio, nor infer that E must be slower than A.** The count of reads and bytes is deterministic for this fixed fixture, but not representative of optical CD/HFS costs. Source-line counts on the current functions are: shared parsing/collect/resolution ~72, A ~6, D including publisher and integrity check ~20, E including publisher ~22. None is an estimate of total lifecycle maintenance cost.

**Balanced-order follow-up, final current measurement:** the preceding two 80-trial batches ran A, then D, then E, so temporal load and cache drift could confound timing. The current benchmark uses 84 blocks, cycling all six A/D/E execution orders exactly 14 times, with one trial per design in each block. A 16th test checks its per-variant counts and file operations. This run produced:

| Variant | Consumer file reads | Consumer bytes | Median ms (84 balanced trials) | Median traced Python allocation peak |
| --- | ---: | ---: | ---: | ---: |
| A: scan | 6 (+1 directory enumeration) | 1,134 | 0.6305 | 14,080 bytes |
| D: sealed + SHA-256 | 1 | 874 | 0.1502 | 5,678 bytes |
| E: manifest | 7 | 1,865 | 0.6259 | 14,214 bytes |

The A/E medians are essentially indistinguishable at this scale, and no statistical performance claim is made. The current executable reports balanced-order results; the two earlier tables remain for transparency as explicitly superseded diagnostic runs.

The Python timings were not cold-cache controlled, CPU-pinned, or replicated on Classic; they measure JSON parsing, hash computation and Python bookkeeping on tiny fixtures. Traced peaks exclude interpreter/runtime and OS memory and are **not** C++/G4 peak data. D deliberately shifts work to publishing; publishing cost is *excluded* from the consumer columns. File-op counts reflect this fixture and implementation, not actual HFS/CD performance; A additionally enumerates the directory once. Earlier routine source-line totals (D ~14 before its checksum verifier) are superseded by the updated figures above.

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


## Independent code review, corrected revision (2026-11-16)

**Attribution:** ChatGPT independently reviewed the actual experiment code and CI on PR #179 at \`42c57c1e\`; the corrections below are new research-only commits on the same task branch, not a change to the preregistered question or a production format. The earlier 16-test, 84-trial results and corresponding measurements above are historical evidence for the *pre-review* revision.

**What the 720 permutations really establish:** Each permutation injects a differently ordered list of the **same six valid mock files**. A and E both call the same \`collect/resolve\` function; D calls it on the publisher side and emits a seal. Exactly one logical snapshot per design verifies **permutation-invariance conditional on this shared policy**. It does *not* independently establish semantic truth, real filesystem install-order behavior, a general composition algorithm, or correctness of all possible inputs. One added **independent fixture oracle** now asserts the expected five visible slots and the exact two attributed conflicting river candidates, rather than relying only on cross-design agreement.

**Material defects found and reproduced before correction:**
1. \`make_manifest(reversed(paths))\` produced different bytes from \`make_manifest(paths)\`. Logical E output was invariant, but its published manifest was **not reproducible from an unordered pack set**. The publisher now sorts the flat file list and reads each file once when forming the size/digest pair. All 720 publisher input permutations now produce exactly **one manifest byte digest**, in addition to the one A/E/D logical snapshot.
2. A missing optional scan folder raised \`TypeError\`; missing manifest/sealed files raised \`FileNotFoundError\` at the consumer read boundary. A missing external source must not invalidate the useful embedded fallback. A now classifies an absent optional directory, and D/E have minimal file-entry-point wrappers that classify missing/read failures and return the embedded result.
3. The previous oracle mostly compared designs that called one shared resolver. The new independent fixture expectation makes a common-mode mistake more observable. Malformed arbitrary geospatial data and publisher semantics remain out of scope.

**Retest — current corrected branch:** Linux 6.18.57 x86_64, Python 3.13.5; source/test GitHub blob digests verified against the isolated local review worktree. Commands and outcomes:

\`\`\`sh
python3 -B -m unittest discover -s atlas/research -p test_composition_ade.py -q
# 20 tests: OK
python3 -B atlas/research/composition_ade.py verify
# 720 permutations, one A/D/E output hash, one manifest hash, fallback unchanged
python3 -O -B atlas/research/composition_ade.py verify
# same results: the primary permutation check is not disabled by Python optimization
python3 -B atlas/research/composition_ade.py bench
# 84 balanced warm host trials; A 0.5591 ms, D 0.1479 ms, E 0.5697 ms
\`\`\`

In the corrected run, file read counts remain A six plus directory enumeration; D one; E seven. Median Python-traced peaks were A 14,297 B, D 5,678 B, E 14,214 B. These are **illustrative warm host measurements only**: a fixture-specific design advantage in operations is not a G4 startup result, and C++ allocation, HFS seek costs, and native screen/partition budgets were not measured.


**Validation asymmetry and CI qualification:** All three designs use the same toy conflict policy, but **not the same consumer validation work**. A structurally validates discovered packs without an externally pinned digest; E repeats those checks and verifies sizes/digests against a supplied (not authenticated) manifest; D structurally validates at *publication* and checks a self-contained sealed checksum on consumption. Importantly, D's publisher does **not** certify geographic truth or discard a disputed claim: the sealed file deliberately carries the unresolved conflict and embedded fallback. The original repository green CI did not invoke this new Python suite; a small \`.github/workflows/checks.yml\` step was added on the experiment branch to make its stdlib conformance tests part of host Checks. Consult the check run for the precise head revision rather than transferring an older green status.

**Remaining review caveats:** The "sealed" reader verifies a checksum over the snapshot and basic shape, not independent authenticity or comprehensive schema/semantic correctness; a self-consistent malicious publication cannot be distinguished from a legitimate one by a self-contained digest. E's digest pins selected mock bytes but does not authenticate a publisher. D shifts validation/composition to the publisher, so its one-file consumer timing is not a comparable whole-system cost. \`chmod\` plus fingerprint checks are not a physical read-only HFS mount. Synthetic conflicts have no topology, longitude/latitude, identity concordance, provenance/rights decision or political interpretation. No need for B, C or package management has been demonstrated.

**Decision after review:** the bounded test is suitable as **research-only evidence** for #31/#52 once CI on the corrected head is verified. Preserve D as the simple baseline; reserve E for genuinely useful explicit add-on selection; do not require A. The next production work remains #48's safe single existing optional external file, without any dependency on this mock format or a manifest. No owner hardware, production Reader, public contract, \`SIZE\` value, release milestone or accepted ADR was changed.
