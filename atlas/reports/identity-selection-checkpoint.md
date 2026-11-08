# Identity and settlement-selection checkpoint — November 8, 2026

Research only, based on `52ccb33`. No production packs, recipes, runtime, font policy or source
locks change. The explicit next review contract is in `../catalog/README.md`; the executable fixture
preserves 35 separately allocated identities and their unreviewed source/name/point assertions.

## Reconciliation of #53

The previous work was already integrated: `codex/atlas-audit` at `39e46f1`, both historical CI
branches, Claude's committed typography and Classic work are ancestors of the inspected main.
The two later semantic commits `a5c5ba8` and `be0278d` are present. After the authorized fetch,
`origin/main` and observed main were `52ccb33`; the three newer commits are coordination documents.
No merge was performed by this session. Existing owner/agent renderer WIP and the old stash are
preserved. Full workspace recovery details are in the session handoff.

Verification of important existing evidence did not repeat the full exploratory program:
all 20 retained NE archives verify against the lock; all 21 retained experimental layer JSONL
hashes, byte counts and record counts match `editions.json`; the existing semantic cross-report
checks pass. The initial 373/1,362/7,342 place counts remain reproducible evidence. Raster conversion
and planar topology findings remain attributed to the earlier reports, not remeasured here.

Documentation clarification: the older recommendation says that source representative points
were not independently checked and that NOAA/RESOLVE had not been reconverted. Those were dated
initial-pass limitations, narrowed by `semantic-followup.md` and its actual raster/geometry reports.
They should not be read as the current state. Similarly, #61's historical comment that typography
was unmerged is stale relative to the inspected ancestry. Installed fonts remain the governing rule.

#53 remains open with no changed acceptance boxes. Suggested review disposition: accept the
bounded research foundation only by explicit owner decision; track reviewed identity/concordance,
capital inventory/points, settlement policy, spherical picking, hydrographic coherence and typed
pack feasibility as separate scoped follow-ups under #52. This session advances the first and
third as proposals; it does not certify the full world inventory or close those follow-ups.

## Actual selection comparison

All variants use the same pinned 7,342 places, capital/alternate-capital flags, source importance
thresholds and one representative per source country code. Only grid sampling changes; a separate
comparison restores the original count using farthest-first spherical-distance supplements.
Uniform longitude × uniform sin(latitude) yields equal spherical cell area, not equal cell shape.
No M49 reference table is redistributed or used to assert political regional membership.

| Edition/policy | Places | Reference equal-area cells represented | Source-settlement p95 nearest distance, km | Largest distance, km | Actual gzip JSONL bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Compact equal-angle | 373 | 153 | 713.804 | 1,721.481 | 103,605 |
| Compact equal-area | 368 | 146 | 739.795 | 1,862.507 | 102,182 |
| Compact equal-area, equal count | 373 | 151 | 711.078 | 1,372.461 | 103,693 |
| Standard equal-angle | 1,362 | 182 | 353.045 | 941.671 | 369,439 |
| Standard equal-area | 1,355 | 183 | 358.692 | 941.671 | 367,485 |
| Standard equal-area, equal count | 1,362 | 183 | 350.084 | 724.759 | 369,808 |

Reference cells are a common 15° nominal grid (24 longitude × 12 equal-area latitude bands),
limited to cells containing source settlements. Distances are measured for every source settlement
to its nearest selected settlement on a spherical Earth. These are **not** land/population coverage,
selection-independent quality scores or sovereign-state completeness. All six candidates preserve
214 unique source national-or-alternate-capital records (200 national-flag records plus additional
alternate-only records) and 225 source country codes, with zero missing flagged capitals. All eight
named microstate/island fixtures retain at least one settlement. Country-code absence lists remain
raw namespace comparisons; SSD/SDS and territories require concordance before interpreting omissions.

Equal-area alone drops 7/adds 2 Compact places and drops 10/adds 3 Standard places. The complete
changed rosters, coordinates, country-code/latitude distributions, missing source units, small-country
counts, byte counts and hashes are in `selection-comparison.json`. Compact adds Minneapolis/Durban;
Standard adds Alotau/Ust-Ilimsk/Walvis Bay. Some removed places are Arctic or Antarctic settlements.
The equal-count supplements recover wider distances, including Atka/Horta/Chelyuskin in Compact and
Raoul Island Station in Standard. Stations and very small isolated settlements expose the need for
editorial review; spherical distance alone is not an atlas importance policy.

The common role/country rules dominate counts, so equal-area bins alone are not demonstrated to
be a better rule. The equal-count experiment modestly improves p95 and substantially improves
largest source-settlement distance, at slightly larger compressed payload. **Recommendation:** keep
shipping/experimental recipes unchanged pending editorial review of actual changed candidates and
an independent regional/territory inventory. Catalog inclusion remains separate from marker
visibility and text placement. No screen-space label policy is supplied here.

The report gives an explicitly illustrative 32-byte record + UTF-8 default-name resident scenario;
JSONL/gzip figures are actual payload measurements. Neither is measured runtime RAM. A repeat build
must compare the report and all six output pairs. Build-host elapsed time and peak RSS are recorded
in the session verification file; those measure Python, not the G4.

## Cross-agent requests and next decisions

At branch `codex/atlas-contract`, Claude owns the next #36/#38/#61 API decision. Requested inputs:
which encoding profiles the actual installed-font adapters can consume, how missing-glyph results
are signaled, and whether alternate anchors are needed. Atlas can provide stable entity/source IDs,
role flags, source ranking, exact name/language/field, unreviewed encoding fallback candidates and
independent representative points (`catalog/fixture.json`, `research/name_profiles.py`). The adapter
must measure its actual selected string. This is a dedicated handoff request; no renderer changes
or messages to other agents are needed to adopt this fixture.

Jack owns political scope, exceptional crosswalk decisions, capital inventory, naming/fallback
approval and selection policy. The smallest next atlas task is review of the 35 fixture identities
and flagged assertions, with named decisions/evidence; then expand the registry to a complete,
reviewed country/territory inventory. Do not mint global IDs by automatic QID/name/ISO merging.

The separate #55 proposal in `memory-ledger-design.md` is precise enough for a small independent
accounting primitive. Allocation-site integration remains a later coordinated runtime slice;
no degradation, eviction, CI budget gate or current guard replacement is implemented here.
