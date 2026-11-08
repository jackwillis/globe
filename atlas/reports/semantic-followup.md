# Source concordance and semantic QA follow-up (#53)

November 8, 2026. This continues the existing atlas-production research. It does not implement
#36/#38/#61, choose runtime text encoding, change production packs, repair geometry, or add a memory
ledger. The current descriptions/comments of #36, #38 and #61 were reread after their November 8
updates: installed system fonts, no bundled fonts/glyphs, platform metrics, and consistent geographic
meaning rather than pixel-identical text remain the boundary.

## Political inventories and identity concordances

The official [M49 reference](https://unstats.un.org/unsd/methodology/m49/overview/) was acquired and
hashed for an offline comparison. Its 248 statistical area codes are not a sovereign-state list.
The current 258-record Natural Earth country roster represents 237 distinct M49 ISO codes; the
298-record map-unit roster represents 247. Their union represents all 248 codes in this snapshot.
That does **not** establish a one-to-one identity mapping or complete geographic coverage.

French Guiana, Guadeloupe, Martinique, Mayotte, Réunion, Christmas Island, Cocos Islands, Bouvet,
Svalbard and Tokelau illustrate why the country layer alone is an inadequate canonical inventory.
Conversely, the collective US Minor Outlying Islands code occurs in the country roster but is not
a collective map-unit code. Some codes group multiple representations. Jan Mayen's map-unit fields
also illustrate differing code conventions. Preserve source layer/feature/relationship context;
do not deduplicate country and map-unit records merely because an ISO or NE_ID matches.

There were no disagreements between valid three-digit NE UN codes and matched M49 codes in this
comparison. Negative/missing NE code sentinels are reported separately from conflicts. The join
never guesses an ISO identity from a name. The raw UN HTML/table is **not committed or emitted into
editions**: [UN website terms](https://www.un.org/en/about-us/terms-of-use) are restrictive, and this
research does not assert a broad redistribution license for that table. The source lock retains
URL/hash/retrieval information, and the report contains aggregate comparisons and NE-derived
exceptions. Any production adoption requires its own source/license decision.

Eighteen targeted Wikidata entities were acquired as CC0 structured-data snapshots, with revision,
modified date, source URL, revision URL and SHA-256. All eighteen revision-specific URLs were fetched
again into a separate directory and reproduced the pinned bytes. This follows the documented
[entity/revision access mechanism](https://www.wikidata.org/wiki/Wikidata:Data_access#Revisions_and_caching),
without a bulk query or fuzzy-name merge.

`concordance.json` compares 28 NE place/map-unit records against those targeted snapshots and
retains best-rank coordinate/country/class/capital claims and dated population statements. A
25 km coordinate discrepancy is a **review heuristic**, not authority to replace a source point.
Especially strong review candidates include:

| NE record | Attached QID's current identity | Coordinate separation |
| --- | --- | ---: |
| Pyongsan | Q18808, Pyongyang | 93.8 km |
| Ascension, Bolivia | Q2450325, Ascensión in Mexico | 7,076 km |
| Jianmen | Q71323, Jingmen | 100.1 km |
| Bose | Q92381, Baicheng in Jilin | 2,821 km |
| Spratly map unit | Q43296, Wake Island | 5,521 km |

These findings reinforce the rule against deduplicating by QID alone. Kuta/Denpasar and
Sakarya/Adapazarı need concept-boundary review even where coordinates are close. Seychelles is an
important counterexample to a simplistic distance gate: an archipelago label point and Wikidata's
representative coordinate differ by hundreds of kilometers without proving a wrong entity.
No replacement QIDs or coordinates were invented or written into an edition.

## Independent geometry checks

An optional, isolated **build-host** environment pins Shapely 2.1.2, GEOS 3.13.1, NumPy 2.5.4 and
h5py 3.16.0. Wheel hashes/URLs are recorded. Shapely/GEOS are independent planar geometry oracles,
not new runtime dependencies. There is no `make_valid`, buffer, snapping or silent source repair.
The pinned package [licenses](https://pypi.org/project/shapely/2.1.2/) are recorded; no library
binaries ship with Globe.

| Polygon source | Invalid source records under planar GEOS | Invalid after microdegree experiment |
| --- | ---: | ---: |
| 110m countries | 0 | 2 |
| 50m countries | 0 | 0 |
| 10m countries | 1 (Egypt) | 1 |
| 50m lakes | 0 | 0 |
| 10m lakes | 2 | 2 |

The 10m lake cases are an unnamed record and Bratsk Reservoir. A much coarser 360/65536-degree
polygon precision experiment produces 101 problematic 10m country records and 137 problematic
10m lake records; it is not a proposed polygon format. Closure, parts and holes matter as much
as total vertices.

A direct check of the **actual committed GPOL integers and explicit hole-parent metadata** also
finds planar self-intersections in the USA and Sudan records. This corroborates the 110m
microdegree experiment, but does not prove that the existing spherical picker gives a wrong result.
The strict parser and current runtime tests still pass. The findings warrant an independent
spherical/landmark review before any pack regeneration; no production bytes were changed.

The point/country oracle first reconciles source codes. For example, places use `SSD`, while the
country layer's ADM0 code is `SDS`; this is a documented ISO alias, not a coordinate error. A
territory-to-country geometry association through a map unit is tracked separately from identity
equality. After those distinctions, the 10m comparison yields 7,279 expected geometry-code hits,
60 no-hits and three other-code-only hits among 7,342 places. Invalid Egypt geometry is excluded
from this oracle, so **42 of those no-hits** reflect that exclusion rather than missing geography.

The three remaining other-code cases are Mucusso (Angola assignment, point in Namibia geometry),
Baykonur (Kazakhstan assignment, separate leased-area geometry) and Nicosia (Cyprus assignment,
point in the source's buffer-zone geometry). Their meanings differ; none is automatically
reassigned. Coarser sources produce substantially more coastal/border disagreements. Source country
label points also sometimes lie outside the selected geometry and cannot serve as guaranteed
interior/picking anchors. This is Cartesian lon/lat evidence with explicit polar/dateline limitations.

## Rivers, deltas and lake coherence

The earlier main-stem endpoint distances were warnings requiring a network check, **not evidence
of missing Nile or Danube mouths**. GEOS noding plus an undirected component diagnostic now shows
separately named Rosetta/Damietta and Danube delta branches linking named main stems toward shore.
Each of the six required systems has at least one near-shore component candidate in full 50m,
full 10m, and the Compact/Standard rank-filtered recipes. Other pieces of the same source QID
remain disconnected in this diagnostic, so complete routing connectivity is not established.

`river-components.json` records names and components for all four profiles. Endpoint keys use a
microdegree quantum; planar intersections can join crossings and near coordinates, and there is
no inferred flow direction. It is a cartographic QA tool, not a hydrological model.

A further classification problem emerged: filtering `FEATURECLA=Canal` does **not** remove all
artificial waterways. Suez, Panama, Ismailiya and other named canals have `River` or
`Lake Centerline` records in these sources. The report explicitly lists retained canal-name hints.
A future catalog needs separate physical-origin/seasonality and geometry-role fields, rather than
using one source classification as unquestioned truth. The current recipes preserve source classes.

At 50m, 41 of 113 lake-centerline records have none of three per-part interior samples inside the
50m lake polygons; 42 have all samples inside. At 10m, those figures are 5/259 and 245/259,
respectively (two invalid lake geometries are excluded). This can reflect missing smaller lake
polygons, generalized alignment, or source segmentation. Sample inclusion does not certify the
whole line, and these diagnostics do not justify drawing every centerline as a visible river.

## International names, encoding fallbacks and edition impact

The data-only name experiment retains exact Unicode source names, source IDs and language-field
provenance. A clearly labeled **English display candidate** uses NFC `NAME_EN` where available,
otherwise the source name with language `und`. This is an experiment, not a universal canonical
name, endonym/exonym policy, or final runtime encoding decision.

For a constrained profile, an exactly encodable name is retained. If it cannot be encoded, the
source's printable `NAMEASCII` is proposed as a **lossy, review-required** fallback; no general
NFKD/diacritic stripping or guessed transliteration is applied. All original strings and identities
survive. A future platform must still check installed-font glyph coverage and measure the actual
chosen string through #36/#61's interface. Codec success alone establishes neither glyph coverage
nor shaping support.

| Candidate | Places | Mac Roman fallback proposals | UTF-8 preferred text bytes | Mac Roman candidate text bytes |
| --- | ---: | ---: | ---: | ---: |
| Compact | 373 | 1 | 3,057 | 3,035 |
| Standard | 1,362 | 22 | 11,254 | 11,100 |
| Expanded | 7,342 | 123 | 61,968 | 60,999 |

All fallback proposals in this source sample have a printable source ASCII candidate, but they
remain unapproved editorial choices. Compact's case is Chișinău → Chisinau. Standard includes
Łódź → Lodz, İzmir → Izmir, Ísafjörður → Isafjordur and Whangārei → Whangarei. Reykjavík retains
its diacritic in Mac Roman. The 123 Expanded count differs from the earlier 101 because this
experiment tests the localized English field, not only generic source `NAME`.

Keeping both preferred UTF-8 and constrained text, plus an illustrative 20-byte metadata record
(two offsets, two lengths, profile/loss flags), costs about **13,552 / 49,594 / 269,807 bytes** for
these selected settlement names. Those are name-payload scenarios, not an approved format or an
addition to the resident edition estimates without accounting for existing name storage. Country,
physical-feature and optional localized names add further bytes. String costs are modest beside
terrain caches; this provides no rationale for discarding source-name fidelity or bundling glyphs.
Repeated visible strings remain separate identities and are explicitly reported, not deduplicated.

## Actual raster reproduction and memory boundary

The already-cached NOAA and RESOLVE archives were found and verified by the existing builders.
Two fresh conversions each reproduce the committed 1° GELV, 0.5° GELV and class GGRD byte-for-byte.
This supersedes the earlier pass's limitation that those sources had not yet been reconverted.
`raster-reproduction.json` records commands, hashes, sizes, host elapsed time and process peak RSS.
Process RSS describes Python conversion, **not** runtime allocations or G4 memory.

Two additional 0.25° conversions agree on a **1,037,352-byte** GELV, SHA-256
`d162426b67cfd417b176b45b6fa8c6a20b9b15f0b02fc0c06322d2e4a0905e34`.
It is an extra research candidate, not a changed Expanded recipe. The current strict GELV parser
accepts it, but the existing host terrain-cache creation rejects its **57,132,961-byte** requirement
against the **33,554,432-byte** guard. The 1°/0.5° caches still create successfully. This is concrete
evidence that a roughly 1 MB packed raster can exceed the current decoded/workspace budget.
No guard, ledger, loader or partition setting was changed, and no G4 claim is made.

## Reproduction and remaining decisions

The standard-library concordance/name scripts require explicit pinned reference acquisition; normal
edition generation still makes no network calls. Optional geometry tools use the separately hashed
research requirements. The source-reproduction driver only writes to a research output directory.
See `atlas/README.md` for commands and retained outputs.

New fixtures cover English-table selection, sentinel code handling, duplicate codes, Wikidata rank
and globe selection, antimeridian distances, faithful/explicit fallback behavior, out-of-order holes,
split-dateline islands, collapsed tiny rings, and ISO aliases versus cartographic parent relationships.
The current runtime suite is independent of these optional GIS dependencies.

Open work remains editorial: review source-scoped political identities and anomalous QIDs; distinguish
capital roles and dated claims; review fallback strings; choose reliable per-geometry representative
points; resolve planar/spherical geometry findings; and establish river/lake coherence criteria.
Any production adoption still needs a reviewed typed contract, bounded loading and authorized target
measurements. No issue or epic is declared complete by these research results.
