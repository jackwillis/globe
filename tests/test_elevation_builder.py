# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path
import struct
import hashlib
import unittest
import zlib
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('builder',ROOT/'tools/elevation-builder/build_elevation.py')
builder=importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)
class Format(unittest.TestCase):
    def test_fixture(self):
        data=builder.encode(3,2,bytes([0,51,102,153,204,255]))
        self.assertEqual(data,(ROOT/'tests/data/elevation.gelv').read_bytes())
        self.assertEqual(len(data),558)
        self.assertEqual(data[:4],b'GELV')
        self.assertEqual(struct.unpack_from('<I',data,36)[0],zlib.crc32(data[:36]+data[40:]))
        values=struct.unpack_from('<256h',data,40)
        self.assertEqual((values[0],values[128],values[255]),(-11000,0,9000))
        self.assertTrue(all(a<b for a,b in zip(values,values[1:])))
    def test_quantization(self):
        for value,index in [(-12000,0),(-11000,0),(-1,127),(-0.5,127),(0,128),(9000,255),(10000,255)]:
            self.assertEqual(builder.quantize(value),index)
        with self.assertRaises(ValueError): builder.quantize(float('nan'))
    def test_pack_pin(self):
        data=(ROOT/'data/elevation-1deg.gelv').read_bytes()
        self.assertEqual(hashlib.sha256(data).hexdigest(),'8d0fd2e6777f24dc13ac47e0f73aa459e503c70116d66a61bb2b339b618766a6')
    def test_invalid(self):
        for w,h,cells in [(0,2,b''),(2,2,b'123'),(32769,1,b'')]:
            with self.assertRaises(ValueError): builder.encode(w,h,cells)
if __name__=='__main__': unittest.main()
