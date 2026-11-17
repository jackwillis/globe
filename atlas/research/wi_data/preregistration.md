# EXP-WI-DATA-01 preregistration

Committed **before** any source acquisition or prototype code. 2026-11-16, America/Chicago.
Research only. Build-host only. Refs #52, #53. Does not change release scope, accepted ADRs
or readiness labels, and touches no G4. Synthetic or negative substitutes are labelled as such.

## Question and frame check

Can the existing atlas-production architecture carry heterogeneous Wisconsin Edition data
(statistics with uncertainty, dated historical boundaries, named physical features) through a
small reusable set of contracts without breaking the runtime's memory and portability limits?

Frame check (R&I frame-agreement gate). The assignment's wording leans toward "a shared
bounded semantic inspection interface". An alternative reading would make the question moot:
the existing build-time catalog (`atlas/research/catalog.py`: opaque IDs, source records,
time-qualified assertions, provenance) may already be the shared contract. The runtime may
need nothing new until a consumer exists (#137 cards, #63 inspector). Model D tests that.

## Models

- **A. Generic object store.** One physical row type: `(entity, property, kind, flags, value,
  aux, valid_from, valid_to, source)`, sorted by `(entity, property, valid_from)`. Geometry is
  an opaque blob value. Two variants isolate parameters from structure:
  - **A-wide:** fixed 32-byte rows, zero decode.
  - **A-lean:** per-entity grouped, varint-coded rows, sequential decode.
- **B. Specialized layer formats.** A columnar statistics table, a time-layered polygon
  layer, and a named-feature layer, each with its own native accessors and no common
  interface.
- **C. Specialized physical storage plus a shared bounded inspection/query interface.** B's
  bytes unchanged. Adds `describe(ref, budget)` and a few typed queries over per-layer
  descriptor tables.
- **D. No generalization (status quo plus build-time catalog).** Source builders emit
  existing-style layers per need. The sole shared contract is the existing catalog
  (identity, provenance, time) at build time. Tested by encoding the samples into the
  existing `catalog.py` contract and recording what cannot be expressed.

Reference points for sizes: GPOL v1 (int32 microdegree pairs, 16-byte ring and country
records), GPLC v1 (24-byte places).

## Data under test

1. Census: Dane County (FIPS 55025) tracts, cartographic boundary file, plus ACS 5-year
   `B01003_001` (population), `B19013_001` (median household income), `B25077_001` (median
   home value), `B01002_001` (median age): estimate and margin of error each.
2. Newberry Atlas of Historical County Boundaries: Wisconsin county versions with start and end
   dates.
3. WI DNR 24K hydrography: named waterbody polygons (Lakes Mendota, Monona, Wingra, Waubesa,
   Kegonsa) with WBIC identifiers; optionally a few named flowlines.

Licence handling: none of the three is on the allowed list in `docs/licensing-policy.md`
(Census TIGER status is a stated open question; Newberry's terms are bespoke; the DNR service
states none). All three are **research-only**: raw bytes stay outside Git, and committed tests
use synthetic fixtures. Only counts, sizes and hashes from real data enter the report.

## Predictions

Each states what would falsify it. Thresholds are set now. A prediction that fails is
classed afterwards as a **model failure** (structure wrong) or a **parameter error**
(direction right, magnitude wrong).

**P1. Dense statistics favour specialized storage.** Packed bytes for the statistics, per
tract, A-wide / B >= 3.0 and A-lean / B >= 1.3. Falsified if A-lean / B < 1.15, or A-wide /
B < 2.0. Expected B: 8 bytes per variable (int32 estimate, int32 MOE) plus a small header.

**P2. Heavy geometry is storage-neutral.** Where geometry dominates, an opaque-blob store
using B's coordinate coding is within 5% of B's bytes on the county and hydro samples.
Falsified if the difference exceeds 10%.

**P3. Lookups differ in probes, not in capability.** For (entity, variable) statistic lookup,
A needs about log2(rows) probes, B one. Deterministic probe counts (not Python wall time)
are the primary metric; wall time is secondary and carries no target claim. Falsified if
A's mean probes <= 2 x B's.

**P4. Identity is a shared build-time problem, independent of A/B/C.** All of A, B, C and D
need the same stable entity reference and source-qualified ID table, and the same count of
ID-handling exceptions (difference <= 1). Falsified if some model avoids the ID table
without a correctness loss on the identity negative fixtures (GEOID reuse across vintages,
ambiguous names, same WBIC on multiple parts).

**P5. Temporal correctness is representable in every model.** "Which county contained point P
on date D" returns identical answers in A, B and C and equals an independent brute-force
oracle run on the raw source, for at least 20 preregistered (point, date) probes including
dates on change boundaries (half-open `[start, end)`). Falsified by any disagreement. A
disagreement with the oracle counts as a model failure only if the oracle is verified
independent of the adapters.

**P6. Uncertainty and missing values need genuinely specialized behaviour.** Census sentinel
codes (estimate unavailable, MOE not computable, top/bottom-coded) cannot be normalized into
a bare number plus null. A model that stores `None` for them loses at least one distinction
that a query needs ("no data" vs "median exceeds top code"). Falsified if all sentinel
classes in the real sample collapse to one reason without changing any preregistered query
answer. At least one sentinel class is expected in Dane's four ACS variables (UW-Madison
campus tracts, zero-population tracts).

**P7. C needs no change to B's bulk bytes, only descriptors.** The inspection interface can be
layered over B with a per-layer descriptor block under 2 KiB and a shared provenance table
under 1 KiB. Falsified if C needs per-record provenance or time inside B's payload (bulk
bytes change by more than 2%).

**P8. Inspection is bounded.** `describe(ref, budget)` renders all three entity kinds within
a 1 KiB output and at most 24 property lines, never allocating proportional to layer size.
Falsified if any entity exceeds the budget without truncation being reported, or if truncation
loses the provenance line.

**P9. Shared behaviour is mostly semantic, not physical.** Counted by lines in the prototype:
at least 50% of query/inspect code is shared across entity kinds in C, 100% in A, 0% in B.
Source-specific exception handlers are registered by id and counted. Falsified if C's shared
fraction is below 35%.

**P10. D alone is insufficient for stats and time.** Encoding the samples in the existing
`catalog.py` contract loses or rejects at least one of: ACS margin of error as an attribute of
a value, multi-valued dated properties on one entity, or geometry association. Falsified if
all three are expressible with existing relations and fields without changing `validate()`.

**P11. Peak decoded memory.** For the real samples, B's decoded working set stays below 1 MiB
plus layer bytes (zero-copy or per-candidate), A-lean's full-decode working set stays below
2 x its packed bytes plus 64 KiB. Measured by `tracemalloc` peak and by accounting. Falsified
if either bound is exceeded. Host figures, not G4.

**P12. Simplification and quantization.** Microdegree int32 coding moves no vertex by more than
5e-7 degrees (round-trip), and creates no new invalid ring in the real samples that the source
did not already have. Falsified by a quantization-induced invalid ring.

## Expected outcome under each model (stated before testing)

- A: simple, one reader, loses on dense tables (P1) and on a typed uncertainty (P6).
- B: smallest, but three interfaces; inspection and cross-layer queries repeat.
- C: B's bytes plus shared semantics; extra cost is descriptors.
- D: cheapest now; must fail P10 at the catalog and cannot answer runtime queries.

My prior favours a narrow result: a specialized physical format per heavy kind, a shared
identity/provenance contract at build time, and an inspection interface only when a consumer
exists. This is a prior, not a result. C is not assumed to win.

## Queries (fixed before running)

- Q1 inspect a tract: all variables with estimate, MOE and flags, plus provenance, within
  budget.
- Q2 which tract contains the Capitol Square point (43.0747, -89.3841), and its population.
- Q3 which county contained a point at Madison on 1840-01-01, 1848-06-01, 1850-01-01,
  2000-01-01, and on every change boundary date present in the Dane-containing versions.
- Q4 list waterbodies named "Lake Mendota" with WBIC, area, source ID.
- Q5 tracts whose bounding box intersects Lake Mendota's (cross-layer join).
- Q6 Dane County boundary history: versions, ranges and change notes.
- Q7 median household income for a tract whose value is missing or annotated.

## Negative fixtures (synthetic, labelled)

Malformed GEOID, GEOID reused across vintages, ambiguous name, same WBIC on several parts,
contradictory population claims in one source, temporal gap, overlapping versions, inverted
interval, open ring, self-intersecting ring, duplicate vertices, sentinel misuse (a MOE
without an estimate), non-finite coordinate.

## Measurements and rules

Record input bytes, feature counts, intermediate and packed bytes, probes, tracemalloc peak,
wall time (host only), quantization error, exception count, shared/duplicated line counts and
reproducibility (two runs, byte-identical outputs). Each figure is labelled **measured**,
**estimated** or **assumed**. Host numbers are not G4 evidence. No causal claim is drawn from
Census data: the experiment is architectural; ACS estimates are used as data shapes only.

## Untested by design

Classic Mac execution, G4 timing, CD seek behaviour, 68k builds, real tract-vintage changes
(synthetic substitute only), non-Wisconsin generality, production licensing decisions.
