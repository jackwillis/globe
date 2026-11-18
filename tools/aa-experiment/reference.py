#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Independent area oracle; only tiny analytic controls run in this phase.

Deliberately simple reference, not a benchmark or a copy of B's signed-distance filter.
Full-frame scoring and acceleration of this oracle await review/authorization.
"""
import math
import struct
import tempfile
from pathlib import Path


def read_geometry(path):
    path = Path(path)
    if path.stat().st_size > 16 * 1024 * 1024:
        raise ValueError("geometry size bound")
    # Read at most the bound plus one byte even if a file grows after stat().
    with path.open("rb") as source:
        data = source.read(16 * 1024 * 1024 + 1)
    if len(data) > 16 * 1024 * 1024 or data[:8] != b"PEAAAB1\0":
        raise ValueError("geometry envelope")
    pos = 8

    def take(fmt):
        nonlocal pos
        size = struct.calcsize(fmt)
        if pos + size > len(data):
            raise ValueError("truncated geometry")
        value = struct.unpack_from(fmt, data, pos)
        pos += size
        return value

    count, = take("<I")
    if count > 8192:
        raise ValueError("feature bound")
    features = []
    points_total = chains_total = 0
    for _ in range(count):
        identity, color, n = take("<IHI")
        chains_total += n
        if chains_total > 32768:
            raise ValueError("chain bound")
        chains = []
        for _ in range(n):
            closed, points = take("<BI")
            points_total += points
            if closed > 1 or points_total > 262144 or points < 2:
                raise ValueError("point/flag bound")
            geometry = [take("<dd") for _ in range(points)]
            if any(not math.isfinite(v) or abs(v) > 1e6 for p in geometry for v in p):
                raise ValueError("coordinate domain")
            chains.append((geometry, bool(closed)))
        features.append((identity, color, chains))
    if pos != len(data):
        raise ValueError("trailing geometry")
    return features


def contains_chain(points, closed, x, y):
    # Exact consecutive duplicates have no geometric effect, including at open caps.
    p = []
    for q in points:
        if not p or q != p[-1]:
            p.append(q)
    if len(p) < 2:
        return False
    edges = list(zip(p, p[1:]))
    if closed and p[-1] != p[0]:
        edges.append((p[-1], p[0]))
    for (ax, ay), (bx, by) in edges:
        dx, dy = bx - ax, by - ay
        length2 = dx * dx + dy * dy
        projection = (x - ax) * dx + (y - ay) * dy
        cross = (x - ax) * dy - (y - ay) * dx
        if 0 <= projection <= length2 and cross * cross <= 0.25 * length2:
            return True
    joins = p if closed else p[1:-1]
    return any((x - ax) ** 2 + (y - ay) ** 2 <= 0.25 for ax, ay in joins)


def channels(value):
    r, g, b = (value >> 11) & 31, (value >> 5) & 63, value & 31
    return ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))


def reference_pixel(features, background, x, y, samples=64):
    if samples not in (64, 128, 256, 512, 1024):
        raise ValueError("sample schedule")
    sums = [0, 0, 0]
    for sy in range(samples):
        py = y + (sy + 0.5) / samples
        for sx in range(samples):
            px = x + (sx + 0.5) / samples
            color = background
            for _, candidate, chains in features:
                if any(contains_chain(p, closed, px, py) for p, closed in chains):
                    color = candidate
            rgb = channels(color)
            for i in range(3):
                sums[i] += rgb[i]
    rgb = [value / (samples * samples * 255) for value in sums]
    q = [math.floor(value * maximum + 0.5) for value, maximum in zip(rgb, (31, 63, 31))]
    return rgb, (q[0] << 11) | (q[1] << 5) | q[2]


def self_test():
    line = ([(3.5, 10.5), (19.5, 10.5)], False)
    white = [(0, 0xffff, [line])]
    rgb, q = reference_pixel(white, 0, 3, 10)
    assert rgb == [0.5, 0.5, 0.5] and q == 0x8410
    assert reference_pixel(white, 0, 4, 10) == ([1, 1, 1], 0xffff)
    rev = [(0, 0xffff, [(line[0][::-1], False)])]
    assert reference_pixel(rev, 0, 3, 10) == (rgb, q)
    # Coincident half-pixel features: last writer blue occupies the same samples as red.
    # Compositing averaged alpha would incorrectly leave a red contribution.
    colored = [(0, 0xf800, [line]), (1, 0x001f, [line])]
    assert reference_pixel(colored, 0, 3, 10)[0] == [0, 0, 0.5]
    assert reference_pixel(colored[::-1], 0, 3, 10)[0] == [0.5, 0, 0]
    assert reference_pixel([(0, 0xffff, [line, line])], 0, 3, 10) == (rgb, q)
    assert not contains_chain([(3.5, 3.5), (3.5, 3.5)], False, 3.5, 3.5)
    # Independent circle-membership area and symmetry control at the declared refinement level.
    n = 256
    quadrants = []
    for ix, iy in ((0, 0), (0, 1), (1, 0), (1, 1)):
        hits = sum((ix + (x + .5)/n - 1) ** 2 + (iy + (y + .5)/n - 1) ** 2 <= .25
                   for y in range(n) for x in range(n))
        quadrants.append(hits)
    assert len(set(quadrants)) == 1
    assert abs(sum(quadrants)/(n*n) - math.pi/4) < 1/1024
    valid = (b"PEAAAB1\0" + struct.pack("<I", 1) + struct.pack("<IHI", 7, 0xffff, 1)
             + struct.pack("<BI", 0, 2) + struct.pack("<dddd", 3.5, 10.5, 19.5, 10.5))
    with tempfile.TemporaryDirectory(prefix="pe-aa-reader-test-") as directory:
        path = Path(directory) / "fixture"
        path.write_bytes(valid)
        assert read_geometry(path) == [(7, 0xffff, [line])]
        invalid = [valid[:-1], valid + b"x", b"BADMAGIC" + valid[8:],
                   b"PEAAAB1\0" + struct.pack("<I", 8193),
                   valid[:22] + struct.pack("<BI", 2, 2) + valid[27:],
                   valid[:-8] + struct.pack("<d", float("nan"))]
        for data in invalid:
            path.write_bytes(data)
            try:
                read_geometry(path)
            except ValueError:
                pass
            else:
                raise AssertionError("invalid geometry accepted")
        with path.open("wb") as out:
            out.truncate(16 * 1024 * 1024 + 1)
        try:
            read_geometry(path)
        except ValueError:
            pass
        else:
            raise AssertionError("oversized geometry accepted")
    print("PASS: independent rectangle, reversal, union, colored last-writer, disc and bounded-reader controls; no geographic scoring")


if __name__ == "__main__":
    self_test()
