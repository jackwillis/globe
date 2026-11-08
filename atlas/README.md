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
