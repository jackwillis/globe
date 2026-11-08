# Atlas production research (#53)

Build-time experiments only. No new runtime formats, production pack replacements or target-machine
measurements. Start with [the measured baseline](reports/baseline.md) and
[the recommendation and limitations](reports/recommendation.md).

Python's standard library is sufficient. No automatic network access occurs during auditing,
testing or edition generation. Acquire the exact locked Natural Earth archives explicitly:

```sh
python3 atlas/research/sources.py /tmp/globe-atlas-sources --download
python3 atlas/research/sources.py /tmp/globe-atlas-sources
python3 atlas/research/test_audit.py -v
python3 atlas/research/baseline.py --output /tmp/baseline.json
python3 atlas/research/audit.py /tmp/globe-atlas-sources /tmp/globe-atlas-output
python3 atlas/research/hydro_qa.py /tmp/globe-atlas-sources /tmp/hydro-endpoints.json
```

The lock records exact archive URLs, versions observed inside ZIPs, retrieval timestamps, byte
counts and SHA-256. The URLs can change upstream: mismatch is a hard error, never an automatic
lock update. Keep verified archives separately if long-term reproduction matters. Sources are
public-domain Natural Earth; credit is retained voluntarily. No upstream archive or experimental
binary is committed. Existing NOAA/RESOLVE/Wikidata inputs remain governed by `data/SOURCES.md`.

`audit.py` emits three candidate folders, each containing a manifest, typed **research JSONL** layers,
deterministic gzip copies, and a monolithic comparison. These are inspectable experiments, not
formats a Globe executable understands. Country source rings, closure and multipart structure
are retained; quantization clamps the documented source roundoff. Hole containment, winding,
self-intersections and quantization-induced geometry damage are not certified by this tool.

The identity catalog takes 258 Natural Earth 10m admin0 country records, retaining source classification
and sovereign relations even when coarse polygons omit the entity. It is not a definitive sovereign
state inventory. NE and QID concordances remain provisional and names never serve as join keys.
All selected places include original and available localized names. `NAME_ZHT` maps to `zh-Hant`;
other language fields retain their language suffix and exact source field. Generic source names
have unknown language (`und`), not an invented endonym/exonym designation.

Recipes and required cases are committed separately. Semantic checks cover the named microstates,
Greenland/island cases, selected capitals/centers and six named rivers. Parser fixtures cover invalid
coordinates, truncation, null geometry alignment and checksums. Reports distinguish failures,
warnings and accepted source exceptions. They do not establish complete geopolitical or hydrological
correctness. Gzip byte identity is checked on the recorded Python/zlib toolchain; uncompressed JSONL
is the preferred cross-toolchain reproducibility reference.

Repeat the build into a second directory, then compare:

```sh
python3 atlas/research/audit.py /tmp/globe-atlas-sources /tmp/globe-atlas-output-repeat
diff -qr /tmp/globe-atlas-output /tmp/globe-atlas-output-repeat
```

`reports/*.json` are reviewable snapshots from these commands. Generated full catalogs and geometry
are deliberately outside Git. The source reader handles only the pinned SHP/DBF subset; it is not
a general GIS library, topology validator or hardened parser for arbitrary untrusted archives.

For this session, verified archives and the generated edition outputs are also retained in the
owner's ignored `build-atlas-research/sources/` and `build-atlas-research/editions/` directories.
These are local research artifacts, not published releases or installed runtime resources.

## Further semantic research

The [follow-up findings](reports/semantic-followup.md) add targeted revision-pinned Wikidata
concordances, reference-only M49 comparisons, independent geometry/delta checks, per-edition name
fallback profiles, and fresh source reproduction of the raster packs.

```sh
python3 atlas/research/concordance.py /tmp/globe-atlas-sources /tmp/globe-atlas-references --download
python3 atlas/research/concordance.py /tmp/globe-atlas-sources /tmp/globe-atlas-references /tmp/concordance.json
python3 atlas/research/name_profiles.py /tmp/globe-atlas-sources /tmp/name-profiles /tmp/name-profiles.json
python3 atlas/research/test_concordance.py -v
python3 -m venv /tmp/globe-atlas-gis-env
/tmp/globe-atlas-gis-env/bin/python -m pip install --require-hashes -r atlas/research/requirements-research.lock.txt
/tmp/globe-atlas-gis-env/bin/python atlas/research/test_geometry_qa.py -v
/tmp/globe-atlas-gis-env/bin/python atlas/research/geometry_qa.py /tmp/globe-atlas-sources /tmp/geometry-qa.json
/tmp/globe-atlas-gis-env/bin/python atlas/research/river_components.py /tmp/globe-atlas-sources /tmp/river-components.json
```

The optional requirements lock targets the observed CPython 3.14 Linux x86-64 wheel environment;
other build hosts need separately reviewed wheel hashes. No dependency is added to CMake or the
runtime. UN M49 HTML is a privately retained reference, not an edition input or a claimed
public-domain redistribution. Wikidata snapshots are CC0 structured data; revision URLs and exact
hashes are recorded in `references.lock.json`.

For full raster source reproduction (eight conversions, several minutes on this host), use:

```sh
python3 atlas/research/reproduce_rasters.py /tmp/globe-atlas-rebuild \
  --etopo /path/to/ETOPO_2022_v1_60s_N90W180_surface.nc \
  --ecoregions /path/to/Ecoregions2017.zip --lakes /path/to/ne_50m_lakes.zip \
  --python /tmp/globe-atlas-gis-env/bin/python --run --report /tmp/raster-reproduction.json
```

Without `--run`, the command verifies retained outputs/logs and their production matches. The
quarter-degree file is a research output only and is deliberately absent from edition recipes.
