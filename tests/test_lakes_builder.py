#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path
import tempfile
import json
import unittest
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('builder',ROOT/'tools/elevation-builder/build_elevation.py')
builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
class Lakes(unittest.TestCase):
    def test_holes_and_rows(self):
        outer=[(-50,-40),(50,-40),(50,40),(-50,40),(-50,-40)]
        hole=[(-20,-20),(20,-20),(20,20),(-20,20),(-20,-20)]
        cells=builder.lake_cells(12,6,[[outer,hole]])
        self.assertEqual(cells,{2*12+4,2*12+7,3*12+4,3*12+7})
        # A northern lake cannot be mirrored to the south-first array's first rows.
        north=[(-10,60),(10,60),(10,80),(-10,80),(-10,60)]
        self.assertEqual(builder.lake_cells(36,18,[[north]]),{15*36+17,15*36+18,16*36+17,16*36+18})
    def test_antimeridian_and_union(self):
        ring=[(170,-10),(-170,-10),(-170,10),(170,10),(170,-10)]
        expected={8*36,8*36+35,9*36,9*36+35}
        self.assertEqual(builder.lake_cells(36,18,[[ring],[ring]]),expected)
    def test_water_encoding(self):
        cells=bytearray([128,192,64,255]);builder.mark_water(cells,{0,1,2})
        self.assertEqual(cells,bytearray([127,127,64,255]))
        self.assertEqual(builder.lookup()[127],-1)
    def test_embedded_great_lakes(self):
        fixture=json.loads((ROOT/'tests/data/great-lakes-centers.json').read_text())
        self.assertEqual(fixture['source_sha256'],builder.LAKES_SHA256)
        selected=fixture['indices_south_first'];self.assertEqual(len(selected),30)
        cells=(ROOT/'data/elevation-1deg.gelv').read_bytes()[552:]
        self.assertTrue(all(cells[i]<128 for i in selected))
    def test_source_hash_rejection(self):
        with tempfile.TemporaryDirectory() as folder:
            p=Path(folder)/'bad.zip';p.write_bytes(b'not a source')
            with self.assertRaises(ValueError):builder.read_lakes(p)
if __name__=='__main__':unittest.main()
