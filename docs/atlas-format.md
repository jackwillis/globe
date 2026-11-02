# Atlas format (GATL version 1)

The bundled coarse atlas: world coastlines and land borders as polylines. Fixed little-endian,
no native structs, no alignment requirements, every field read byte by byte
(`globe/bytes.hpp`). Written by `tools/atlas-builder/build_atlas.py`; decoded by `src/data`
(step 3). Deterministic: the same source files always give the same bytes.

## Layout

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 4 | magic `GATL` |
| 4 | 2 | version, 1 |
| 6 | 2 | reserved, must be 0 |
| 8 | 4 | polyline count `P` |
| 12 | 4 | total point count `N` |
| 16 | 4 | provenance text length `T` in bytes |
| 20 | 4 | CRC-32 (IEEE 802.3, as zlib) of every byte from offset 24 to the end |
| 24 | 4 * P | polyline table: `u8 layer`, `u8 0`, `u16 point_count` |
| 24 + 4P | 4 * N | points, all polylines in table order: `u16 lon_q`, `u16 lat_q` |
| 24 + 4P + 4N | T | provenance text, UTF-8, no NUL bytes |

Total size is exactly `24 + 4P + 4N + T`; anything shorter or longer is invalid.

## Fields

- **layer:** 1 = coastline, 2 = land border. Other values are invalid in version 1.
- **point_count:** at least 2 per polyline. The sum over all polylines equals `N`.
  There is no implicit closure; a ring repeats its first point.
- **Quantization:** one step is 360/65536 degrees (about 0.0055 degrees, roughly 600 m at the
  equator). `lon = lon_q * 360/65536 - 180`, `lat = lat_q * 360/65536 - 90`.
  `lat_q` is 0..32768. `lon_q` is 0..65535; longitude +180 is stored as 0 because it is the same
  point on the sphere as -180.
- **Original vertices only.** Segments are not pre-subdivided. Consumers draw each segment as the
  shorter great-circle arc and subdivide at run time (`subdivide_arc`). Consecutive vertices that
  quantize to the same point are collapsed by the builder.
- **Limits** (enforced by the builder and by the decoder): at most 65536 polylines, 65535 points
  per polyline, 1,000,000 points in total, provenance text at most 4096 bytes.
- **Provenance text:** source names, versions, URLs, source-file SHA-256, license and caveats,
  generated from the inputs. It contains no timestamps, so the file stays reproducible.

## Measured (2026-11-01, Natural Earth 110m)

| | Polylines | Points |
| --- | --- | --- |
| Coastline | 134 | 5,127 |
| Borders | 333 | 2,993 |

Total 35,068 bytes: 32,480 of point data, 2,628 of table, 24 of header, the rest provenance
text (about 930 bytes). gzip -9 gives 32,380 bytes, so the data is already high-entropy.
Delta coding with zigzag varints was estimated, not implemented: point data would shrink from
32,480 to about 25,800 bytes (-20%, about 6.7 KB overall). That does not yet justify a more
complex decoder; revisit if the floppy experiment is short of space.

## Not in version 1

Delta or varint coding (measure first; see the size report), multiple detail levels and tiles
(M2/M3), labels, and feature attributes. A new layout gets a new version number; decoders reject
versions they do not know.
