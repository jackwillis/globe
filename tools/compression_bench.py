#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Modern-machine size experiment only. No runtime codec or new pack format is selected."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import zlib

class Model:
    def __init__(self):
        self.freq = [1]*256
        self.rebuild()
    def rebuild(self):
        self.total = sum(self.freq)
        self.tree = [0]*257
        for symbol, frequency in enumerate(self.freq):
            self.add(symbol, frequency)
    def add(self, symbol, amount):
        i = symbol+1
        while i <= 256:
            self.tree[i] += amount
            i += i & -i
    def lower(self, symbol):
        result = 0
        while symbol:
            result += self.tree[symbol]
            symbol -= symbol & -symbol
        return result
    def symbol(self, value):
        index = 0
        bit = 256
        while bit:
            next_index = index+bit
            if next_index <= 256 and self.tree[next_index] <= value:
                index = next_index
                value -= self.tree[next_index]
            bit >>= 1
        if index >= 256:
            raise ValueError('invalid range symbol')
        return index
    def update(self, symbol):
        self.freq[symbol] += 1
        self.total += 1
        self.add(symbol, 1)
        if self.total >= 32768:
            self.freq = [(f+1)//2 for f in self.freq]
            self.rebuild()

def range_encode(raw):
    model = Model()
    low, width, cache, pending = 0, 0xffffffff, 0, 1
    out = bytearray()
    def shift():
        nonlocal low, cache, pending
        carry = low >> 32
        if (low & 0xffffffff) < 0xff000000 or carry:
            out.append((cache+carry) & 255)
            out.extend(bytes([(255+carry) & 255])*(pending-1))
            cache = (low >> 24) & 255
            pending = 0
        pending += 1
        low = (low & 0xffffff) << 8
    for symbol in raw:
        width //= model.total
        low += model.lower(symbol)*width
        width *= model.freq[symbol]
        while width < 0x1000000:
            width <<= 8
            shift()
        model.update(symbol)
    for _ in range(5):
        shift()
    return bytes(out)

def range_decode(raw, length):
    if len(raw) < 5 or raw[0] != 0:
        raise ValueError('truncated range stream')
    index = 5
    code = int.from_bytes(raw[1:5], 'big')
    width = 0xffffffff
    model = Model()
    out = bytearray()
    for _ in range(length):
        width //= model.total
        symbol = model.symbol(code//width)
        code -= model.lower(symbol)*width
        width *= model.freq[symbol]
        while width < 0x1000000:
            if index >= len(raw):
                raise ValueError('truncated range stream')
            code = ((code << 8) | raw[index]) & 0xffffffff
            index += 1
            width <<= 8
        out.append(symbol)
        model.update(symbol)
    return bytes(out)

def predict(raw, width):
    out = bytearray()
    for i, value in enumerate(raw):
        left = raw[i-1] if i % width else 0
        up = raw[i-width] if i >= width else 0
        diagonal = raw[i-width-1] if i >= width and i % width else 0
        estimate = max(0, min(255, left+up-diagonal))
        out.append((value-estimate) & 255)
    return bytes(out)

def unpredict(raw, width):
    out = bytearray()
    for i, value in enumerate(raw):
        left = out[i-1] if i % width else 0
        up = out[i-width] if i >= width else 0
        diagonal = out[i-width-1] if i >= width and i % width else 0
        out.append((value+max(0, min(255, left+up-diagonal))) & 255)
    return bytes(out)

def varint(value):
    out = bytearray()
    while value >= 128:
        out.append((value & 127) | 128)
        value >>= 7
    out.append(value)
    return out

def atlas_delta(raw):
    lines, points = struct.unpack_from('<II', raw, 8)
    offset = 24+4*lines
    out = bytearray()
    decoded = []
    cursor = 0
    for line in range(lines):
        count = struct.unpack_from('<H', raw, 24+4*line+2)[0]
        previous = [0, 0]
        for _ in range(count):
            point = struct.unpack_from('<HH', raw, offset+cursor*4)
            for axis, coordinate in enumerate(point):
                delta = coordinate-previous[axis]
                if axis == 0:
                    delta = (delta+32768) % 65536-32768
                zigzag = 2*delta if delta >= 0 else -2*delta-1
                out.extend(varint(zigzag))
                previous[axis] = coordinate
            cursor += 1
    # Independent reader verifies the candidate variable-length point stream.
    at = 0
    for line in range(lines):
        previous = [0, 0]
        for _ in range(struct.unpack_from('<H', raw, 24+4*line+2)[0]):
            for axis in range(2):
                value, shift = 0, 0
                while True:
                    byte = out[at]; at += 1
                    value |= (byte & 127) << shift
                    if byte < 128: break
                    shift += 7
                delta = value//2 if value % 2 == 0 else -(value//2)-1
                previous[axis] = (previous[axis]+delta) % 65536 if axis == 0 else previous[axis]+delta
            decoded.append(tuple(previous))
    assert decoded == list(struct.iter_unpack('<HH', raw[offset:offset+4*points]))
    return {'raw_point_bytes': points*4, 'delta_varint_point_bytes': len(out),
            'whole_file_candidate_bytes': len(raw)-points*4+len(out), 'roundtrip': True}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('elevation'); p.add_argument('atlas'); p.add_argument('output')
    args = p.parse_args()
    raw = Path(args.elevation).read_bytes()
    if raw[:4] != b'GELV': p.error('expected GELV')
    width, height = struct.unpack_from('<II', raw, 8)
    cells = raw[552:]
    if len(cells) != width*height: p.error('grid size')
    predicted = predict(cells, width)
    assert unpredict(predicted, width) == cells
    results = {}
    for name, data in [('plain', cells), ('gradient', predicted)]:
        compressed = zlib.compress(data, 9)
        assert zlib.decompress(compressed) == data
        begin = time.perf_counter()
        encoded = range_encode(data)
        encode_ms = (time.perf_counter()-begin)*1000
        begin = time.perf_counter()
        assert range_decode(encoded, len(data)) == data
        decode_ms = (time.perf_counter()-begin)*1000
        results[name] = {'zlib_level9_bytes': len(compressed), 'adaptive_range_bytes': len(encoded),
                         'range_encode_host_ms': round(encode_ms, 3), 'range_decode_host_ms': round(decode_ms, 3),
                         'roundtrip': True}
    report = {'elevation_bytes': len(raw), 'cell_bytes': len(cells), 'grid': '%dx%d' % (width, height),
              'elevation_sha256': hashlib.sha256(raw).hexdigest(), 'candidates': results,
              'atlas_delta': atlas_delta(Path(args.atlas).read_bytes()),
              'limits': 'modern Python prototype, order-0 adaptive byte range coder; sizes exclude container and 552-byte metadata; no target decoder, target timing or runtime format selected'}
    Path(args.output).write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
