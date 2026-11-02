# Polyline culling pass — 2026-11-01

Each Scene now caches a conservative spherical cap per atlas polyline: normalized mean direction
and sine of the maximum vertex angle. Caps of 90 degrees or more, or a degenerate mean, disable
culling. Smaller caps contain the shorter great-circle arcs as well as their endpoints. A frame
skips the polyline only when `dot(axis, camera.forward) < -sin_radius - 1e-12`, retaining tangent
geometry and a rounding margin. All square roots occur at Scene creation, not in this frame test.

Scene storage increases from 226,968 to 241,912 bytes (+14,944 bytes; 467 caps at 32 bytes each).
There are two up-front allocations, no frame-path allocation. G4 Scene construction was 15.1956 ms
in the culling run versus 13.4267 ms in the preceding word-store run (one run each).

Same `globe-bench --convert --repeat 2 tests/data/bench_drag.txt`, 640x480, 750 frames,
G4 cross GCC 12.5.0 `-O2 -static`; word-store packing and line changes present in both runs:

| G4 renderer | Mean | p50 | p95 |
| --- | --- | --- | --- |
| No cap culling | 18.6814 ms | 18.9170 ms | 21.6558 ms |
| Cap culling | 17.7811 ms | 18.3264 ms | 21.0044 ms |

Raw files: `pass-20261101-g4-pack-words.json`, `pass-20261101-g4-cull.json`. The modest difference
is one run each, not a robust statistical estimate. Visible segment and rasterized line counts
are identical; checksum remains `41c7adb508763323` on host and G4. Combined host render/pack
means were 0.2898/0.1890 ms; combined G4 means were 17.7811/6.9910 ms. Nothing measures real-display
submission or perceived interaction latency.

Tests compare culled, unculled cached, and uncached frames over 40 deterministic random cameras
including high zoom, plus every layer combination in a synthetic atlas. They require a nonzero
culling count. Considered-segment counts may decrease; visible/rasterized counts must agree.
Disabled-culling, undersized-radius, and wrong-hemisphere mutants were rejected. Golden frames
and replay pins are unchanged; render/replay tests pass on G4. Retro68 PowerPC/68k compile only.
