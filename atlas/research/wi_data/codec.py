#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Byte codecs shared by the A and B prototypes. Little-endian, explicit, bounded.

Geometry coding is deliberately the same for both models (int32 microdegrees, ring table plus
point table, as in GPOL v1) so that byte differences come from the surrounding structure.
"""
import math
import struct

MICRO = 1_000_000
INT32_MIN, INT32_MAX = -2**31, 2**31 - 1

def quantize(value):
    q = round(value * MICRO)
    if not (-180 * MICRO <= q <= 180 * MICRO):
        raise ValueError('coordinate out of range')
    return q

def quantize_rings(rings):
    for ring in rings:
        for lon, lat in ring:
            if not (math.isfinite(lon) and math.isfinite(lat)):
                raise ValueError('non-finite coordinate')
    return [[(quantize(lon), quantize(lat)) for lon, lat in ring] for ring in rings]

def drop_closing(ring):
    return ring[:-1] if len(ring) > 1 and ring[0] == ring[-1] else ring

def bbox(qrings):
    xs = [p[0] for ring in qrings for p in ring]
    ys = [p[1] for ring in qrings for p in ring]
    return (min(xs), min(ys), max(xs), max(ys))

def pack_geometry(qrings):
    """u32 ring_count, u32 point_count per ring, then int32 lon/lat pairs. No closing vertex."""
    out = [struct.pack('<I', len(qrings))]
    out += [struct.pack('<I', len(r)) for r in qrings]
    for ring in qrings:
        out.append(struct.pack(f'<{2 * len(ring)}i', *[v for p in ring for v in p]))
    return b''.join(out)

def unpack_geometry(blob, offset=0):
    """Strict and bounded: returns (rings, next_offset)."""
    (count,), offset = struct.unpack_from('<I', blob, offset), offset + 4
    if count == 0 or count > 65536 or offset + 4 * count > len(blob):
        raise ValueError('geometry ring table')
    sizes = struct.unpack_from(f'<{count}I', blob, offset)
    offset += 4 * count
    rings = []
    for n in sizes:
        if n < 3 or n > 1_000_000 or offset + 8 * n > len(blob):
            raise ValueError('geometry ring bounds')
        flat = struct.unpack_from(f'<{2 * n}i', blob, offset)
        rings.append(list(zip(flat[::2], flat[1::2])))
        offset += 8 * n
    return rings, offset

def contains(qrings, x, y):
    """Even-odd rule over all rings of one feature (holes subtract). Returns (inside, edges_tested)."""
    inside, edges = False, 0
    for ring in qrings:
        j = len(ring) - 1
        for i in range(len(ring)):
            xi, yi = ring[i]
            xj, yj = ring[j]
            edges += 1
            if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
                inside = not inside
            j = i
    return inside, edges

def varint(n):
    if n < 0:
        raise ValueError('varint is unsigned')
    out = bytearray()
    while True:
        byte = n & 0x7F
        n >>= 7
        out.append(byte | (0x80 if n else 0))
        if not n:
            return bytes(out)

def read_varint(buf, offset):
    n = shift = 0
    while True:
        if offset >= len(buf) or shift > 63:
            raise ValueError('varint bounds')
        byte = buf[offset]
        offset += 1
        n |= (byte & 0x7F) << shift
        if not byte & 0x80:
            return n, offset
        shift += 7

def zigzag(n):
    return (n << 1) ^ (n >> 63)

def unzigzag(n):
    return (n >> 1) ^ -(n & 1)
