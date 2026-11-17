# EXP-WI-DATA-01 report: Wisconsin data architecture

2026-11-16, America/Chicago. Linux x86_64, Python 3.14.8, standard library only. Research only; refs
#52, #53. Nothing here changes release scope, an ADR or a readiness label, and no G4 or Classic Mac
was touched. Predictions were committed first (`bf6537f`, [preregistration](preregistration.md)).
Host figures are not G4 evidence. "Measured" means produced by the commands in section 5;
"estimate" and "assumption" are marked where used.

## Summary

- **The byte layout is not where the heterogeneity is.** Geometry dominates every comparison.
  Full-resolution Newberry county history is 17.07 MB packed (2,127,371 vertices) in every model.
  A generic store costs +0.2% on it and +2.4% on the waterbodies. The expensive differences
  are *semantic* (Census sentinels and censoring, closed source dates with a data horizon, proposal
  records) and every model had to implement them once, in the same place.
- **Recommendation: keep specialized physical layers and do not generalize the runtime now.**
  Extend the build-time catalog contract with a validated claims table (it rejects statistics
  today). Add a descriptor-driven inspector only when a real consumer (#137 cards) exists.
  This is closest to model B plus a build-time model-D extension, with C deferred.
- Two predictions failed (P3, P11) and one missed narrowly (P8). Section 6 classifies each.

## 1. Baseline

The existing atlas is already a family of specialized layers (GATL, GPOL, GPLC, GELV, GGRD), each
with a strict zero-copy decoder, plus a build-time catalog (`atlas/research/catalog.py`) holding
opaque IDs, locked source records, time-qualified assertions, names and points. ADR 0003 favours
typed layers with a small manifest; draft ADR 0005 wants stable `EntityRef`s separate from
source IDs and pack indexes. PR #182 independently studies edition-selection semantics over
synthetic data; PR #181 is the first external-file load path (GATL only). Neither overlaps these
files. Existing code reused: `atlas/research/sources.py` (strict SHP/DBF). Existing code
that did **not** extend: `catalog.validate` (section 4, D).

## 2. Source qualification

All three are **research-only** under `docs/licensing-policy.md`; no raw byte is committed.
`sources.lock.json` holds URL, size, SHA-256 and licence status.

| Source | Acquired | Licence finding |
| --- | --- | --- |
| Census CB 2022 tracts 1:500k (WI) | 1,037,532 B zip; 125 Dane tracts | TIGER public-domain status is an open question in the policy |
| ACS 2018-2022 5-year, tables B01003, B19013, B25077, B01002 | 3.8-6.2 KB extracts of Dane tract rows | Data API now needs a key (a registration email the owner keeps out of requests); nationwide table files are 17-26 MB each. Rows are sorted by GEO_ID and the server honours Range, so 13 ranged requests per table fetched only Dane rows. Full-file hashes were not computed; extract hashes and source sizes are locked. |
| Newberry AHCB, Wisconsin | 24,535,249 B (smallest offered; above my 8 MiB default cap, so a per-source cap) | **Conflict:** the website says reuse for any lawful purpose; the bundled READ ME and deed say CC BY-NC-SA 2.5. NC is on the hold list. Owner review needed before any use. |
| WDNR 24K waterbody, 5 named Dane lakes | 137,723 B ArcGIS JSON | The service states no licence |

Real counts: 125 tracts (4,404 vertices), 389 county versions of 134 IDs (2,127,371 vertices;
72 current at the 2000-12-31 horizon; 3 are proposals), 5 lake features (3,221 vertices). The 125
ACS GEOIDs equal the 125 boundary GEOIDs exactly. The ACS reference period is 2018-2022, not an instant.

## 3. Prototype

`atlas/research/wi_data/` (README lists files). Shared build-time model: source-qualified keys
(`census:geoid@2020:...`) to stable synthetic `EntityRef`s in the reserved test range, claims
`(entity, property, value, status, MOE, interval, source)`, half-open day intervals, provenance.
- **A-wide / A-lean:** one statement store (32-byte rows, or varint groups with an entity index).
- **B:** four independent layers (columnar stats with Census codes in-band; tract, county-version
  and waterbody polygon layouts), geometry as int32 microdegrees as in GPOL.
- **C:** B's bytes unchanged plus 331 B descriptors, 323 B provenance and a bounded `describe`,
  `locate`, `find_name`, `intersecting`, `history` interface (`iface_c.py`). The same inspector
  also runs over A, which is how cross-model equality was tested.
- **D:** 7 attempts against the real `catalog.validate`.
Queries Q1-Q7 and 93 county (point, date) probes (every boundary day around Madison's versions,
six cities, the four preregistered dates) were checked against an independent oracle (nonzero
winding on raw floats and the source's closed day ranges; the models use even-odd on integers).

## 4. Results

### Bytes (measured)

| Category | B | A-wide | A-lean | A-wide/B | A-lean/B |
| --- | ---: | ---: | ---: | ---: | ---: |
| Statistics (125 x 4 vars, est+MOE, period) | 4,536 | 20,016 | 10,451 | 4.41 (3.53 without entity-kind rows) | 2.30 |
| Tract geometry + names | 41,431 | 51,931 | 44,101 | 1.25 | 1.06 |
| County versions | 17,066,136 | 17,096,098 | 17,075,551 | 1.002 | 1.001 |
| Waterbodies | 25,990 | 26,605 | 26,176 | 1.024 | 1.007 |

Gzip of the county layer: 11.17 MB (B), 11.19 MB (A). Inspection blocks (identity table 8,337 B,
descriptors 331 B, provenance 323 B) are shared by A and C. The identity table is larger than
all the statistics, an estimate worth remembering if source-qualified IDs are shipped at runtime.
Time at row level (A-lean) lifted its stats ratio from 1.64 to 2.30 once the ACS period was
modelled; B keeps one period in the layer header.

### Lookups and decode (measured counts; host Python timings carry no target claim)

Statistic lookup compares per lookup: B 6.99 (binary search over refs), A-wide 9.36, A-lean
6.99 plus 5 decoded rows. With dense refs B would be 1, but stable cross-edition references are
not dense (assumption). One query suite decodes 3,092,889 points in every model; the largest
single feature is 145,016 points = 1,160,128 B as int32 pairs, over the preregistered 1 MiB.
Host wall time per suite: C 5.8 s, A-wide 9.3 s, A-lean 18.9 s (A-lean has no geometry index).
Python `tracemalloc` peaks (about 24 MB) measure Python object overhead and are not used.

### Correctness

0 disagreements among A-wide, A-lean, C and the oracle (93 + 33 synthetic probes). Quantization:
max error 3.0e-7 deg (counties), 5.0e-7 (lakes), 0 (tracts); 0 collapsed vertices; 0 of 1,468
rings non-simple in source or made non-simple (all rings checked with a grid test verified
against a quadratic reference). Source quirks recorded: 29 ACS sentinel cells (14 estimates,
14 MOE, 1 open-interval MOE), 3 proposals, 72 horizon-dated versions, 3 entities with temporal
gaps. Sentinel classes seen: 14 missing-estimate with missing MOE, 1 censored cell (a median at
its top code, MOE open-interval), 485 ordinary. Collapsing them would print the censored cell
as an exact 2,000,001. Describe output: 264 entities, max 1,038 B and 24 lines, 12 truncated
(long county histories), every one keeping its provenance line.

### Model D (measured)

`catalog.validate` accepts identity, time-qualified names, successor assertions and geometry by
reference. It rejects entity kind `tract`, an estimate as a name, a `statistic` relation, and
sources absent from its two lock files. It also **accepts an unknown top-level `statistics` table with
garbage in it**, so a quiet extension would go unvalidated.

### Seeded defects and a real bug

Five seeded defects (off-by-one interval, proposals as counties, always-inside polygon, ignored
censoring, A ignoring entity kind) are each caught. A rectangle-only fixture initially missed
the always-inside mutant; a notched polygon and a hole were added. The proposal fixture exposed a real
defect in my own pipeline (proposals answered "which county"), fixed by an entity kind in A and
a flag in B's county layout.

## 5. Commands

```sh
python3 -B -m unittest atlas/research/wi_data/test_wi_data.py       # 40 tests, 1 skipped (real cache)
WI_DATA_CACHE=build-wi-data/sources python3 -B -m unittest atlas/research/wi_data/test_wi_data.py
python3 atlas/research/wi_data/experiment.py out.json               # synthetic, committed as results-synthetic.json
python3 atlas/research/wi_data/experiment.py out.json --real build-wi-data/sources   # results-real.json
tools/check-policy.sh                                                # policy: ok
```
Both modes build twice and compare fingerprints of every packed layer: byte-identical. Not run:
C++ build/ctest (no C++ changed), CI wiring (workflow shared with #179/#182), G4, 68k.

## 6. Predictions against observations

| | Prediction | Observed | Verdict |
| --- | --- | --- | --- |
| P1 | A-wide/B >= 3, A-lean/B >= 1.3 | 4.41 / 2.30 | held. Wide row width is a parameter; per-statement entity, property and interval overhead is structural |
| P2 | geometry within 5% | counties +0.2%, water +2.4% (tracts +25%, outside stated scope) | held |
| P3 | A probes > 2x B | 9.36 vs 6.99 (1.3x) | **failed**: B also needs a ref search unless refs are dense. Parameter error (direction right, my "1 probe" assumed dense refs) |
| P4 | identity shared, same exceptions | one registry for all; a bare GEOID key merges vintages | held, partly by construction |
| P5 | identical to oracle | 0 disagreements, 126 probes | held |
| P6 | specialized sentinel behaviour needed | 3 classes seen; collapse prints a false exact value | held; 4 documented codes unobserved |
| P7 | descriptors < 2 KiB, provenance < 1 KiB, bulk change < 2% | 331 B, 323 B, 0% | held; B itself needed a proposal flag |
| P8 | describe <= 1 KiB, <= 24 lines | max 1,038 B, 24 lines, 12 truncated | **narrow miss**: parameter error (marker and reserved provenance counted late) |
| P9 | C shared >= 50% | 60.5% (A 80.1%, B 0%) | held (lines of prototype code; weak metric) |
| P10 | catalog insufficient | statistics rejected 3 ways; unknown table silently accepted | held |
| P11 | B decoded set < 1 MiB + layer | largest feature 1,160,128 B | **failed** for full-resolution counties. Model-level finding: the geometry budget, not A/B/C, is the constraint |
| P12 | quantization creates no invalid ring | 0 | held |

**Requisite variety.** Genuinely specialized: Census annotation and censoring semantics; AHCB
closed dates, horizon and proposal types. Safely normalized: source-qualified identity with
vintage scoping, half-open intervals, names, provenance, geometry coding, bounding boxes. Waterbodies
needed nothing source-specific beyond treating WBIC as a claim and not as identity (synthetic
shared-WBIC fixture; the five real features each had a distinct WBIC).

## 7. Comparison and recommendation

| | A | B | C | D |
| --- | --- | --- | --- | --- |
| Bytes | equal on geometry, 2-4x on dense stats | smallest | B + about 650 B | nothing at runtime |
| Complexity | one reader, but descriptors still needed | four layouts, no provenance in bytes | B + 220 lines of glue | cheapest now |
| Identity/time | via shared model | same | same | cannot hold statistics |
| Queries | scan or extra index | native per layer | uniform | none |
| Classic fit | untested | matches GPOL family | adds a small table reader | none |

No-generalization option: valid and cheapest. Statistics-only data can ship as one B-style layer
without any shared interface. The shared interface pays off only once more than one consumer needs
cross-layer inspection.

**Recommended path (owner decision):** (1) keep specialized layers; (2) add a validated
`claims` table to the build-time catalog (value, MOE, status, period, source), failing closed
on unknown tables; (3) defer C until #137 needs `describe`, then fold descriptors into each layer
header; (4) before any county-history layer, generalize geometry (full-resolution WI is 1.16 MB per
feature decoded and about 51 MB as a 24 B/vertex unit-vector cache, an estimate assuming the
CountryScene design).

## 8. Limits and untested assumptions

Synthetic only: GEOID reuse across vintages, shared WBIC, ambiguous names, temporal overlap, open
rings, bowties. Dane only; one ACS vintage; four variables; 4 of the documented annotation
codes unobserved; ~100 variables would change the stats picture (estimate: A-wide about 4.9 MB vs
B about 1.2 MB for 1,528 tracts). Counties end at the 2000-12-31 horizon. A bounding-box cross-layer
join, not true intersection. Name search is linear. Even-odd fill, planar. No Classic Mac, G4, CD
or memory-ledger evidence. Census data was used for shapes only; no causal claim is made.

## 9. Smallest next slice

A stdlib builder that emits one validated ACS claims table into the catalog for Dane tracts using
the existing identity allocation, plus a catalog change rejecting unknown tables. It needs owner
approval of the licence question for TIGER/ACS first; the Newberry source needs a separate
licence ruling and a generalization experiment.
