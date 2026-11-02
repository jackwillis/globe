#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Tests for tools/atlas-builder/build_atlas.py and the committed atlas files.

    python3 tests/test_atlas_builder.py                  run the tests
    python3 tests/test_atlas_builder.py --write-fixture  regenerate tests/data/tiny.gatl
"""
import hashlib
import io
import os
import struct
import sys
import tempfile
import unittest
import zipfile
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "atlas-builder"))
import build_atlas as b  # noqa: E402

TINY_PATH = os.path.join(ROOT, "tests", "data", "tiny.gatl")
TINY_TEXT = "tiny test atlas"
# lon_q/lat_q chosen to cover: the lon 180 -> 0 wrap, both poles, and a mid-range point.
TINY_LAYERS = [
    (b.LAYER_COASTLINE, [[(0, 0), (65535, 32768), (32768, 16384)]]),
    (b.LAYER_BORDER, [[(1, 2), (3, 4)]]),
]


def shapefile(parts_list):
    """A PolyLine shapefile; each element of parts_list is one record: a list of parts."""
    records = b""
    for number, parts in enumerate(parts_list, 1):
        points = [p for part in parts for p in part]
        starts, n = [], 0
        for part in parts:
            starts.append(n)
            n += len(part)
        content = struct.pack("<I4dII", 3, 0, 0, 0, 0, len(parts), len(points))
        content += struct.pack("<%dI" % len(starts), *starts)
        for lon, lat in points:
            content += struct.pack("<dd", lon, lat)
        records += struct.pack(">II", number, len(content) // 2) + content
    header = struct.pack(">I20xI", 9994, (100 + len(records)) // 2)
    header += struct.pack("<II", 1000, 3) + struct.pack("<8d", 0, 0, 0, 0, 0, 0, 0, 0)
    return header + records


def zip_of(name, shp, version="9.9.9"):
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w") as z:
        z.writestr(name + ".shp", shp)
        z.writestr(name + ".VERSION.txt", version + "\n")
    return buf.getvalue()


def decode(blob):
    """Independent reading of docs/atlas-format.md; strict like the planned C++ decoder."""
    assert len(blob) >= 24 and blob[:4] == b"GATL"
    version, reserved, polys, points, text_len, crc = struct.unpack_from("<HHIIII", blob, 4)
    assert version == 1 and reserved == 0
    assert len(blob) == 24 + 4 * polys + 4 * points + text_len
    assert zlib.crc32(blob[24:]) == crc
    table = [struct.unpack_from("<BBH", blob, 24 + 4 * i) for i in range(polys)]
    assert sum(t[2] for t in table) == points
    offset = 24 + 4 * polys
    out = []
    for layer, zero, count in table:
        assert layer in (b.LAYER_COASTLINE, b.LAYER_BORDER) and zero == 0 and count >= 2
        line = [struct.unpack_from("<HH", blob, offset + 4 * i) for i in range(count)]
        assert all(lat <= 32768 for _, lat in line)
        out.append((layer, line))
        offset += 4 * count
    text = blob[offset:].decode("utf-8")
    assert "\0" not in text
    return out, text


class Quantize(unittest.TestCase):
    def test_corners(self):
        self.assertEqual(b.quantize(-180, -90), (0, 0))
        self.assertEqual(b.quantize(0, 0), (32768, 16384))
        self.assertEqual(b.quantize(0, 90), (32768, 32768))

    def test_antimeridian_wraps_to_one_value(self):
        self.assertEqual(b.quantize(180, 0), b.quantize(-180, 0))
        self.assertEqual(b.quantize(180, 0)[0], 0)
        self.assertEqual(b.quantize(179.999, 0)[0], 0)  # within half a step of 180

    def test_rounds_to_nearest_step(self):
        step = b.STEP
        self.assertEqual(b.quantize(-180 + 0.49 * step, 0)[0], 0)
        self.assertEqual(b.quantize(-180 + 0.51 * step, 0)[0], 1)

    def test_latitude_never_exceeds_range(self):
        self.assertEqual(b.quantize(0, 90.0)[1], 32768)
        self.assertEqual(b.quantize(0, -90.0)[1], 0)
        self.assertEqual(b.quantize(0, 95.0)[1], 32768)  # defensive clamp
        self.assertEqual(b.quantize(0, -95.0)[1], 0)


class Encode(unittest.TestCase):
    def test_exact_bytes_of_a_tiny_atlas(self):
        blob = b.encode([(b.LAYER_COASTLINE, [[(1, 2), (3, 4)]])], "hi")
        body = bytes([1, 0, 2, 0]) + struct.pack("<HHHH", 1, 2, 3, 4) + b"hi"
        expected = b"GATL" + struct.pack("<HHIIII", 1, 0, 1, 2, 2, zlib.crc32(body)) + body
        self.assertEqual(blob, expected)

    def test_round_trips_through_the_independent_decoder(self):
        decoded, text = decode(b.encode(TINY_LAYERS, TINY_TEXT))
        self.assertEqual(text, TINY_TEXT)
        self.assertEqual([layer for layer, _ in decoded], [1, 2])
        self.assertEqual(decoded[0][1], TINY_LAYERS[0][1][0])

    def test_rejects_invalid_input(self):
        with self.assertRaises(ValueError):
            b.encode([(1, [[(0, 0)]])], "")  # one point
        with self.assertRaises(ValueError):
            b.encode([(1, [[(0, 0)] * 65536])], "")  # too many points in one polyline
        with self.assertRaises(ValueError):
            b.encode([(1, [[(0, 0), (1, 1)]])], "a\0b")  # NUL in text
        with self.assertRaises(ValueError):
            b.encode([(1, [[(0, 0), (1, 1)]])], "x" * (b.MAX_TEXT + 1))
        with self.assertRaises(ValueError):
            b.encode([(1, [[(0, 0), (1, 1)]] * (b.MAX_POLYLINES + 1))], "")

    def test_committed_fixture_is_current(self):
        with open(TINY_PATH, "rb") as f:
            self.assertEqual(f.read(), b.encode(TINY_LAYERS, TINY_TEXT))


class Shapefile(unittest.TestCase):
    def test_parts_become_separate_polylines_with_original_vertices(self):
        shp = shapefile([[[(0, 0), (10, 10), (20, 5)], [(30, 0), (40, 0)]], [[(-180, -90), (180, 90)]]])
        lines = b.read_polylines(shp)
        self.assertEqual(lines, [[(0, 0), (10, 10), (20, 5)], [(30, 0), (40, 0)], [(-180, -90), (180, 90)]])

    def test_no_subdivision_across_the_dateline(self):
        lines = b.read_polylines(shapefile([[[(179.5, 10), (-179.5, 10)]]]))
        self.assertEqual(len(lines[0]), 2)

    def test_rejects_malformed_files(self):
        good = shapefile([[[(0, 0), (1, 1)]]])
        with self.assertRaises(ValueError):
            b.read_polylines(b"short")
        with self.assertRaises(ValueError):
            b.read_polylines(b"\0" * 100)  # wrong file code
        with self.assertRaises(ValueError):
            b.read_polylines(good[:-5])  # truncated record
        with self.assertRaises(ValueError):
            b.read_polylines(good + b"\0\0\0")  # trailing partial header
        with self.assertRaises(ValueError):
            b.read_polylines(shapefile([[[(0, 0), (200, 1)]]]))  # longitude out of range
        with self.assertRaises(ValueError):
            b.read_polylines(shapefile([[[(0, 0), (1, 91)]]]))  # latitude out of range
        with self.assertRaises(ValueError):
            b.read_polylines(shapefile([[[(0, 0), (float("nan"), 1)]]]))
        with self.assertRaises(ValueError):
            b.read_polylines(good[:32] + struct.pack("<I", 5) + good[36:])  # not PolyLine

    def test_every_truncation_is_rejected_or_parsed_cleanly(self):
        good = shapefile([[[(0, 0), (1, 1), (2, 2)], [(5, 5), (6, 6)]]])
        for n in range(len(good)):
            try:
                b.read_polylines(good[:n])
            except ValueError:
                pass  # struct.error would escape and fail the test, as it should


class Collapse(unittest.TestCase):
    def test_duplicates_collapse_and_short_lines_drop(self):
        near = [(0.0, 0.0), (0.0001, 0.0001), (10.0, 10.0)]  # first two share a quantum
        self.assertEqual(len(b.quantized_polyline(near)), 2)
        self.assertIsNone(b.quantized_polyline([(1.0, 1.0), (1.0001, 1.0001)]))


class Build(unittest.TestCase):
    def make_zips(self, d):
        paths = []
        for name, key in (("coastline", "coastline"), ("borders", "borders")):
            shp = shapefile([[[(0, 0), (10, 10)]]]) if key == "coastline" else shapefile([[[(5, 5), (6, 7), (8, 9)]]])
            path = os.path.join(d, key + ".zip")
            with open(path, "wb") as f:
                f.write(zip_of(b.SOURCES[key]["member"], shp))
            paths.append(path)
        return paths

    def test_hash_mismatch_is_an_error_unless_allowed(self):
        with tempfile.TemporaryDirectory() as d:
            coast, borders = self.make_zips(d)
            with self.assertRaises(SystemExit):
                b.build(coast, borders)
            blob, layers = b.build(coast, borders, allow_mismatch=True)
            decoded, text = decode(blob)
            self.assertEqual([layer for layer, _ in decoded], [1, 2])
            with open(coast, "rb") as f:
                actual = hashlib.sha256(f.read()).hexdigest()
            self.assertIn(actual, text)  # the actual hash is recorded
            self.assertIn("9.9.9", text)

    def test_build_is_deterministic(self):
        with tempfile.TemporaryDirectory() as d:
            coast, borders = self.make_zips(d)
            first = b.build(coast, borders, allow_mismatch=True)[0]
            second = b.build(coast, borders, allow_mismatch=True)[0]
            self.assertEqual(first, second)


class CommittedAtlas(unittest.TestCase):
    def test_bundled_atlas_is_valid(self):
        path = os.path.join(ROOT, "data", "atlas.gatl")
        with open(path, "rb") as f:
            blob = f.read()
        decoded, text = decode(blob)
        coast = [line for layer, line in decoded if layer == b.LAYER_COASTLINE]
        borders = [line for layer, line in decoded if layer == b.LAYER_BORDER]
        self.assertGreater(len(coast), 100)
        self.assertGreater(len(borders), 300)
        self.assertEqual(len(decoded), len(coast) + len(borders))
        self.assertIn("public domain", text)
        self.assertIn(b.SOURCES["coastline"]["sha256"], text)
        self.assertIn(b.SOURCES["borders"]["sha256"], text)
        self.assertLess(len(blob), 64 * 1024)  # size budget; tighten as the format matures


if __name__ == "__main__":
    if "--write-fixture" in sys.argv:
        with open(TINY_PATH, "wb") as f:
            f.write(b.encode(TINY_LAYERS, TINY_TEXT))
        print("wrote", TINY_PATH)
    else:
        unittest.main()
