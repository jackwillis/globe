#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path
import random
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('bench',ROOT/'tools/compression_bench.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
rng=random.Random(7)
for raw in [b'',bytes(1000),b'\xff'*1000,bytes(range(256))*150,bytes(rng.randrange(256) for _ in range(40000))]:
    encoded=m.range_encode(raw)
    assert m.range_decode(encoded,len(raw))==raw
    if raw:
        predicted=m.predict(raw,1)
        assert m.unpredict(predicted,1)==raw
raw=bytes(rng.randrange(256) for _ in range(128*20))
assert m.unpredict(m.predict(raw,128),128)==raw
atlas=m.atlas_delta((ROOT/'data/atlas.gatl').read_bytes())
assert atlas['delta_varint_point_bytes']==26706 and atlas['roundtrip']
print('ok')
