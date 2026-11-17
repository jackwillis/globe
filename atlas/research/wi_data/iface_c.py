#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Shared, bounded semantic inspection/query interface. Layer-neutral; reads descriptors only.

A backend supplies: kind_of(ref), claims(ref, day), locate(x, y, kind, day), find_name(text),
bbox(ref), name(ref). Everything else (formatting, budgeting, status wording, provenance, time
handling, truncation) is here. The descriptor and provenance blocks are the only metadata added
to the physical layers.
"""
import struct

import model

KIND_NAMES = {model.KIND_TRACT: 'tract', model.KIND_COUNTY: 'county', model.KIND_WATERBODY: 'waterbody', model.KIND_PROPOSAL: 'proposed county'}
LICENSE = {'unreviewed': 0, 'conflicting': 1, 'allowed': 2}
LICENSE_NAMES = {v: k for k, v in LICENSE.items()}

def _pstr(text):
    raw = text.encode()
    if len(raw) > 255:
        raise ValueError('descriptor string too long')
    return bytes([len(raw)]) + raw

def _read_pstr(buf, pos):
    n = buf[pos]
    return buf[pos + 1: pos + 1 + n].decode(), pos + 1 + n

def encode_descriptors(layers):
    """layers: [(layer_id, entity_kind, time_mode, geometry, codes, [prop ids])]."""
    out = [bytes([len(layers)])]
    for layer_id, kind, time_mode, geometry, codes, props in layers:
        out.append(bytes([layer_id, kind, time_mode, geometry, codes, len(props)]))
        for p in props:
            name, vkind, scale, unit, _, top = model.PROPS[p]
            out.append(bytes([p, {'int': 0, 'text': 1, 'geom': 2}[vkind], scale]) +
                       struct.pack('<i', 0 if top is None else top) + _pstr(name) + _pstr(unit))
    return b''.join(out)

def decode_descriptors(blob):
    pos, layers = 1, []
    for _ in range(blob[0]):
        layer_id, kind, time_mode, geometry, codes, n = blob[pos:pos + 6]
        pos += 6
        props = {}
        for _ in range(n):
            p, vkind, scale = blob[pos:pos + 3]
            (top,) = struct.unpack_from('<i', blob, pos + 3)
            name, pos = _read_pstr(blob, pos + 7)
            unit, pos = _read_pstr(blob, pos)
            props[p] = {'name': name, 'vkind': vkind, 'scale': scale, 'unit': unit, 'top': top or None}
        layers.append({'id': layer_id, 'kind': kind, 'time': time_mode, 'geometry': geometry,
                       'codes': codes, 'props': props})
    return layers

def encode_provenance(sources):
    out = [bytes([len(sources)])]
    for ident, title, version, license_status in sources:
        out.append(_pstr(ident) + _pstr(title) + _pstr(version) + bytes([LICENSE[license_status]]))
    return b''.join(out)

def decode_provenance(blob):
    pos, out = 1, []
    for _ in range(blob[0]):
        ident, pos = _read_pstr(blob, pos)
        title, pos = _read_pstr(blob, pos)
        version, pos = _read_pstr(blob, pos)
        out.append({'id': ident, 'title': title, 'version': version, 'license': LICENSE_NAMES[blob[pos]]})
        pos += 1
    return out

def _fmt_num(value, scale):
    if scale == 1:
        return f'{value:,}'
    return f'{value // scale}.{value % scale:0{len(str(scale)) - 1}d}'

def _fmt_interval(c):
    lo = 'beginning' if c['from'] == model.UNBOUNDED_FROM else model.from_days(c['from']).isoformat()
    if c['to'] == model.UNBOUNDED_TO:
        return f'{lo}..open'
    last = model.from_days(c['to'] - 1).isoformat()
    return f"{lo}..{last}{' (source horizon)' if c['horizon'] else ''}"

class Inspector:
    def __init__(self, backend, descriptors, provenance, identities):
        self.backend = backend
        self.layers = decode_descriptors(descriptors)
        self.sources = decode_provenance(provenance)
        self.identities = identities     # ref -> source-qualified key (identity table, shared by all models)
        self.props = {p: d for layer in self.layers for p, d in layer['props'].items()}

    def _line(self, c):
        d = self.props[c['prop']]
        label = d['name']
        if d['vkind'] == 2:
            return f"{label}: {c['summary']}  [{_fmt_interval(c)}]" if c['to'] != model.UNBOUNDED_TO or c['from'] != model.UNBOUNDED_FROM else f"{label}: {c['summary']}"
        if d['vkind'] == 1:
            return f"{label}: {c['value']}"
        if c['status'] != 'ok':
            return f"{label}: not available ({c['status']})"
        text = _fmt_num(c['value'], d['scale'])
        if c['censored']:
            return f"{label}: at least {_fmt_num(c['value'] - 1, d['scale'])} {d['unit']} (top-coded; margin of error not computable)"
        margin = f" +/- {_fmt_num(c['moe'], d['scale'])}" if c['moe'] is not None else f" (margin unavailable: {c['moe_status']})"
        return f"{label}: {text}{margin} {d['unit']}"

    def describe(self, ref, budget=1024, max_lines=24, at=None):
        kind = self.backend.kind_of(ref)
        if kind is None:
            return {'lines': [f'unknown reference {ref:#x}'], 'bytes': 24, 'truncated': False, 'status': 'unknown'}
        claims = self.backend.claims(ref, at)
        claims.sort(key=lambda c: (c['prop'] == 1, c['prop'], c['from']), reverse=False)
        head = [f"{self.backend.name(ref, at)} [{KIND_NAMES[kind]}]", f"id: {self.identities[ref]}"]
        periods = {(c['from'], c['to']) for c in claims if self.props[c['prop']]['vkind'] == 0}
        if len(periods) == 1:   # one shared reference period is stated once, not on every line
            lo, hi = next(iter(periods))
            head.append('period: ' + _fmt_interval({'from': lo, 'to': hi, 'horizon': False}))
        body = [self._line(c) for c in claims if c['prop'] != 1]
        used = sorted({c['source'] for c in claims})
        tail = [f"source: {self.sources[s]['id']} {self.sources[s]['version']} licence {self.sources[s]['license']}" for s in used]
        lines, truncated = head + body, False
        reserved = sum(len(t.encode()) + 1 for t in tail)
        kept = []
        total = reserved + sum(len(h.encode()) + 1 for h in head)
        for line in body:
            cost = len(line.encode()) + 1
            if len(head) + len(kept) + len(tail) + 1 > max_lines or total + cost > budget:
                truncated = True
                continue
            kept.append(line)
            total += cost
        lines = head + kept + (['... truncated'] if truncated else []) + tail
        return {'lines': lines, 'bytes': sum(len(l.encode()) + 1 for l in lines), 'truncated': truncated, 'status': 'ok'}

    def value(self, ref, prop, at=None):
        matches = [c for c in self.backend.claims(ref, at) if c['prop'] == prop]
        if not matches:
            return {'status': 'no-claim'}
        if len(matches) > 1:
            return {'status': 'ambiguous', 'count': len(matches)}
        c = matches[0]
        return {'status': c['status'], 'value': c['value'], 'moe': c['moe'], 'moe_status': c['moe_status'],
                'censored': c['censored'], 'source': self.sources[c['source']]['id']}

    def locate(self, lon, lat, kind, at=None):
        return self.backend.locate(lon, lat, kind, at)

    def find_name(self, text):
        return self.backend.find_name(text)

    def history(self, ref):
        return self.backend.history(ref)

    def intersecting(self, ref, kind):
        """Entities of `kind` whose bounding box meets ref's (a cheap, honest cross-layer join)."""
        a = self.backend.bbox(ref)
        return [r for r in self.backend.entities(kind)
                if r != ref and not (self.backend.bbox(r)[2] < a[0] or a[2] < self.backend.bbox(r)[0] or
                                     self.backend.bbox(r)[3] < a[1] or a[3] < self.backend.bbox(r)[1])]
