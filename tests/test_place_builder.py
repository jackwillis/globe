#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('builder',ROOT/'tools/places-builder/build_places.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
assert m.encode([(1,'Front',0,0),(2,'Back',150,0)])==(ROOT/'tests/data/places.gplc').read_bytes()
records=m.source(ROOT/'data/wikidata-cities-20261101.json')
assert len(records)==19 and all(r[0]!=3139 for r in records)
assert m.encode(records)==(ROOT/'data/cities.gplc').read_bytes()
for records in [[(1,'Bad',181,0)],[(0,'Bad',0,0)],[(1,'Bad\0',0,0)],[(1,'A',0,0),(1,'B',1,1)]]:
    try:m.encode(records)
    except ValueError:pass
    else:raise AssertionError('accepted invalid input')
print('ok')
