#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Model A: one generic statement store, in a wide (zero-decode) and a lean (varint) variant."""
import struct

import codec
import model

STATUS = ['ok', 'insufficient-sample', 'insufficient-cases', 'not-applicable', 'moe-insufficient-sample',
          'moe-open-interval', 'moe-controlled-zero', 'empty-cell', 'moe-insufficient-cases',
          'moe-not-applicable']
STATUS_CODE = {name: i for i, name in enumerate(STATUS)}
KIND_INT, KIND_TEXT, KIND_GEOM = 0, 1, 2
KIND_ENTITY = 9   # structural row: the entity's kind, which a generic store must also record
ROW = struct.Struct('<IHBBqiiiHH')   # 32 bytes
FLAG_CENSORED, FLAG_HORIZON = 1, 2

class Probes:
    def __init__(self):
        self.compare = self.decode = self.edges = self.points = 0

def _kind_of(prop):
    return {'int': KIND_INT, 'text': KIND_TEXT, 'geom': KIND_GEOM}[model.PROPS[prop][1]]

def _rows(claims, kinds):
    """Common row construction; the heap holds text and geometry. Returns (rows, heap)."""
    heap, rows = bytearray(), []
    for entity, kind in sorted(kinds.items()):
        rows.append((entity, KIND_ENTITY, KIND_INT, 0, kind, 0, codec.INT32_MIN, codec.INT32_MAX, 0, 0))
    for c in sorted(claims, key=lambda c: (c['entity'], c['prop'], c['from'])):
        kind = _kind_of(c['prop'])
        flags = (FLAG_CENSORED if c['censored'] else 0) | (FLAG_HORIZON if c['horizon'] else 0)
        status = STATUS_CODE[c['status']] | STATUS_CODE[c['moe_status']] << 8
        if kind == KIND_INT:
            value, aux = (0 if c['value'] is None else c['value']), (0 if c['moe'] is None else c['moe'])
        else:
            blob = (c['value'].encode() if kind == KIND_TEXT else _geometry_blob(c['value']))
            value, aux = len(heap), len(blob)
            heap += blob
        rows.append((c['entity'], c['prop'], kind, flags, value, aux, c['from'], c['to'], c['source'], status))
    rows.sort(key=lambda r: (r[0], r[1], r[6]))
    return rows, bytes(heap)

def _geometry_blob(rings):
    q = codec.quantize_rings([codec.drop_closing(r) for r in rings])
    return struct.pack('<4i', *codec.bbox(q)) + codec.pack_geometry(q)

def decode_geometry(blob):
    return codec.unpack_geometry(blob, 16)[0], struct.unpack_from('<4i', blob)

class WideStore:
    name = 'A-wide'

    def __init__(self, claims, kinds):
        rows, heap = _rows(claims, kinds)
        geom_rows = [i for i, r in enumerate(rows) if r[2] == KIND_GEOM]
        self.data = (b'WIA1' + struct.pack('<III', len(rows), len(heap), len(geom_rows)) +
                     b''.join(ROW.pack(*r) for r in rows) + heap + struct.pack(f'<{len(geom_rows)}I', *geom_rows))
        self.packed_bytes = len(self.data)
        self.probes = Probes()
        self._load()

    def _load(self):
        magic, n, heap, g = self.data[:4], *struct.unpack_from('<III', self.data, 4)
        if magic != b'WIA1' or 16 + n * ROW.size + heap + 4 * g != len(self.data):
            raise ValueError('A-wide size')
        self.n, self.heap_at, self.g = n, 16 + n * ROW.size, g
        self.index_at = self.heap_at + heap

    def row(self, i):
        return ROW.unpack_from(self.data, 16 + i * ROW.size)

    def _lower(self, entity, prop, lo_from=codec.INT32_MIN):
        lo, hi = 0, self.n
        while lo < hi:
            mid = (lo + hi) // 2
            self.probes.compare += 1
            if self.row(mid)[:2] + (self.row(mid)[6],) < (entity, prop, lo_from):
                lo = mid + 1
            else:
                hi = mid
        return lo

    def rows_for(self, entity, prop):
        i, out = self._lower(entity, prop), []
        while i < self.n and self.row(i)[:2] == (entity, prop):
            out.append(self.row(i))
            i += 1
        return out

    def blob(self, row):
        return self.data[self.heap_at + row[4]: self.heap_at + row[4] + row[5]]

    def geometry_rows(self):
        return [self.row(i) for i in struct.unpack_from(f'<{self.g}I', self.data, self.index_at)]

    def kind_of(self, entity):
        rows = self.rows_for(entity, KIND_ENTITY)
        return rows[0][4] if rows else None

    def claims(self, entity, prop):
        return [self._claim(r) for r in self.rows_for(entity, prop)]

    def _claim(self, r):
        entity, prop, kind, flags, value, aux, lo, hi, source, status = r
        out = {'entity': entity, 'prop': prop, 'from': lo, 'to': hi, 'source': source,
               'status': STATUS[status & 255], 'moe_status': STATUS[status >> 8],
               'censored': bool(flags & FLAG_CENSORED), 'horizon': bool(flags & FLAG_HORIZON)}
        if kind == KIND_INT:
            ok = out['status'] == 'ok'
            out['value'], out['moe'] = (value if ok else None), (aux if out['moe_status'] == 'ok' else None)
        elif kind == KIND_TEXT:
            out['value'], out['moe'] = self.blob(r).decode(), None
        else:
            out['value'], out['moe'] = r, None
        return out

    def locate(self, x, y, entity_kind, day=None):
        """Entities of `entity_kind` whose geometry covers (x, y) in microdegrees at `day`."""
        found = []
        for r in self.geometry_rows():
            if self.kind_of(r[0]) != entity_kind or not _live(r[6], r[7], day):
                continue
            blob = self.blob(r)
            x0, y0, x1, y1 = struct.unpack_from('<4i', blob)
            if not (x0 <= x <= x1 and y0 <= y <= y1):
                continue
            rings, _ = codec.unpack_geometry(blob, 16)
            inside, edges = codec.contains(rings, x, y)
            self.probes.edges += edges
            self.probes.points += sum(len(g) for g in rings)
            if inside:
                found.append((r[0], r[6], r[7]))
        return sorted(found)

    def bbox_of(self, entity):
        return [struct.unpack_from('<4i', self.blob(r)) for r in self.rows_for(entity, 6)]

    def entities_of(self, kind):
        return sorted({self.row(i)[0] for i in range(self.n) if self.row(i)[1] == KIND_ENTITY and self.row(i)[4] == kind})

def _live(lo, hi, day):
    return day is None or lo <= day < hi

class LeanStore:
    name = 'A-lean'

    def __init__(self, claims, kinds):
        rows, heap = _rows(claims, kinds)
        streams, index = bytearray(), []
        by_entity = {}
        for r in rows:
            by_entity.setdefault(r[0], []).append(r)
        for entity in sorted(by_entity):
            index.append((entity, len(streams)))
            group = by_entity[entity]
            streams += codec.varint(len(group))
            for _, prop, kind, flags, value, aux, lo, hi, source, status in group:
                streams += codec.varint(prop) + bytes([kind | flags << 2, status & 255, status >> 8])
                streams += codec.varint(codec.zigzag(value)) + codec.varint(codec.zigzag(aux))
                span = (lo != codec.INT32_MIN) | (hi != codec.INT32_MAX) << 1
                streams += bytes([span])
                if span & 1:
                    streams += codec.varint(codec.zigzag(lo))
                if span & 2:
                    streams += codec.varint(codec.zigzag(hi))
                streams += codec.varint(source)
        self.data = (b'WIL1' + struct.pack('<III', len(index), len(streams), len(heap)) +
                     b''.join(struct.pack('<II', *i) for i in index) + bytes(streams) + heap)
        self.packed_bytes = len(self.data)
        self.probes = Probes()
        self.n = len(index)
        self.stream_at = 16 + 8 * self.n
        self.heap_at = self.stream_at + len(streams)

    def _decode_entity(self, i):
        entity, offset = struct.unpack_from('<II', self.data, 16 + 8 * i)
        pos = self.stream_at + offset
        count, pos = codec.read_varint(self.data, pos)
        rows = []
        for _ in range(count):
            self.probes.decode += 1
            prop, pos = codec.read_varint(self.data, pos)
            kind_flags, s_est, s_moe = self.data[pos], self.data[pos + 1], self.data[pos + 2]
            pos += 3
            value, pos = codec.read_varint(self.data, pos)
            aux, pos = codec.read_varint(self.data, pos)
            span = self.data[pos]
            pos += 1
            lo, hi = codec.INT32_MIN, codec.INT32_MAX
            if span & 1:
                lo, pos = codec.read_varint(self.data, pos)
                lo = codec.unzigzag(lo)
            if span & 2:
                hi, pos = codec.read_varint(self.data, pos)
                hi = codec.unzigzag(hi)
            source, pos = codec.read_varint(self.data, pos)
            rows.append((entity, prop, kind_flags & 3, kind_flags >> 2, codec.unzigzag(value), codec.unzigzag(aux),
                         lo, hi, source, s_est | s_moe << 8))
        return rows

    def _index_of(self, entity):
        lo, hi = 0, self.n
        while lo < hi:
            mid = (lo + hi) // 2
            self.probes.compare += 1
            if struct.unpack_from('<I', self.data, 16 + 8 * mid)[0] < entity:
                lo = mid + 1
            else:
                hi = mid
        if lo < self.n and struct.unpack_from('<I', self.data, 16 + 8 * lo)[0] == entity:
            return lo
        return None

    def rows_for(self, entity, prop):
        i = self._index_of(entity)
        return [] if i is None else [r for r in self._decode_entity(i) if r[1] == prop]

    def blob(self, row):
        return self.data[self.heap_at + row[4]: self.heap_at + row[4] + row[5]]

    _claim = WideStore._claim
    claims = WideStore.claims
    bbox_of = WideStore.bbox_of

    def kind_of(self, entity):
        rows = self.rows_for(entity, KIND_ENTITY)
        return rows[0][4] if rows else None

    def geometry_rows(self):
        return [r for i in range(self.n) for r in self._decode_entity(i) if r[2] == KIND_GEOM]

    def entities_of(self, kind):
        return sorted({r[0] for i in range(self.n) for r in self._decode_entity(i) if r[1] == KIND_ENTITY and r[4] == kind})

    def locate(self, x, y, entity_kind, day=None):
        return WideStore.locate(self, x, y, entity_kind, day)
