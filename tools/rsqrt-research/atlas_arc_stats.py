#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Angular length of every segment in a GATL atlas (docs/atlas-format.md), i.e. the arcs draw_arc is given.
usage: atlas_arc_stats.py data/atlas.gatl"""
import math, struct, sys
d = open(sys.argv[1], 'rb').read()
P, N, _T = struct.unpack_from('<III', d, 8)
tab = [struct.unpack_from('<BBH', d, 24 + 4 * i) for i in range(P)]
off = 24 + 4 * P
pts = [struct.unpack_from('<HH', d, off + 4 * i) for i in range(N)]
def vec(p):
    lon = math.radians(p[0] * 360 / 65536 - 180); lat = math.radians(p[1] * 360 / 65536 - 90)
    return (math.cos(lat) * math.cos(lon), math.sin(lat), math.cos(lat) * math.sin(lon))
i = 0; angs = []
for _layer, _z, c in tab:
    for k in range(1, c):
        a, b = vec(pts[i + k - 1]), vec(pts[i + k])
        angs.append(math.degrees(math.acos(max(-1.0, min(1.0, sum(x * y for x, y in zip(a, b)))))))
    i += c
angs.sort(); n = len(angs)
print(f"segments {n}  max {angs[-1]:.6f} deg  p99.9 {angs[int(.999*n)]:.3f}  p99 {angs[int(.99*n)]:.3f}  median {angs[n//2]:.3f}")
print(f"over 10 deg: {sum(a > 10 for a in angs)}  over 20 deg: {sum(a > 20 for a in angs)}  over 33 deg: {sum(a > 33 for a in angs)}  over 90 deg: {sum(a > 90 for a in angs)}")
print(f"min l2 at the longest arc (cos^2(phi/2)): {math.cos(math.radians(angs[-1]) / 2) ** 2:.12f}")
