# Coastline provenance

Natural Earth 1:110m coastline, advertised version 4.1.0.

- Dataset: https://www.naturalearthdata.com/downloads/110m-physical-vectors/110m-coastline/
- Source ZIP: https://naturalearth.s3.amazonaws.com/110m_physical/ne_110m_coastline.zip
- Terms (public domain): https://www.naturalearthdata.com/about/terms-of-use/
- Retrieved: 2026-11-01
- ZIP SHA-256: 664449b39070027e882abb295974d182afec18ca21107273d17e9e8bf6f64817

Regenerate with Python standard library only:

```
python3 tools/prepare_map.py /path/to/ne_110m_coastline.zip data/coastline.plnt
```

The asset contains 134 polylines and 5389 points. No GIS runtime is needed.
Long segments are subdivided at no more than two degrees in longitude/latitude,
using the shortest longitude delta across the date line. Projection uses sphere
coordinates rather than drawing in longitude space, avoiding wraparound streaks.

## Country land boundaries

Natural Earth Admin 0 boundary lines, 1:110m, advertised version 5.1.0.
Natural Earth uses de facto boundaries by default; these are a cartographic
reference layer, not a statement about sovereignty.

- Dataset: https://www.naturalearthdata.com/downloads/110m-cultural-vectors/110m-admin-0-boundary-lines/
- Source ZIP: https://naturalearth.s3.amazonaws.com/110m_cultural/ne_110m_admin_0_boundary_lines_land.zip
- Terms: same public-domain terms as above.
- Retrieved: 2026-11-01
- ZIP SHA-256: bb2954981570498a4b4344f8e83fa54caa5858e15e800094112407d2bd2f940d
- Prepared: 333 polylines / 3185 points, same PLNTMAP version 1 format.

```
python3 tools/prepare_map.py /path/to/ne_110m_borders.zip data/borders.plnt ne_110m_admin_0_boundary_lines_land.shp
```

## Wikidata city snapshot

`wikidata/places.json` contains 13 selected city records retrieved on 2026-11-01
from Wikidata's individual-entity JSON interface. The runtime asset is
`places.plnt` (1664 bytes, PLNTPOI version 1).

- Access documentation: https://www.wikidata.org/wiki/Wikidata:Data_access
- License: CC0-1.0 for Wikidata structured data.
- Source endpoint pattern: https://www.wikidata.org/wiki/Special:EntityData/Q90.json
- Runtime asset SHA-256: f443faf613e8e2596bfe6d1b60707360258458196a880729a43eb08867acfa8e
- Runtime asset POSIX cksum: 1115819544, 1664 bytes (Linux and native G4).

Included entities: Q90 Paris, Q84 London, Q60 New York City, Q1297 Chicago,
Q1490 Tokyo, Q85 Cairo, Q3870 Nairobi, Q5465 Cape Town, Q8678 Rio de Janeiro,
Q3130 Sydney, Q1353 Delhi, Q956 Beijing, Q1486 Buenos Aires.

For every record, the JSON snapshot retains entity revision, modification time,
entity/revision URLs, SHA-256 of the downloaded entity JSON, the selected P625
coordinate statement and P1082 population statement including their references,
English label/description, and the English Wikipedia sitelink URL. Wikipedia
article text and images are not copied into this dataset.

Preferred, non-deprecated statements take precedence. Coordinates must refer to
Earth (Q2). Among eligible population statements at the selected rank, the latest
P585 date not later than the snapshot date is chosen. Undated statements are used
only if no later dated eligible statement is available. Dates shown in popovers
are years; full qualifier dates and precision remain in the JSON snapshot.
Population refers to the Wikidata entity's geographic scope; the values are not
normalized into comparable city-proper or metropolitan totals.

Rebuild offline, without downloading:

```
python3 tools/prepare_places.py
```

Refresh on the modern development host:

```
python3 tools/prepare_places.py --fetch
```

The fetch tool uses bounded entity requests and a cache separated by retrieval
date. `--date` labels the actual snapshot date; it does not fetch historical
entities. English runtime strings are length-limited ASCII approximations for
core X11 fonts; the retained JSON preserves original Unicode.
