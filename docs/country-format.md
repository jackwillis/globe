# GPOL v1 and spherical picking

All integers little-endian; coordinates signed int32 microdegrees (longitude then latitude).
Header 32 bytes: magic GPOL, uint16 version 1 and flags 0; uint32 country count, ring count,
point count, text bytes, reserved 0, CRC at byte 28. CRC covers bytes 0..27 then 32..EOF.
Limits: 4096 countries, 65536 rings, one million points, one MiB text; all counts nonzero.
Exact length is header + 16*country count + 16*ring count + 8*point count + text bytes.

Country records: uint32 first ring, ring count, name offset, name length. Country ring ranges and
name ranges partition their tables contiguously. Names contain 1..95 printable ASCII bytes,
without terminators. Ring records: uint32 first point, point count, hole flag (0/1), parent.
Point ranges partition the point table; each ring has at least three vertices. An exterior's
parent is itself. A hole's parent is an earlier exterior in the same country. Coordinates must
be within longitude +/-180 and latitude +/-90 degrees. Closing vertices are implicit.

The modern builder preserves shapefile parts and hole relationships, removes duplicate closing
vertices, and gives all rings positive signed spherical area (the chosen interior is less than
half the sphere). Runtime picking caches unit vectors at load, uses signed tangent winding,
subtracts holes from their parent exterior, and checks minor-arc boundaries explicitly. Exterior
boundaries are included and hole boundaries excluded; shared boundaries choose source record
order. Antipodal winding must remain signed: taking its absolute value selects the wrong side.

The strict zero-copy decoder checks structure, ranges, text, CRC and exact size without allocating.
It does not certify arbitrary ring geometry or repair self-intersections. The builder's source
and orientation convention are part of the contract. CountryScene allocates up to 24 MB of unit
vectors once; picking allocates nothing and is a click operation, not a frame loop.

The core returns an InfoCard of bounded string views into stable data/literals. X11 displays a
separate scrolling window; wheel, arrows, Page Up/Down and its hand-drawn scrollbar work. A click
without a camera-changing drag selects a country. Double-click zoom remains available. The window
can be resized or closed; it follows the main window's right edge on main resize.

Tests cover synthetic antimeridian/polar/hole rings, exact boundaries, antipodal rejection,
truncations, every-byte corruption, repaired-CRC structural errors and actual world samples.
CRC-disabled, absolute-winding and hole-disabled mutants fail. Builder output reproduces the
fixture and the pinned real pack. G4 results and host suite results are recorded in the handoff.
