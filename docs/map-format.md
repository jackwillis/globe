# PLNTMAP version 1

All integers are unsigned 32-bit big-endian. No native structs are serialized.

Header: 8 bytes `PLNTMAP` plus NUL, version (1), polyline count, total point count.
For each polyline: point count, then pairs of longitude and latitude integers.
Longitude is (degrees + 180) * 1,000,000; latitude is (degrees + 90) * 1,000,000,
rounded to nearest integer by Python. Ranges are 0..360000000 and 0..180000000.
Each polyline must contain at least two points. There are no implicit links
between polylines, and closure exists only when endpoints are repeated.

The loader rejects unsupported versions, truncated or trailing bytes, counts
inconsistent with the header, invalid coordinate ranges, more than 100000 lines,
or more than 1000000 points. Memory is bounded to about 25 MB on a 32-bit target.
Decoded sphere coordinates are retained in memory. The caller must free a loaded
map before reusing its Map object with map_load.

Tests include a handcrafted valid big-endian file, little-endian rejection,
truncation, trailing bytes, coordinate bounds and allocation/count limits.
