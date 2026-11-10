# Country-aware settlement selection (#91)

**Experimental research; do not merge without owner review. No production policy change.**

Recommend a future **country-local nearest-selected-distance policy**, provisionally with a
400 km saturation scale, after reviewing mandatory capital/territorial and remote-place seeds.
Keep inverse-square as a research comparator. Its modest advantage over the tuned simpler rule
on this dataset does not yet justify an extra population exponent, smoothing scale and cumulative
competition model. Neither candidate is ready to replace the Compact selection.

## Scope and provenance

Inspected baseline and local main: `a0e01f3be96f6556502d62d51320a0e9c1ccc75d`, exactly the assignment's
baseline. Fetch did not advance origin/main; it exposed independent UI/housekeeping branches.
The owner's unstaged renderer edits and untracked AGENTS.md remain in the original checkout.
All pre-existing accessible worktrees were inspected and were clean except those original edits;
no existing branch, PR, build directory or hardware was changed. Work is isolated in
`codex/country-aware-city-selection`, at `/tmp/globe-country-selection`.

Inputs actually read: the local hash-locked Natural Earth 10m populated places **5.1.2**
(7,342 rows, 225 raw ADM0_A3 pools) and countries **5.1.1** (258 rows). All 20 local NE archives
passed the existing lock verifier. The existing catalog README, fixture/review infrastructure,
recipes, audit/selection/role-selection implementations and tests, identity checkpoint,
concordance evidence and live issues #91/#52/#53/#38/#88 were inspected. PR #93 is separate UI work.
No new source was acquired and neither lock changed. Full source hashes and implementation
hashes are in the companion comparison JSON; source bytes are not redistributed here.

The two retained production Wikidata responses are separately pinned, dated 2026-11-01 and
2026-11-02 (20 and 534 bindings). The latter SHA-256 is
`88958f1d3f969c3d2423f299e2c460b397ff1af7dd3c61273d0a3098d569b98f`.
The existing GPLC builder selects >=1M population, at most 30 per source country, and deduplicates
by QID with a documented exclusion. Rebuilding it produced **304 records, 9,641 bytes**, identical
to production `cities.gplc`. The SPARQL uses MAX population and SAMPLE coordinates/country; the
retained response, not reexecuting that query, is the reproducible input. NE IDs/ADM0_A3 and
Wikidata QIDs/P17 are separate namespaces: no cross-source identity join is made here.

## Policy and review boundaries

A reproduces `audit.select_places` with the unmodified Compact recipe: source capital flags,
cartographic zoom importance, regional centers, one source-ranked representative per country
and occupied equal-angle grid bin. It is distinct from the production Wikidata policy.

B retains mandatory seeds then ranks population within each source-country pool. C uses
`(P/1M)^alpha / (1 + lambda * sum((Ps/1M)^beta * L^2 / (d^2 + d0^2)))`.
Defaults: alpha=beta=lambda=1, L=100 km and d0=25 km. Thus a selected 1M city at 100 km contributes
about 0.94 to the penalty before lambda. D uses `(P/1M)^alpha * min(1, nearest_km/L)`, initially
L=100 km, with 400 and 1,000 km comparators. All distances are great-circle spherical km.
These parameters are research defaults, not learned geographic truth.

Default stopping requires positive finite POP_MAX >=50,000 and marginal score >=0.05 (in
million-person normalized score units). Missing, nonpositive, nonnumeric and nonfinite population
is ineligible for secondary selection. Ten actual source rows have missing/nonpositive population.
Mandatory seeds bypass both thresholds. Candidates compete within their ADM0_A3 source pools.
Countries propose one candidate each round against the same previously selected snapshot; ties
use population then lexicographic source ID. This is explicitly a round-based allocation policy,
including under a budget; it is not a global best-score optimum. Each selected/candidate competition
pair is updated once in a research workspace. There is no inference about runtime storage costs.

A separate illustrative byte ceiling accounts for 32 fixed bytes + exact UTF-8 name + NUL per
record; a ceiling below mandatory-seed cost fails. An equal-count option and country caps are
used **only for controlled experiments**, with marginal threshold set to zero and the same
intrinsic eligible pool. They are not a fixed national quota or proposed stopping rule.

There are **zero approved capital identities** in the supplied review overlay. Verified capital
recall is consequently **undefined**, never 100%. All experiments preserve 214 provisional
national-or-alternate flag records; the 244-entry capital candidate inventory also includes
30 class-only rows. The broader-role variant includes all 244 provisionally. Current national,
multiple-function, territorial/regional and historical/disputed roles cannot be established
from these fields alone. Juba, Yangon, Johannesburg and Quezon City require role-specific review.

An optional reviewed-seed JSON supplies an exact source SHA and a `seeds` array. Each entry needs
`source_id`, `status: approved`, a named `reviewer`, ISO `date`, `evidence`, a `role` (national,
multiple-function, territorial or regional), and `basis_sha256` from the generated capital
inventory (or `country_selection.basis` for other candidates). The basis binds the complete
adapted record including the raw source-record digest. Changed source/coordinate/attribution
invalidates it; duplicate/missing seeds fail. Approval of the research checkpoint or catalog ID
allocation never creates an approved capital claim. Territorial/regional approved seeds are
reported separately from approved national/multiple-function capital recall.

## Duplicate QA before selection

There are **14 suspicious pair-evidence groups**, including **nine shared-QID pairs**, ten
nearby normalized-name/alias pairs and two cross-country review pairs. There are no repeated
NE_IDs or <=250 m coincidence pairs in this pinned place roster. No actual lexical city/metro
name pair was detected; that detector is exercised synthetically. This does not establish the
absence of city-versus-urban-area duplicates: POP_MAX itself is metro-oriented and mixed-date.

Evidence includes source hash, original record ordinals, raw-record digests, names, QIDs,
coordinates, distances and reasons. Pairs never become transitive equivalence clusters. Only
identical full raw-record digests plus identical adapted records qualify as definite repeated
records; repeated IDs block scoring, even if the records appear identical. Distinct IDs are
never automatically merged or excluded, so unresolved pairs can affect experimental scores.

Examples: Kasserine/Qasserine 11.058 km; Sakarya/Adapazarı 3.915 km; Mangyshlak/Aqtau 10.680 km;
Bandar Lampung pair 4.273 km; Kuta/Denpasar 8.241 km. Pyongsan/Pyongyang share a QID despite
93.937 km separation; Jingmen/Jianmen 100.637 km and Baicheng/Bose 2,820.560 km expose identifiers
that cannot safely define place equality. Ascensión (Mexico)/Ascension (Bolivia), 7,075.599 km,
is a cross-country shared-QID anomaly. Existing revision-pinned concordance evidence is included
with its report digest, attributed as prior work rather than newly fetched verification; its
Spratly/Wake map-unit cases remain outside the current settlement layer.

The same-name Kansas Cities (2.140 km), Maldonado/Punta del Este, Timon/Teresina,
Gisenyi/Goma (4.617 km) and Antarctic bases demonstrate false-positive risk. Accent folding and
localized aliases are suspicion signals, not an adjudication that these are one settlement.
Country-attribution QA is heuristic: it catches shared-QID/name/coincidence conflicts but does
not independently validate every country assignment against polygons or a political inventory.

## Results

Nearest distances sample **every source settlement**, including selected rows with distance zero.
They measure neither land area nor represented population. The common 15° equal-area reference
cells measure occupied-source-cell representation, not political regions or geographic fairness.

| Policy | Records | Median km | p95 km | Represented cells |
| --- | ---: | ---: | ---: | ---: |
| A Compact baseline | 373 | 240.927 | 713.804 | 153 |
| B population, natural stop | 4,237 | 0.000 | 452.831 | 142 |
| C inverse-square, natural stop | 2,044 | 67.894 | 469.111 | 141 |
| D nearest 100 km, natural stop | 3,556 | 14.658 | 452.831 | 142 |
| D nearest 400 km, natural stop | 2,066 | 68.712 | 469.111 | 140 |
| B matched C country counts | 2,044 | 78.257 | 645.885 | 136 |
| D 100 km matched C country counts | 2,044 | 67.788 | 580.134 | 137 |
| D 400 km matched C country counts | 2,044 | 67.154 | 483.911 | 141 |
| D 1,000 km matched C country counts | 2,044 | 67.194 | 480.213 | 141 |
| C cross-border major-city penalty | 1,715 | 79.467 | 532.292 | 140 |
| C broader provisional roles | 2,053 | 67.778 | 461.603 | 143 |
| C 32 KiB illustrative ceiling | 805 | 169.459 | 1,153.688 | 120 |

At matched 2,044 records **and country counts**, C improves p95 by 27.4% against population
ranking and 19.1% against the 100 km nearest comparator. Tuning the simpler distance scale
shrinks that advantage to **3.1% (400 km)** or **2.3% (1,000 km)**. The stronger nearest rule
slightly improves the median and matches occupied cells. These are post-hoc tuning experiments
on one source, not independent validation; no statistical significance claim is made.

At exactly the baseline's **373 records**, B/C/D have p95 **2,497.906 / 2,401.880 / 2,496.230 km**
and each represents only 97 cells. All are much worse than A's 713.804 km and 153 cells.
Population eligibility and round allocation lose remote/low-population settlements: adding cities
does not substitute for a reviewed geographic coverage policy. C's largest gap is 4,835.916 km
at Zhongshan Station versus A's 1,721.481 km at Chelyuskin. The 32 KiB ceiling also worsens tail
gaps despite selecting more than twice the baseline count. Budget is not a quality stopping rule.

C's natural-stop illustrative payload is **83,490 bytes**, versus A's **15,331 bytes**; these
exclude indexes, aliases, provenance, geometry, decoder overhead, allocation, workspaces and
peak residency. For the emitted metadata-rich records, A JSONL/gzip is **240,486 / 82,387 bytes**, C is
**1,249,131 / 437,544 bytes**, and matched-count nearest 400 km is **1,234,589 / 435,725 bytes**.
Exact JSONL and deterministic gzip byte counts/hashes, with Python/zlib versions,
are recorded per variant in the JSON. That serialization contains research metadata and is
neither GPLC size nor runtime RAM. No G4, OS 9 or NetBSD runtime performance was measured.

## Behavior and sensitivity

- Microstates: AND/LIE/MCO/SMR/VAT each retain one source-flag seed even below the population
  floor. This is provisional source recall, not independent capital acceptance.
- Very large countries: C selects USA 115, RUS 123, CHN 175, CAN 23 and AUS 16; there is no
  30-country quota. CHN is 8.56% of the natural-stop roster versus the baseline's largest-country
  share of 5.63%. Raw source attribution, including politically sensitive rows, is unchanged.
- Islands/archipelagos: IDN 51 and PHL 18 contrast with KIR/MDV/TUV one each. Tarawa and Malé
  survive as provisional seeds, while smaller remote settlements may fail significance.
  Longyearbyen, Tórshavn, Avarua and Antarctic stations illustrate baseline removals needing review.
- Dense neighbors: the cross-border experiment lets **previous-round selected cities >=1M**
  compete across boundaries, while candidate pools remain national/source-local. It drops 329
  records, reduces represented source pools from 208 to 206 and worsens p95. It has no same-round
  lookahead, so do not interpret it as equilibrium competition or endorse it as default policy.
- Thin/inconsistent populations: seeds survive missing population; no secondary population is
  imputed. Pure significance policies represent 208 raw source pools versus A's 225. Flagless
  territories and very small remote settlements are a structural gap, not proof they do not exist.
- Multiple administrative centers: multiple seeds are retained independently. The broader-role
  variant represents 216 pools; it cannot settle the roles of The Hague/Amsterdam, South African
  centers or Bolivia's centers. Juba remains in SSD; the countries layer's SDS is not auto-joined.

One-at-a-time C sensitivity: alpha 0.5→1.5 changes count 3,253→1,383 and p95 452.831→644.855 km;
beta 0.5→1.5 yields 2,113→1,866 and p95 472.015→490.426. Lambda 0.25→4 changes count 3,123→1,228,
p95 458.575→534.914. L=50→200 gives exactly the same outcomes: lambda and L² are redundant
parameters in this formula. Smoothing 10→100 km changes count 2,015→2,473 without changing
p95 (469.111), exposing limitations of a single tail statistic. A 10k population floor changes
nothing because marginal stopping already excludes those rows; 100k reduces count to 1,906,
p95 571.387. Marginal thresholds .01→.1 change count 3,765→1,349 and p95 452.831→593.249.
These unconstrained sensitivity runs vary count, so they diagnose instability rather than prove
an equal-budget improvement. With its own natural stop, nearest 400 km selects 2,066 places
and matches C’s p95 of 469.111 km, further weakening the case for inverse-square complexity.
Full latitude/country distributions and changed rosters accompany
the machine-readable comparison. Source dates and city/metro definitions remain uncontrolled.

Representative C additions against A include Satu Mare, Pattani, Manzini, Trincomalee, Uyo,
Gueckedou and Lashkar Gah; removals include Mariehamn, Famagusta, Capitol Hill, Grytviken,
Grand Turk, Douglas and remote stations. Exact country codes, IDs and complete added/removed
lists are in generated `*-changes.json` and selected `*.jsonl`; snapshot JSON retains 20 examples
per direction and full counts. These are source names, not approved naming or identity decisions.

## Reproduce and verify

Use existing pinned archives; commands never download. Full JSONL/gzip outputs stay outside Git.
The report records source/implementation hashes rather than HEAD so committing a report cannot
invalidate its own reproduction. Numeric reproducibility is scoped to the recorded Python/libm
and zlib toolchain; gzip bytes can vary across versions.

```sh
python3 atlas/research/sources.py /tmp/globe-atlas-sources
python3 atlas/research/country_selection_report.py /tmp/globe-atlas-sources /tmp/country-selection-final
python3 atlas/research/country_selection_report.py /tmp/globe-atlas-sources /tmp/country-selection-repeat
diff -qr /tmp/country-selection-final /tmp/country-selection-repeat
cmp atlas/reports/country-selection-comparison.json /tmp/country-selection-final/comparison.json
cmp atlas/reports/country-selection-duplicate-qa.json /tmp/country-selection-final/duplicate-qa.json
cmp atlas/reports/country-selection-capital-inventory.json /tmp/country-selection-final/capital-inventory.json
python3 atlas/research/test_country_selection.py -v
/tmp/globe-atlas-gis-env/bin/python -m unittest discover -s atlas/research -p 'test_*.py' -v
cmake -S . -B /tmp/globe-country-selection-build -G Ninja
cmake --build /tmp/globe-country-selection-build -j 4
ctest --test-dir /tmp/globe-country-selection-build --output-on-failure
tools/check-policy.sh
git diff --check
```

Host Linux x86_64, CPython 3.14.8: 15 new tests, 98 total research tests pass using the existing
GIS environment (no package installation); the system Python's initial discovery failed to import
optional Shapely. All 59 repository tests pass with private Xvfb display access; the first sandboxed
run failed eight X11 tests because displays could not open. No unrelated tests were excluded.
The new tests cover ordering/ties, multiple mandatory seeds, no eligible secondary, coordinates,
dateline/poles/antipodes, invalid populations, monotonic penalties, thresholds and independent
budgets, duplicate false positives, evidence-bound review, cross-border competition and an
independent direct-formula oracle for incremental score accumulation.

Files in this deliverable: `atlas/research/country_selection.py`,
`atlas/research/country_selection_report.py`, `atlas/research/test_country_selection.py`,
`atlas/reports/country-selection-{research.md,comparison.json,duplicate-qa.json,capital-inventory.json,verification.json}`.
No CMake registration is needed: the existing research suites use standalone unittest discovery.

## Decisions before any future edition prototype

1. Review current national/multiple-function capital identities and territorial/regional centers,
   binding named decisions to exact evidence; checkpoint acceptance is insufficient.
2. Adjudicate duplicate pairs, especially anomalous QIDs, without name-based automatic merging.
3. Decide the remote-settlement/station and tiny-territory coverage policy separately from
   population significance. Preserve the current Compact recipe until that policy has evidence.
4. Choose population time/definition/quality rules and how to handle missing values; do not
   treat mixed-date metro maxima as comparable census population.
5. Calibrate the simpler country-local distance rule and stopping threshold at intended edition
   sizes, with held-out geographic cases. Define country-allocation fairness under a tight budget.
   Cross-border suppression remains opt-in research.
6. Keep catalog inclusion separate from #38 label visibility and #88 symbol-size classes.
   Runtime format, memory ceilings and G4 acceptance require separately authorized work.

The next bounded task is editorial review of the generated capital/duplicate inventories, followed
by a coverage-preserving comparison of the simpler rule. This assignment stops at the draft
research artifact; it does not authorize production migration or runtime integration.
