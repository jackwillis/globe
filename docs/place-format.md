# GPLC v1 and v2 places

All integers little-endian. The 24-byte header contains magic GPLC, uint16 version 1 or 2 and flags 0,
uint32 count (1..4096), text bytes (1..1048576), reserved 0 and CRC at byte 20. CRC covers bytes
0..19 then 24..EOF. Exact size: 24 + 24*count + text bytes.

Each 24-byte record contains uint32 Wikidata Q number, int32 longitude and latitude in microdegrees,
uint32 name offset, name length and a final uint32. In v1 that last field is reserved 0. In v2 it is the
population tier, 0..3 (0 largest): at least 10,000,000 is tier 0, 5,000,000 tier 1, 2,500,000 tier 2,
and anything smaller down to the 1,000,000 floor is tier 3. A reader reports tier 3 for every v1 place.
The tier is a display rank for choosing which names to draw by zoom, not a population or a ranking claim. Q numbers are positive and strictly increasing.
Coordinates are within +/-180 / +/-90 degrees. Names partition the text table contiguously;
each is 1..95 printable ASCII bytes, without a terminator. Labels are transliterated for classic
fonts. Decoder validation is bounded, zero-copy and allocation-free.

The bundled pack is v2 and contains 304 world cities (9,641 bytes; tiers 0 to 3 hold 27, 43, 43 and 191; the cap is 4,096 places). Selection rule:
Wikidata instances of city or big city with coordinates, an English label and a largest population
statement of at least 1,000,000, at most 30 per country (by population, ties by lower Q number) so that
China's 252 qualifying prefecture-level cities do not supply half the pack. Population itself is not stored
in the pack, only its tier, and neither is a ranking: definitions differ between city proper, prefecture and urban area.
The retained response (`data/wikidata-cities-large-20261102.json`) and exact query
(`tools/places-builder/cities-large.sparql`) are pinned; the builder checks the response hash, skips
the inspected Q3139 (Alpes-Maritimes, a department) and transliterates labels to ASCII, failing on any
character it cannot map. The earlier 19-city starter is kept as a test fixture
(`tests/data/cities-starter19.gplc`). Multiple coordinates for one Q number are resolved
deterministically by tuple order. See data/SOURCES.md for license, retrieval and hashes.

Unit vectors are cached once. Settled X11 bands draw a small white square with a black outline.
Click within six pixels to identify the nearest visible city; ties choose the lower Q number.
Country selection is the fallback. Both use the same scrolling information window. Moving views
stay outlines only; the benchmark's terrain pins exclude this optional decoration.

Tests cover front/back selection and markers, bounds, malformed/repaired-CRC records, truncations,
every-byte corruption and Python/C++ fixture agreement. Backface-picking, backface-drawing and
non-strict-Q-order mutants fail. GCC sanitizer and clang suites pass 28/28; G4 unit test prints ok.
Retro68 PowerPC and 68k compile checks pass. X11 layout was inspected on a private virtual display;
real display behavior and OS 9 execution remain owner acceptance.
