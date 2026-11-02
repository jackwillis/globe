#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Encoder for the GL-29 prototype codecs (see include/globe/grid_codec.hpp and point_codec.hpp).

  grid_codec.py gelv  data/elevation-1deg.gelv out.ggrd      cells of a GELV file
  grid_codec.py npz   grids.npz KEY out.ggrd                  one array of an .npz (needs numpy)
  grid_codec.py raw   WIDTH HEIGHT cells.bin out.ggrd         raw one-byte cells
  grid_codec.py atlas data/atlas.gatl out.dvar                delta-varint stream of every polyline

Standard library only except for the npz mode.
"""
import struct
import sys
import zlib

BUCKET = [0 if d == 0 else 1 if d == 1 else 2 if d < 4 else 3 if d < 8 else 4 if d < 24 else 5
          for d in range(256)]


class RangeEncoder:
    """LZMA-style binary range encoder: 11-bit probabilities, shift 5, carry propagation."""

    def __init__(self):
        self.low = 0
        self.range = 0xFFFFFFFF
        self.cache = 0
        self.cache_size = 1
        self.out = bytearray()

    def _shift(self):
        if self.low < 0xFF000000 or self.low >= 1 << 32:
            carry = self.low >> 32
            byte = self.cache
            while True:
                self.out.append((byte + carry) & 255)
                byte = 255
                self.cache_size -= 1
                if not self.cache_size:
                    break
            self.cache = (self.low >> 24) & 255
        self.cache_size += 1
        self.low = (self.low & 0x00FFFFFF) << 8

    def bit(self, probs, index, bit):
        bound = (self.range >> 11) * probs[index]
        if bit:
            self.low += bound
            self.range -= bound
            probs[index] -= probs[index] >> 5
        else:
            self.range = bound
            probs[index] += (2048 - probs[index]) >> 5
        while self.range < 1 << 24:
            self.range = (self.range << 8) & 0xFFFFFFFF
            self._shift()

    def finish(self):
        for _ in range(5):
            self._shift()
        return bytes(self.out[1:])  # the first byte is always 0


def encode_cells(width, height, cells):
    """Range-coded payload for `cells` (bytes, row-major)."""
    assert len(cells) == width * height
    probs = [[1024] * 256 for _ in range(6)]
    enc = RangeEncoder()
    for y in range(height):
        row = y * width
        for x in range(width):
            left = cells[row + x - 1] if x else (cells[row - width] if y else 128)
            up = cells[row - width + x] if y else left
            residual = (cells[row + x] - ((left + up) >> 1)) & 255
            p = probs[BUCKET[abs(left - up)]]
            m = 1
            for k in range(7, -1, -1):
                b = (residual >> k) & 1
                enc.bit(p, m, b)
                m = (m << 1) | b
    return enc.finish()


def encode_grid(width, height, cells):
    """Complete GGRD container."""
    cells = bytes(cells)
    payload = encode_cells(width, height, cells)
    header = b'GGRD' + struct.pack('<HHIIIIII', 1, 0, width, height, width * height, len(payload),
                                   zlib.crc32(payload), zlib.crc32(cells))
    return header + payload


def varint(value, out):
    while value >= 0x80:
        out.append(value & 0x7F | 0x80)
        value >>= 7
    out.append(value)


def zigzag(v):
    return v * 2 if v >= 0 else -v * 2 - 1


def encode_points(points):
    """Delta-varint bytes for one polyline of (x, y) pairs, starting from (0, 0)."""
    out = bytearray()
    px = py = 0
    for x, y in points:
        varint(zigzag(x - px), out)
        varint(zigzag(y - py), out)
        px, py = x, y
    return bytes(out)


def read_atlas_polylines(path):
    data = open(path, 'rb').read()
    polylines, points, _ = struct.unpack_from('<III', data, 8)
    table = [struct.unpack_from('<BxH', data, 24 + 4 * i) for i in range(polylines)]
    raw = struct.unpack_from('<%dH' % (2 * points), data, 24 + 4 * polylines)
    result, k = [], 0
    for _, n in table:
        result.append([(raw[2 * (k + i)], raw[2 * (k + i) + 1]) for i in range(n)])
        k += n
    return result


def main(argv):
    if len(argv) == 4 and argv[1] == 'gelv':
        cells = open(argv[2], 'rb').read()[552:]
        open(argv[3], 'wb').write(encode_grid(360, 180, cells))
    elif len(argv) == 5 and argv[1] == 'npz':
        import numpy as np
        a = np.load(argv[2])[argv[3]]
        open(argv[4], 'wb').write(encode_grid(a.shape[1], a.shape[0], a.astype(np.uint8).tobytes()))
    elif len(argv) == 6 and argv[1] == 'raw':
        open(argv[5], 'wb').write(encode_grid(int(argv[2]), int(argv[3]), open(argv[4], 'rb').read()))
    elif len(argv) == 4 and argv[1] == 'atlas':
        open(argv[3], 'wb').write(b''.join(encode_points(p) for p in read_atlas_polylines(argv[2])))
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main(sys.argv)
