#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Model B: independent physical formats and native accessors. No shared interface.

Layers: a columnar statistics table (Census sentinel codes kept in-band, as published), a tract
polygon layer, a time-versioned county layer and a named waterbody layer. They share only the
geometry byte coding in codec.py. Provenance is deliberately absent from these bytes.
"""
import struct

import codec
import readers

EMPTY_CELL = codec.INT32_MIN
EST_INBAND = {'insufficient-sample': -666666666, 'insufficient-cases': -999999999, 'not-applicable': -888888888}
MOE_INBAND = {'moe-insufficient-sample': -222222222, 'moe-open-interval': -333333333,
              'moe-controlled-zero': -555555555, 'moe-insufficient-cases': -999999999,
              'moe-not-applicable': -888888888}
EST_FROM = {v: k for k, v in EST_INBAND.items()}
MOE_FROM = {v: k for k, v in MOE_INBAND.items()}

class Counter:
    def __init__(self):
        self.compare = self.edges = self.points = 0

def acs_decode(est, moe, top):
    """The one place Census in-band codes become (value, status, moe, moe_status, censored)."""
    if est == EMPTY_CELL:
        return None, 'empty-cell', None, 'ok', False
    if est in EST_FROM:
        return None, EST_FROM[est], None, MOE_FROM.get(moe, 'ok'), False
    moe_status = MOE_FROM.get(moe, 'ok')
    censored = top is not None and est == top and moe_status == 'moe-open-interval'
    return est, 'ok', (None if moe_status != 'ok' else moe), moe_status, censored

def _cell(value, status, table):
    if value is not None:
        return value
    return EMPTY_CELL if status == 'empty-cell' else table[status]

class StatsLayer:
    name = 'stats'

    HEAD = 20   # magic, n, nvars, pad, period from, period to (one reference period per layer)

    def __init__(self, claims_by_entity, props):
        refs = sorted(claims_by_entity)
        periods = {(c['from'], c['to']) for cs in claims_by_entity.values() for c in cs.values()}
        if len(periods) != 1:
            raise ValueError('a statistics layer holds exactly one reference period')
        self.period = periods.pop()
        head = b'WIS1' + struct.pack('<IHHii', len(refs), len(props), 0, *self.period)
        table = b''.join(struct.pack('<HH', p, 1) for p in props)
        cols = []
        for p in props:
            est = [_cell(claims_by_entity[r][p]['value'], claims_by_entity[r][p]['status'], EST_INBAND) for r in refs]
            moe = [_cell(claims_by_entity[r][p]['moe'], claims_by_entity[r][p]['moe_status'], MOE_INBAND) for r in refs]
            cols.append(struct.pack(f'<{len(refs)}i', *est) + struct.pack(f'<{len(refs)}i', *moe))
        self.data = head + table + struct.pack(f'<{len(refs)}I', *refs) + b''.join(cols)
        self.packed_bytes = len(self.data)
        self.counter = Counter()
        self._open()

    def _open(self):
        magic, (n, v, _, lo, hi) = self.data[:4], struct.unpack_from('<IHHii', self.data, 4)
        if magic != b'WIS1' or self.HEAD + 4 * v + 4 * n + 8 * v * n != len(self.data) or lo >= hi:
            raise ValueError('stats layer size')
        self.period = (lo, hi)
        self.n, self.props = n, [struct.unpack_from('<HH', self.data, self.HEAD + 4 * i)[0] for i in range(v)]
        self.refs_at = self.HEAD + 4 * v
        self.cols_at = self.refs_at + 4 * n

    def row_of(self, ref):
        lo, hi = 0, self.n
        while lo < hi:
            mid = (lo + hi) // 2
            self.counter.compare += 1
            if struct.unpack_from('<I', self.data, self.refs_at + 4 * mid)[0] < ref:
                lo = mid + 1
            else:
                hi = mid
        if lo < self.n and struct.unpack_from('<I', self.data, self.refs_at + 4 * lo)[0] == ref:
            return lo
        return None

    def get_raw(self, ref, prop):
        row, col = self.row_of(ref), self.props.index(prop)
        if row is None:
            return None
        base = self.cols_at + col * 8 * self.n
        return (struct.unpack_from('<i', self.data, base + 4 * row)[0],
                struct.unpack_from('<i', self.data, base + 4 * self.n + 4 * row)[0])

class _Polys:
    """Geometry tables shared by the three polygon layouts (ring counts, then points)."""

    @staticmethod
    def pack_rings(all_rings):
        qs = [codec.quantize_rings([codec.drop_closing(r) for r in rings]) for rings in all_rings]
        ring_table = [len(r) for q in qs for r in q]
        points = [v for q in qs for r in q for p in r for v in p]
        return qs, struct.pack(f'<{len(ring_table)}I', *ring_table) + struct.pack(f'<{len(points)}i', *points), ring_table

    def rings_of(self, first, count):
        start = sum(self.ring_sizes[:first])
        out = []
        for size in self.ring_sizes[first:first + count]:
            flat = struct.unpack_from(f'<{2 * size}i', self.data, self.points_at + 8 * start)
            out.append(list(zip(flat[::2], flat[1::2])))
            start += size
        return out

    def hit(self, rec_bbox, first, count, x, y):
        x0, y0, x1, y1 = rec_bbox
        if not (x0 <= x <= x1 and y0 <= y <= y1):
            return False
        rings = self.rings_of(first, count)
        inside, edges = codec.contains(rings, x, y)
        self.counter.edges += edges
        self.counter.points += sum(len(r) for r in rings)
        return inside

class TractLayer(_Polys):
    name = 'tracts'
    REC = struct.Struct('<IIHHII4i')   # ref, name_off, name_len, pad, first_ring, ring_count, bbox

    def __init__(self, items):  # items: (ref, name, rings)
        items = sorted(items)
        qs, geom, ring_table = self.pack_rings([i[2] for i in items])
        heap, recs, first = bytearray(), [], 0
        for (ref, name, _), q in zip(items, qs):
            text = name.encode()
            recs.append(self.REC.pack(ref, len(heap), len(text), 0, first, len(q), *codec.bbox(q)))
            heap += text
            first += len(q)
        self._finish(b'WIT1', recs, ring_table, geom, bytes(heap))

    def _finish(self, magic, recs, ring_table, geom, heap):
        self.data = (magic + struct.pack('<III', len(recs), len(ring_table), len(heap)) +
                     b''.join(recs) + geom + heap)
        self.packed_bytes = len(self.data)
        self.counter = Counter()
        self.n, self.ring_sizes = len(recs), ring_table
        self.recs_at = 16
        self.points_at = 16 + len(recs) * self.REC.size + 4 * len(ring_table)
        self.heap_at = len(self.data) - len(heap)

    def locate(self, x, y):
        out = []
        for i in range(self.n):
            ref, _, _, _, first, count, *box = self.REC.unpack_from(self.data, self.recs_at + i * self.REC.size)
            if self.hit(box, first, count, x, y):
                out.append(ref)
        return out

    def name_of(self, ref):
        for i in range(self.n):
            r = self.REC.unpack_from(self.data, self.recs_at + i * self.REC.size)
            if r[0] == ref:
                return self.data[self.heap_at + r[1]: self.heap_at + r[1] + r[2]].decode()

    def bbox_of(self, ref):
        for i in range(self.n):
            r = self.REC.unpack_from(self.data, self.recs_at + i * self.REC.size)
            if r[0] == ref:
                return r[6:]

    def refs(self):
        return sorted({self.REC.unpack_from(self.data, self.recs_at + i * self.REC.size)[0] for i in range(self.n)})

class CountyLayer(TractLayer):
    name = 'counties'
    REC = struct.Struct('<IIIiiHH4iIHIHxx')   # 54 bytes

    def __init__(self, items):  # items: (ref, version, from, to, horizon, name, note, rings, proposal)
        items = sorted(items, key=lambda i: (i[0], i[2]))
        qs, geom, ring_table = self.pack_rings([i[7] for i in items])
        heap, recs, first = bytearray(), [], 0
        for (ref, version, lo, hi, horizon, name, note, _, proposal), q in zip(items, qs):
            n, t = name.encode(), note.encode()
            recs.append(self.REC.pack(ref, first, len(q), lo, hi, int(horizon) | int(proposal) << 1, version, *codec.bbox(q),
                                      len(heap), len(n), len(heap) + len(n), len(t)))
            heap += n + t
            first += len(q)
        self._finish(b'WIC1', recs, ring_table, geom, bytes(heap))

    def versions(self, ref):
        out = []
        for i in range(self.n):
            r = self.REC.unpack_from(self.data, self.recs_at + i * self.REC.size)
            if r[0] == ref:
                out.append({'version': r[6], 'from': r[3], 'to': r[4], 'horizon': bool(r[5] & 1), 'proposal': bool(r[5] & 2),
                            'name': self._text(r[11], r[12]), 'note': self._text(r[13], r[14])})
        return out

    def _text(self, off, ln):
        return self.data[self.heap_at + off: self.heap_at + off + ln].decode()

    def locate(self, x, y, day, proposals=False):
        out = []
        for i in range(self.n):
            r = self.REC.unpack_from(self.data, self.recs_at + i * self.REC.size)
            if bool(r[5] & 2) == proposals and r[3] <= day < r[4] and self.hit(r[7:11], r[1], r[2], x, y):
                out.append((r[0], r[3], r[4]))
        return sorted(out)

class WaterLayer(TractLayer):
    name = 'water'
    REC = struct.Struct('<IIIIII4i')   # ref, hydroid, wbic, name_off|len16, first_ring, ring_count, bbox

    def __init__(self, items):  # items: (ref, hydroid, wbic, name, rings)
        items = sorted(items)
        qs, geom, ring_table = self.pack_rings([i[4] for i in items])
        heap, recs, first = bytearray(), [], 0
        for (ref, hydroid, wbic, name, _), q in zip(items, qs):
            text = name.encode()
            recs.append(self.REC.pack(ref, hydroid, wbic, len(heap) | len(text) << 24, first, len(q), *codec.bbox(q)))
            heap += text
            first += len(q)
        self._finish(b'WIW1', recs, ring_table, geom, bytes(heap))

    def find_all(self):
        out = []
        for i in range(self.n):
            r = self.REC.unpack_from(self.data, self.recs_at + i * self.REC.size)
            name = self.data[self.heap_at + (r[3] & 0xFFFFFF): self.heap_at + (r[3] & 0xFFFFFF) + (r[3] >> 24)].decode()
            out.append({'ref': r[0], 'hydroid': r[1], 'wbic': r[2], 'name': name, 'first': r[4], 'count': r[5], 'bbox': r[6:]})
        return out

    def find(self, text):
        return [r for r in self.find_all() if r['name'].lower() == text.lower()]
