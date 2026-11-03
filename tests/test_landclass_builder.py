# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path
import tempfile
import unittest
spec=importlib.util.spec_from_file_location('land_builder',Path(__file__).parents[1]/'tools/landclass-builder/build_landclasses.py')
b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
def ring(w,s,e,n): return [(w,s),(e,s),(e,n),(w,n),(w,s)]
class Builder(unittest.TestCase):
    def test_centers_holes_rows(self):
        cells=b.cell_centers([ring(-2,-2,2,2),ring(-1,-1,1,1)])
        self.assertEqual(len(cells),12)
        self.assertIn(88*360+178,cells)
        self.assertNotIn(89*360+179,cells)
        south=b.cell_centers([ring(10,-22,12,-20)])
        self.assertEqual(south,{68*360+190,68*360+191,69*360+190,69*360+191})
    def test_wrap_and_half_open(self):
        cells=b.cell_centers([ring(179,-1,-179,1)])
        self.assertEqual(cells,{89*360,89*360+359,90*360,90*360+359})
        self.assertEqual(b.cell_centers([ring(.5,.5,1.5,1.5)]),{90*360+180})
    def test_roundoff_only_clamp(self):
        exact=b.cell_centers([ring(179,-1,180,1)])
        self.assertEqual(exact,b.cell_centers([ring(179,-1,180+1e-13,1)]))
        with self.assertRaises(ValueError): b.cell_centers([ring(179,-1,180.01,1)])
    def test_classification(self):
        self.assertEqual(b.biome_class(13,'Deserts'),b.DESERT)
        self.assertEqual(b.biome_class(1,'Forest'),b.FOREST)
        self.assertEqual(b.biome_class(11,'Tundra'),b.TUNDRA)
        self.assertEqual(b.biome_class(11,'N/A'),b.UNKNOWN)
        with self.assertRaises(ValueError): b.biome_class(99,'Unknown')
    def test_source_rejected(self):
        with tempfile.NamedTemporaryFile() as f:
            f.write(b'wrong source');f.flush()
            with self.assertRaisesRegex(ValueError,'SHA-256'): list(b.records(f.name))
if __name__=='__main__': unittest.main()
