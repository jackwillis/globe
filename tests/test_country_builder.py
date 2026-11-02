#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path
import struct
import zlib
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('builder',ROOT/'tools/country-builder/build_countries.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
data=m.encode([('Date line',[(False,0,[(170,-10),(-170,-10),(-170,10),(170,10)])]),('North pole',[(False,0,[(-135,80),(-45,80),(45,80),(135,80)])]),('Hole country',[(False,0,[(20,-10),(40,-10),(40,10),(20,10)]),(True,0,[(28,-3),(32,-3),(32,3),(28,3)])])])
assert data==(ROOT/'tests/data/countries.gpol').read_bytes()
assert struct.unpack_from('<I',data,28)[0]==zlib.crc32(data[32:],zlib.crc32(data[:28]))
for name,rings in [('Bad\0',[(False,0,[(0,0),(1,0),(0,1)])]),('Bad',[(False,0,[(0,0),(1,0)])]),('Bad',[(False,0,[(float('nan'),0),(1,0),(0,1)])])]:
    try:m.encode([(name,rings)])
    except ValueError:pass
    else:raise AssertionError('accepted invalid fixture')
print('ok')
