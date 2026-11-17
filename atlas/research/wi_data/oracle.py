#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Independent brute-force oracles on the raw float source records.

Independence: these use the nonzero winding rule on unquantized floats and the source's own
closed day-range dates. The models use even-odd on int32 microdegrees and half-open day numbers.
"""
import datetime
import math

def winding(rings, x, y):
    total = 0
    for ring in rings:
        n = len(ring)
        for i in range(n):
            x0, y0 = ring[i]
            x1, y1 = ring[(i + 1) % n]
            if y0 <= y:
                if y1 > y and (x1 - x0) * (y - y0) - (x - x0) * (y1 - y0) > 0:
                    total += 1
            elif y1 <= y and (x1 - x0) * (y - y0) - (x - x0) * (y1 - y0) < 0:
                total -= 1
    return total

def inside(rings, x, y):
    return winding(rings, x, y) != 0

def county_at(counties, x, y, date):
    """Source semantics: START_DATE and END_DATE are both inclusive days; proposals are not counties."""
    return sorted((c['id'], c['start']) for c in counties
                  if c['cnty_type'] == 'County' and c['start'] <= date <= c['end'] and inside(c['rings'], x, y))

def tract_at(tracts, x, y):
    return sorted(t['geoid'] for t in tracts if inside(t['rings'], x, y))

def bbox_intersect(a, b):
    return not (a[2] < b[0] or b[2] < a[0] or a[3] < b[1] or b[3] < a[1])

def raw_bbox(rings):
    xs = [p[0] for r in rings for p in r]
    ys = [p[1] for r in rings for p in r]
    return (min(xs), min(ys), max(xs), max(ys))

def tracts_near(tracts, lake, pad=0.0):
    box = raw_bbox(lake['rings'])
    box = (box[0] - pad, box[1] - pad, box[2] + pad, box[3] + pad)
    return sorted(t['geoid'] for t in tracts if bbox_intersect(raw_bbox(t['rings']), box))

def _orient(a, b, c):
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])

def _cross(ring, i, j):
    n = len(ring)
    a, b, c, d = ring[i], ring[(i + 1) % n], ring[j], ring[(j + 1) % n]
    return _orient(a, b, c) * _orient(a, b, d) < 0 and _orient(c, d, a) * _orient(c, d, b) < 0

def simple_ring_quadratic(ring):
    """Reference: False if two non-adjacent edges properly cross. O(n^2)."""
    n = len(ring)
    return not any(_cross(ring, i, j) for i in range(n) for j in range(i + 2, n) if not (i == 0 and j == n - 1))

def simple_ring_grid(ring):
    """Same answer, near-linear: edges are binned into a uniform grid and only co-located pairs are tested."""
    n = len(ring)
    xs, ys = [p[0] for p in ring], [p[1] for p in ring]
    x0, y0 = min(xs), min(ys)
    g = max(1, int(n ** 0.5))
    sx, sy = (max(xs) - x0) / g or 1.0, (max(ys) - y0) / g or 1.0
    cell = lambda v, o, s: min(g - 1, int((v - o) / s))
    cells = {}
    for i in range(n):
        a, b = ring[i], ring[(i + 1) % n]
        for cx in range(cell(min(a[0], b[0]), x0, sx), cell(max(a[0], b[0]), x0, sx) + 1):
            for cy in range(cell(min(a[1], b[1]), y0, sy), cell(max(a[1], b[1]), y0, sy) + 1):
                cells.setdefault((cx, cy), []).append(i)
    seen = set()
    for members in cells.values():
        for k, i in enumerate(members):
            for j in members[k + 1:]:
                if j < i + 2 or (i == 0 and j == n - 1) or (i, j) in seen:
                    continue
                seen.add((i, j))
                if _cross(ring, i, j):
                    return False
    return True

def simple_ring(ring):
    return simple_ring_quadratic(ring) if len(ring) <= 400 else simple_ring_grid(ring)
