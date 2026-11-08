# Atlas audit and candidate editions (#53)

Research result, November 7–8, 2026, America/Chicago. **Proposed**: retain Natural Earth as the initial
cartographic source, keep a Unicode-rich build-time catalog, select places by geographic role and
cartographic importance, and investigate independently loadable typed layers with a manifest.
The evidence supports further work, not a production migration or an approved release matrix.

## Scope and evidence

The [baseline](baseline.md) covers fetched main `886d126`. Subsequent integration includes the
owner's local main and Claude's committed work; atlas changes themselves are confined to `atlas/`
and the separately committed README correction. All 16 requested issues and their comments were
read (#52, #53, #17, #26, #31, #48, #50, #54–#57, #59, #60, #36, #38, #61), as were the repository
handoffs, architecture, ADRs, format documentation, builders, decoders and the unmerged typography
branch. Issue #36's bitmap-font wording conflicts with the owner's newer system-font decision
and its own later comments; the latter decision governs this proposal.

The source lock contains **20** actually acquired Natural Earth archives, about 34 MB in total.
Exact byte totals, versions, URLs and SHA-256 are in `sources.lock.json`; archives stay outside Git.
Current sources are pinned by observed digest, not a claim of a publisher-signed checksum. The
lock's UTC timestamps fall on November 8; local research crossed midnight November 7–8.
NOAA/RESOLVE were inspected through current primary-source pages and existing provenance rather
than downloaded again. GeoNames, Who's On First, HydroRIVERS, OSM and UN M49 were researched as
alternatives/cross-checks; no bulk data from them is included in the experiments.

## What the source comparison establishes

| Layer | 110m | 50m | 10m |
| --- | ---: | ---: | ---: |
| Country source records | 177 | 242 | 258 |
| Country source vertices (including closure) | 10,654 | 99,613 | 548,471 |
| Populated places | not acquired | 1,251 | 7,342 |
| River/centerline source records | not acquired | 478 (one null) | 1,473 |
| River/centerline vertices | not acquired | 25,751 | 257,645 |
| Lake records | existing historical ledger only | 412 | 1,355 |
| Lake vertices | not remeasured | 19,778 | 162,852 |

The 10m sovereignty, map-unit and subunit layers contain **209 / 298 / 360 records** respectively.
These are different source classifications, not interchangeable answers to “how many countries?”
The 10m country roster has 81 records absent from 110m and 16 absent from 50m. This is a coverage
comparison against a chosen cartographic inventory, not a claim that 81 sovereign states are missing.
Full omission lists and required cases are in `political-coverage.json`.

San Marino, Vatican, Monaco, Liechtenstein and Andorra all lack 110m country polygons and all have
50m/10m polygons. The 110m tiny-country file does **not** supply those five points; its 37-record
roster must not be mistaken for a complete microstate remedy. The 50m tiny file has all five among
76 records. Kiribati, Tuvalu and Maldives have 110m tiny points but no 110m country polygon.
Greenland and multipart Fiji occur at every country scale. A richer identity catalog therefore
belongs in Compact even when fine geometry is absent. Source label points are retained as
representatives; their suitability for picking or placing labels has not been independently checked.

Natural Earth's [admin0 documentation](https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-admin-0-details/)
separates these representations and describes a de facto convention. Observed records need their
own review: `PSX` is “Palestine,” type “Indeterminate,” with sovereign source name “Israel”; `SOL`
is “Somaliland,” type “Sovereign country”; `KOS` is “Disputed”; `SAH` is “Indeterminate”; `TWN`
is “Sovereign country.” These are **source assertions**, not Globe's adjudication. Its explanatory
webpage and actual records are not a complete geopolitical policy. Preserve classifications,
claims, viewpoint, effective dates and provenance separately; do not turn `SOV_A3` into a universal
legal `part_of` relation. Greenland's source relation to Denmark similarly remains attributed.

[UN M49](https://unstats.un.org/unsd/methodology/m49/overview/) is a useful statistical country/area
classification cross-check with regional and ISO concordances. It is not a polygon source or a
sovereignty arbiter. A pinned full M49 join and historical boundary/identity policy remain follow-ups.

## Identity model and concordance risks

Use project-owned immutable entity identifiers in the eventual catalog, with revisioned crosswalk
assertions to source IDs. Keep entity class, geometry ID/resolution, time-qualified relationships,
names, edition inclusion and presentation priority separate. Treat source QIDs and country codes
as evidence to reconcile, not as sufficient proof of equality. A QID can refer to a city, metro area,
municipality or region; an apparently equal population can measure a different object or date.

This prototype uses explicitly provisional `ne:admin0:ADM0_A3` and `ne:place:NE_ID` identifiers.
It retains source code, classification, sovereign source relation and QID rather than assigning a
new world standard. Geometry identity additionally includes the locked source and record ordinal:
several river/border/lake records legitimately share NE_IDs. Segmented geometry must not overwrite
earlier records with the same feature ID. Political and place NE_ID uniqueness is a failure check;
geometry feature reuse is reported separately.

The 10m places file has nine repeated-QID groups, including Pyongsan/Pyongyang (`Q18808`),
Baicheng/Bose (`Q92381`) and Kuta/Denpasar (`Q11506`). Some could reflect overlapping concepts;
some look erroneous. Map units also associate Spratly Islands and Wake Atoll with `Q43296`.
These are review flags, not verified corrections. Rivers sharing a QID across segments are usually
expected. Do not globally deduplicate by QID or name. Missing/invalid QIDs and duplicate groups are
listed in `sources.json`; no fuzzy-name merge or live Wikidata enrichment was performed.

The next real crosswalk should store assertion source/version, candidate relation (same entity,
part, successor, disputed), reviewer, validity interval and confidence. Retain unresolved rows with
source-scoped IDs. Required-feature fixtures should grow from a reviewed country/territory and
capital inventory, with accepted omissions and disputed cases explicitly recorded.

## Place selection and naming

| Strategy | Selected records | Coverage units | NE capital flags |
| --- | ---: | --- | ---: |
| A: exact current Wikidata rule | 304 | 94 Wikidata country IDs | no capital attribute in snapshot |
| B: all 50m cartographic places | 1,251 | 217 NE admin0 codes | 200 |
| C: Standard hybrid recipe from 10m | 1,362 | 225 NE admin0 codes | 200 |

Coverage-unit counts are **not directly equivalent country counts**; a cross-source political join
is still needed. A starts from 534 snapshot rows, applies the actual builder's ≥1M population and
30-per-country rule and retains its exact output. Its query requires direct city/big-city classes,
uses maximum population and `SAMPLE()` coordinates/country, and lacks population observation dates.
The retained response, not reexecuting SPARQL, pins the bytes. Nineteen selected A QIDs are absent
from the 10m place roster. Thus Natural Earth is not an automatic superset replacement.

By QID comparison, A misses 119 of the 200 NE national-capital-flag records. Reykjavík, Bern, Nuuk,
Luxembourg, Vaduz, Ljubljana and Port Louis are absent from A and present in B and all three hybrid
editions. This verifies inclusion, not every source coordinate, capital role or population.
The readable exceptions and full distributions are in `places.json`.

[Natural Earth's place policy](https://www.naturalearthdata.com/downloads/10m-cultural-vectors/10m-populated-places/)
favors regional significance and provides population estimates with mixed vintage and metro-oriented
definitions. B is a strong baseline, not a guarantee of complete regional-center coverage. C retains
national/alternate capital flags, low minimum-zoom places, selected admin1 centers, one source-ranked
representative per admin0 code and per occupied equal-angle bin. Tie-breaking is deterministic.
Angular bins overrepresent high latitudes; this heuristic needs an equal-area and regional fairness
comparison before adoption. Administrative codes with no source settlement remain a catalog gap.

Catalog inclusion never means showing every label. Preserve source `MIN_ZOOM`, labelrank, class and
capital role; let the future platform-measured placement interface choose visible labels. Do not
hard-code a label renderer, zoom-to-font mapping or a universal city population ranking here.

Both current builders are lossy. The place builder uses NFKD, removes combining marks and applies
one Latin-oriented mapping; unmapped non-ASCII causes failure. The country builder uses ASCII
`ignore`, silently dropping remaining characters. These mechanisms cannot preserve meaningful
language distinctions or transliterate arbitrary scripts. In the 7,342 source `NAME` fields,
**696** are changed by the legacy place conversion; **101** cannot be encoded in Mac Roman after
NFC, compared with **108** in ISO-8859-1. Those counts concern source default names, not all the
localized Chinese/Arabic/etc. strings; font glyph coverage is a separate question.

The experimental catalog preserves exact source and localized strings, source field and language.
Generic `NAME` is `und`, not assumed to be an endonym. A future model should distinguish preferred
display name, language-specific preferred name, historic/alternate name, source name, and an
explicitly reviewed platform fallback with transformation/version and loss flags. Preserve originals
before normalization. NFC display strings and search normalization are separate derived products.
Source `NAMEASCII` can be a fallback candidate, never an authoritative translation.

The owner's system-font-only rule governs: bundle neither fonts nor glyph packs. The
[Apple Mac Roman mapping](https://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/ROMAN.TXT) permits many
Western European diacritics but not arbitrary Unicode; the name Reykjavík is representable, while
Łódź is not. Codec success still does not establish that a particular installed font has a glyph.
Classic script/encoding capabilities vary by installation. [Xlib](https://www.x.org/releases/current/doc/libX11/libX11/libX11.html)
distinguishes core font encodings and internationalized drawing; a core-font byte API is not
implicitly UTF-8. Inventory actual X11 fonts/locales and QuickDraw paths in Claude's #61 work.
Do not change runtime encoding under this assignment.

Metadata supplied to that future interface should include entity ID, representative location,
feature/capital role, source importance, name variants and language, preferred/fallback strings,
encoding profile and loss flag. The platform measures the **chosen actual string** in its installed
font. This audit does not claim glyph, shaping or bidirectional text support on OS 9 or NetBSD.

## Hydrography and physical geography

Both scales contain Nile, Amazon, Congo, Mississippi, Danube and Mekong (English/source-name
matching), with multiple source records or tributary language segments in some systems. The 50m
file has 361 River, 113 Lake Centerline and 4 Canal records (one River null geometry); the 10m file
has 1,201 River, 259 Lake Centerline, 11 River (Intermittent), and 2 Canal records. Names can be empty.
Ranks describe cartography; a high-rank tributary segment can belong to a major named system.
Rank filtering must be reviewed for continuity, not just aggregate bytes.

[50m documentation](https://www.naturalearthdata.com/downloads/50m-physical-vectors/50m-rivers-lake-centerlines/)
describes manual alignment/generalization and warns about missing intermittent rivers and suspect
alignments in northeastern Russia and parts of the Amazon. The observed 10m intermittent class
must not be generalized into a claim of comprehensive seasonal coverage.
[Lakes documentation](https://www.naturalearthdata.com/downloads/50m-physical-vectors/50m-lakes-reservoirs/)
describes coordinated ranking but notes changing lake extents. In the measured 50m DBF, 272/412
lakes have scalerank zero; blindly treating zero as globally most important includes too much.
The candidate recipes therefore filter lake `MIN_ZOOM` instead (≤2 Compact, ≤4.7 Standard).

`hydro-endpoints.json` groups the six systems by source QID and measures spherical distance to
nearest **shore vertices**, an upper bound on shoreline distance, at each matching scale. It also
counts endpoint coincidences at a 1e-5-degree quantum. Named main-stem Nile records' closest
endpoint is about 120 km from a coastal vertex; Danube about 30–32 km. Delta distributaries and
source grouping require review before declaring either a gap or a correct outlet. Amazon, Congo,
Mekong and Mississippi have much closer candidate endpoints. These are diagnostics, **not**
validated flow direction, river mouths, coast intersection, lake connectivity or routing topology.
No stream graph or topology repair is implemented. Lake centerlines must remain identifiable and
should not automatically be drawn over filled lake water. Artificial waterways and intermittent
features need separate style/selection rules.

Retain current [NOAA ETOPO](https://www.ncei.noaa.gov/products/etopo-global-relief-model) and
[RESOLVE](https://ecoregions.world/) for these experiments. GELV's -1m lake mask is a color proxy,
not inland-water bathymetry. Height, water, ice and biome semantics need separate future contracts;
Greenland must not be “fixed” by relabeling elevation or inventing an ice height sentinel.
Hydrography belongs in optional layers pending target rendering and consistency tests.

## Reproducible candidate editions

Exact recipes are in `editions/recipes.json`; every candidate includes all 258 political identities.
Compact uses 110m political/coast/border geometry, selected 50m hydrography, 373 role-aware places,
and existing 1° relief/classes. Standard uses 50m geometry, more 50m hydrography, 1,362 places and
both current elevation levels. Expanded uses full 10m geometry/hydrography and all 7,342 places,
with the same existing relief levels. **No finer raster was generated.**

| Candidate | Country geometries | Rivers / lakes | JSONL layer bytes | Independent gzip bytes | Estimated resident payload |
| --- | ---: | ---: | ---: | ---: | ---: |
| Compact | 177 | 62 / 44 | 2,700,988 | 462,178 | 4,779,162 |
| Standard | 242 | 255 / 308 | 9,866,286 | 2,272,812 | 25,373,551 |
| Expanded | 258 | 1,473 / 1,355 | 54,977,435 | 13,747,570 | 65,864,849 |

JSONL/gzip sizes are measured generated **research outputs**, including rich multilingual build-time
metadata, and exclude existing raster bytes. They are not proposed shipping pack sizes or memory
requirements for a JSON parser. Layer manifests report records, parts, vertices, raw/gzip hashes and
raster references. Resident figures are **estimates for hypothetical fixed-width typed payloads**,
with current terrain caches, not measured allocations for these experimental files.

Vector estimate: 8 bytes/quantized vertex + 16/part + 32/record, plus 24/vertex unit-vector cache.
Place/identity estimates use 40-byte metadata records and one chosen UTF-8 string; they do not
keep the rich multilingual JSON in memory. Source raster bytes, decoded classes and terrain caches
are counted. Exclusions include caps/spatial indexes, per-job geometry, framebuffer/shown/zoom
buffers, font/platform storage, label layout, object/allocator overhead and OS memory. This is a
scenario comparison, **not a recommended OS 9 application partition**.

Per-layer decoded/resident/evictable estimates and file bytes are in `editions.json`. The estimated
largest additional vector-loading temporary is **188,985 / 1,655,639 / 8,859,044** bytes respectively,
assuming compressed input plus a decoded staging copy, beyond the new resident representation.
Replacement may require **old resident + candidate resident + staging**, plus all excluded runtime
buffers. Elevation import also retains its input buffer. A streaming decoder could reduce that
staging assumption, but needs separate format and target evidence. G4 timings were not measured.

Standard's simultaneous 1° and 0.5° terrain caches alone are nearly 18 MB. Expanded's all-resident
geometry is plainly a different memory tier. Do not map file-size improvements directly to frame
speed: 10m country vertices alone grow about 51× over source 110m, and 10m river vertices about 10×
over 50m. Picking, clipping, subdivision and label candidate scans need measured target budgets.
Expanded also exceeds GPLC v1's 4,096-place limit, and GATL v1 has no river/lake layer IDs.
These experimental objects cannot simply be installed into today's runtime.

Monolithic gzip comparisons are **463,605 / 2,276,499 / 13,758,731** bytes. Added layer envelopes
make this prototype slightly larger, so there is no demonstrated monolithic compression benefit.
This is one encoding experiment, not a universal compression verdict. Independent typed packs
permit optional loading, separate limits and easier failure isolation, at the cost of more file and
compatibility bookkeeping. A monolith simplifies atomic installation but can couple residency and
updates. A manifest-driven hybrid (mandatory identity/fallback, independently bounded typed detail,
optional distribution archive) is the recommended hypothesis for #26/#31/#48 to evaluate.

## Resource lifetime and degradation (documentation only)

Current Atlas/Countries/Places/Elevation objects borrow pack bytes. Country/Place scenes cache
vectors but still borrow names and records; `InfoCard` string views borrow that same storage.
Country selection uses pack-local indices, which are not stable across editions. Active render
jobs borrow Scene/TerrainScene and mutable projection workspaces; a terrain token prevents stale
use of a workspace, but cannot keep a destroyed resource alive. Startup currently avoids hot
replacement. Cache teardown or replacement without a quiescence protocol would invalidate these
borrows; this is a future integration risk, not proof of a currently exercised use-after-free.

`ElevationPacks::bytes()` accounts for scene/classes, **not all packed input vectors or UI buffers**.
`initialize()` resets scenes; retained file-vector capacity can remain, and external jobs/views must
not survive reinitialization. Failed imports release candidate bytes after failed add; allocating
input before validation still raises peak memory. Several scene/read paths use vector reserve/resize
with exceptions disabled; a counted source-size guard is not a complete failed-allocation policy.
Some inline palettes are included in reported cache bytes rather than heap measurements. #55
should distinguish physical allocation, logical category and packed embedded storage without double
counting. Frame-sized line/shown/zoom buffers need workload-specific accounting beyond this audit.

Proposed publication: a resource owner holds immutable validated storage and decoded indexes;
handles carry generation plus stable entity ID. A render job pins its resource generation for its
entire lifetime. Cancel/quiesce jobs and close/rebuild borrowed cards before releasing or swapping
storage. Selection stores stable identity and resolves a new pack-local index after publication.
Either reserve old+candidate peak before staging, or explicitly deactivate optional resources at a
safe boundary while retaining the embedded fallback. Allocation failure leaves the old generation
usable. These invariants inform #17/#26/#55/#57; no handle, ledger or eviction code was added.

Degrade optional high-resolution geometry and projected/terrain caches first while preserving
mandatory identity, representative points, provenance and selected important places. Fine polygons
may disappear without removing San Marino from lookup. Mark reduced geographic fidelity explicitly;
do not turn missing coverage into ocean, unknown classes into desert or absent geometry into a
claim that an entity does not exist. Exact degradation order and byte savings need #55/#56 target
measurements rather than adoption of this speculative sequence.

## Licensing and source alternatives

[Natural Earth terms](https://www.naturalearthdata.com/about/terms-of-use/) place the map data in the
public domain; credit is voluntary and retained here. Lock/version each layer independently; the
10m coastline archive identifies itself as **5.0.0-pre9**, a review warning before any production
adoption. One 50m River has null geometry (source record 461); one 110m coast longitude exceeds
180 by 4.42e-7 degrees. Both exceptions are explicit, and other coordinate excursions are rejected.

[Wikidata](https://www.wikidata.org/wiki/Wikidata:Licensing) structured data is CC0, useful for QID
concordances, language names and relationships after snapshotting qualified statements and dates.
Do not import wiki prose under that assumption. Live service availability and changing statements
make unpinned queries unsuitable as the reproducibility boundary.

[GeoNames](https://www.geonames.org/export/) offers daily extracts and country subsets with an
attribution license. It can supplement missing settlements/alternate names, but adds a large,
continuously changing roster and another identity/classification reconciliation task. Its data
license/version and exact extract would need a new source review and pin before use.
[Who's On First](https://whosonfirst.org/docs/licenses/) provides a useful hierarchy/concordance
model, with required license links and source-specific attribution. Its mixed source provenance
makes “one blanket license” inadequate; no WOF geometry was imported.

[HydroRIVERS](https://www.hydrosheds.org/products/hydrorivers) supplies a routed network with
8.5 million reaches and a listed global shapefile download around 544 MB. Its HydroSHEDS license
agreement, required attribution and redistribution provisions require a full text review before
adoption. That network may justify a regional future experiment, but substantially exceeds the
current globe cartography need. [OpenStreetMap](https://www.openstreetmap.org/copyright) adds detailed
waterways and update activity, with ODbL attribution/share-alike obligations for relevant derived
databases. Keep any such ingestion separately reviewed; do not casually merge it into a public-domain
catalog. Neither dataset was downloaded or licensed into these outputs.

## Architecture recommendation and remaining acceptance

Keep the existing builders unchanged. Evolve `atlas/` in small reviewed steps: source locks and
explicit acquisition; source adapters; a revisioned identity/name/relation catalog; editorial rules;
edition recipes; semantic fixtures; measured reports; and, only after contract approval, typed
output writers with compatibility manifests. Build-host GIS tools can later provide independent
polygon/topology oracles without adding any target dependency. Python stdlib suffices for this
prototype. A canonical SQLite/GIS database is an alternative for complex joins, but not justified
as an on-device dependency or mandatory first-stage catalog store.

Recommended sequence: review the political inventory and capital roles; audit duplicate/missing
crosswalks and names; independently verify representative points and coast/lake/river relationships;
compare equal-area coverage rules; decide a bounded typed contract; prototype failure-safe loading;
then benchmark memory and interaction on the G4 under explicit hardware authorization. #24,
#26, #29–#31, #36/#38/#61 and #48/#50 remain the existing integration work, not completed by this memo.

The committed fixture tests parser bounds, invalid coordinates, null alignment, source checksum
failure, capital-role preservation, Unicode/language metadata, stable source identity and unchanged
production pack inventory. Full-source QA checks required identities/places/rivers, canonical-source
ID uniqueness, expected nulls, multipart Fiji and preservation of source capital flags. All failures
are diagnostic; geometry feature reuse and unresolved crosswalks remain warnings. Two complete
edition builds are compared byte-for-byte on the recorded host toolchain.

Still **not established**: complete sovereign/territory/capital inventory, validated Wikidata/M49
joins, correct population definitions/dates for every place, global polygon validity after
quantization, lake-interior/river-mouth connectivity, actual system font coverage, target endian
conformance for a new format, target rendering cost, G4 allocation/peak measurements, finer relief,
or production-ready edition installation. These are honest acceptance gaps in a measured subset,
not reasons to declare #52, #53, #54 or the production migration complete.
