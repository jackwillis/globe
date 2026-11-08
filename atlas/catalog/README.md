# Canonical identity/concordance contract prototype

This is a **research fixture**, not an approved political catalog, runtime pack or source correction.
`registry.json` explicitly allocates 35 opaque project IDs for selected examples. Never recycle IDs
or regenerate them from names, ISO codes, QIDs, geometry indexes or sorted input position. The
registry's readable selectors are extraction fixtures for this pinned source only; ambiguous or
missing selectors fail rather than choosing a row. They are not future update matching rules.

`fixture.json` contains 35 exact source-record excerpts from three hash-locked Natural Earth layers,
35 independently allocated entities, 43 unreviewed assertions, 71 name records and 35 points.
All geographic decisions remain unreviewed, including proposed source-record/entity associations.
An allocated ID is bookkeeping and does not approve a claim. No consumer should treat an
unreviewed/rejected/deferred assertion as an approved join. No transitive closure, union-find,
fuzzy matching or automatic entity merge is performed.

## Contract boundaries

- Entity: stable opaque project ID and broad feature kind. No polygon index, display string or
  source code is its identity. The fixture deliberately keeps all selected source concepts separate.
- Source record: locked dataset identity, digest, record ordinal and exact copied fields. The
  ordinal is meaningful only within the locked bytes. NE_ID/ADM0_A3/GU_A3/QID retain their own
  field and layer scope; a new source revision requires reconciliation, never ordinal carryover.
- Assertion: subject/object references, explicit relation, evidence record IDs, source viewpoint,
  rationale, time basis and review status/reviewer/decision/date. Same entity, part of, administrative
  relationship, historical successor, contested attribution, unresolved candidate, code alias and
  geometry association are distinct. A shared SOV_A3 is not an unqualified sovereignty claim.
- Time: a dated interval uses explicit ISO dates; `source-unspecified` requires null bounds.
  Retrieval/version dates do not become effective dates. Open interval endpoints remain null.
  Historical successors are directed subject → successor; administrative/part-of assertions are
  directed subject → parent. The source viewpoint remains even after an editorial decision.
- Names: exact source value, source field, language (`und` for generic NAME), role and provenance.
  A fallback has encoding, explicit loss and a same-subject source-name lineage. It is independently
  reviewable and never replaces the original. Installed glyph coverage remains unknown.
- Points: role, coordinate, source and explicit containment status. A label point is not a verified
  interior anchor and is not required to lie inside a polygon. Geometry references can later be
  attached through typed assertions; no geometry or picker is defined by this prototype.

The validator checks structural integrity, source fields, references, temporal consistency,
capital flag evidence, explicit fallback lineage and review prerequisites. It does **not** establish
that an accepted geographic claim is true, decide a worldview, validate language tags globally,
reconstruct historical boundaries or guarantee acyclic global administrative relationships.
`verify_records` reopens the pinned archives and checks extracted fields against their actual rows;
it currently supports the three NE fixture layers, not arbitrary reference adapters.

## Review fixtures and evidence

- San Marino/Vatican/Monaco/Andorra/Liechtenstein and Kiribati/Tuvalu/Maldives exist independently
  of coarse geometry; Greenland/Denmark source association is attributed and unreviewed.
- Palestine/Israel, Somaliland, Kosovo, Western Sahara and Taiwan retain exact source types and
  sovereignty fields. The explicit Palestine/Israel association is a contested-attribution review
  proposal, not a project sovereignty decision.
- Pyongsan/Pyongyang, Bose/Baicheng, Kuta/Denpasar and Spratly/Wake keep separate IDs despite shared
  QIDs. Jianmen and Ascension retain the anomalous source QIDs. See the revision-pinned evidence
  in `../reports/concordance.json` and `../references.lock.json`; no replacement QID is proposed.
- French Guiana and Tokelau map units illustrate an inventory beyond the countries layer.
- Juba's SSD and the countries layer's SDS are a source-code-alias proposal; the city and country
  remain different entities. Source national/alternate capital flags are preserved with unknown
  effective time, not reduced to a universal one-capital assertion.
- Equatorial Guinea's source-label point is retained with **unverified** containment. Existing
  `../reports/geometry-qa.json` reports it outside the 10m planar source polygon. That report's
  limitations still apply; this fixture deliberately makes no new spherical/picking assertion.
- Reykjavík and Łódź retain Unicode. Lodz remains a lossy Mac Roman fallback candidate, unapproved.

## Reproduce

Use the existing locked source cache; no network is used:

```sh
python3 atlas/research/build_catalog_fixture.py CACHE /tmp/catalog.json
cmp atlas/catalog/fixture.json /tmp/catalog.json
python3 atlas/research/catalog.py atlas/catalog/fixture.json --cache CACHE
python3 atlas/research/test_catalog.py -v
```

The fixture is small enough to review in Git; full downloaded datasets and selected-place JSONL
outputs remain outside Git. Production adoption requires reviewed identity/relationship decisions,
source-update reconciliation, claim-level reference adapters, a capital/territory inventory and a
separately approved bounded output contract. Neither #52 nor #53 is completed by this prototype.

## Expanded source-concept inventory and explicit review

Following Jack's acceptance of the research checkpoint, `allocations.json` extends the fixture
without renumbering its IDs. It allocates **808 proposed source concepts**: all 258 country records,
all 298 map-unit records, and 252 settlements (244 source-role candidates plus retained anomaly
fixtures). Country and map-unit representations remain separate even where source identifiers
match. The first assignment uses a deterministic traversal once; subsequent generation consumes
this persisted registry and cannot allocate implicitly. A changed source revision/ordinal or
missing record fails and requires explicit reconciliation. IDs are never silently dropped or reused.

The full catalog retains 21,818 name records, 808 representative points and 1,059 attributed
assertions. It is generated outside Git (about 12 MB of JSON); the compact allocation registry and
`../reports/catalog-inventory.json` retain reviewable state and diagnostics. Classification/source
fields remain exact. Candidate links and all geographic assertions stay unreviewed.

The broader role roster has 214 national-or-alternate **flag** records and 30 additional Admin-0
**class-only** records. Juba has `FEATURECLA=Admin-0 capital` with both flags zero; Yangon and
Johannesburg also illustrate why class-only rows require review. Quezon City is a flag-only case.
Regional/territory roles remain distinct from national roles. `source-capital-classification`
assertions preserve class-only evidence; they do not assert a current national-capital relationship.
Juba needs an SSD/SDS alias candidate, while Longyearbyen targets a map-unit source representation.

`review-decisions.json` records acceptance of the **research checkpoint**, separately from its
initially empty geographic decision list. `research/reviews.py` binds individual decisions to a
SHA-256 of the claim semantics, referenced evidence, source digests and name lineage. Changed
records/relations/coordinates invalidate prior decisions. Duplicate or missing targets fail.
The overlay changes only review fields, never source data or relation type. An unresolved candidate
cannot be accepted into equality by changing its status. A new typed assertion needs its own review.

```sh
python3 atlas/research/inventory.py CACHE /tmp/inventory.json /tmp/inventory-report.json \
  --review-queue /tmp/review-queue.jsonl
python3 atlas/research/catalog.py /tmp/inventory.json --cache CACHE
python3 atlas/research/test_inventory.py -v
python3 atlas/research/test_reviews.py -v
```

Normal generation consumes the committed allocation registry and review overlay. `--allocate` is
only for deliberately reviewed roster extensions; it is not source-update reconciliation. The
review queue covers relationship assertions, representative points and lossy fallback candidates;
it omits the thousands of exact localized source-name rows to keep the first review manageable.
Each queue entry contains the target table/ID and `basis_sha256` needed for a named, dated decision.
No geographic decision is fabricated in the shipped overlay. Historical source policy and disputed
attributions remain Jack's editorial choices, distinct from approval to implement these tools.
