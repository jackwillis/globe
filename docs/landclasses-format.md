# One-degree land classes (GL-34, v1)

`data/landclasses-1deg.ggrd` uses the existing GGRD v1 compressed-byte-grid container and
`grid_codec` decoder unchanged. This typed resource fixes the geographic interpretation:
360 columns x 180 rows, WGS84, edges -180..180 longitude and -90..90 latitude, row zero south,
column zero west, one byte per cell. It contains no elevation or ice flag and changes no GELV
bytes. A future incompatible geographic layout/class mapping needs a separately versioned typed
resource; it must not silently reinterpret this file.

| Byte | Meaning |
| --- | --- |
| 0 | Unknown (ocean, missing coverage, source N/A rock/ice) |
| 1 | Desert / xeric shrubland |
| 2 | Forest (including taiga and mangroves) |
| 3 | Grass / savanna / shrub / Mediterranean woodland |
| 4 | Tundra |

RESOLVE biome numbers 1–6 and 14 map to forest; 7–10 and 12 to grass/shrub; 11 to tundra;
13 to desert. The N/A record maps to unknown regardless of its numeric biome. There is no
crop/urban claim and no new Greenland ice handling. The height grid still decides land/water;
a class never turns a negative-height cell into land. Antarctica's existing land-latitude override
remains white in all looks. This grid represents ecological regions, not measured current land use.

The builder verifies the exact source archive SHA-256 and CRS, DBF schema/count and all license
fields; the source and license were checked first in [SOURCES](../data/SOURCES.md). It scans polygon
edges into sampled latitude intersections, filling cell centers by even-odd pairs, retaining holes,
longitude unwrapping and south-first rows. Horizontal edges are ignored; lower/left endpoints are
included and upper/right excluded. Ring coordinates just outside +/-180 or +/-90 by at most 1e-9
degrees are clamped: the source has 11 longitudes of 180.0000000000001 in record 134. Larger
excursions are rejected. This is floating-point boundary repair, not a coastal buffer.

Smallest ECO_ID wins if regions overlap at a sampled center; none do in this build. Unknown
centers remain unknown; there is no nearest-biome fill over coasts, lakes or gaps. Source: 846
classified ecoregions and one N/A record. Class counts in byte order: 49,948 / 2,496 / 6,462 /
3,879 / 2,015 (counts of angular cells, not surface-area percentages).

Two complete builds produce the same **3,607-byte** GGRD, SHA-256
`52f39e0b1623ccde92462c591da8335649064ff3d6e892b671cf02f3519070cc`.
Decoded payload: 64,800 bytes. Modern Python standard library only:

```
python3 tools/landclass-builder/build_landclasses.py Ecoregions2017.zip landclasses-1deg.ggrd
```

The runtime validates the GGRD version, flags, exact lengths, shape, stored/decoded CRCs and stream;
requires exactly 360x180 and class bytes 0..4; allocates only the bounded decoded array at startup.
`ElevationPacks` owns it and includes its 64,800 bytes in the aggregate terrain budget. Each terrain
cache samples classes once at its own cell centers (including optional higher-resolution elevation)
and keeps one byte per elevation cell. Two five-class x 256-color tables add 5,120 bytes per cache.
No class lookup, decode or allocation is performed per pixel. Existing height shading interpolates
indices within the cell's selected class table, so hue does not blend across class boundaries.

Data remains **CC BY 4.0**, with attribution, source/license links and adaptation notice in
`data/LANDCLASS-NOTICE.txt`, also packaged on the floppy. Software remains GPL-3.0-or-later.
The source ZIP is never committed. `tests/data/landclass-invalid.ggrd` is an independent synthetic
GGRD with correct checksums/shape but class 5 everywhere, testing semantic rejection after decoding.
