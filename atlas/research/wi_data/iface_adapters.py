#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Backends: the thin, layer-specific glue under the shared Inspector."""
import codec
import model
import readers
import store_a
import store_b

def _micro(lon, lat):
    return codec.quantize(lon), codec.quantize(lat)

class ABackend:
    """The generic store needs almost no glue: its rows already are claims."""

    def __init__(self, store):
        self.store = store

    def kind_of(self, ref):
        return self.store.kind_of(ref)

    def claims(self, ref, day=None):
        out = []
        for prop in model.PROPS:
            for c in self.store.claims(ref, prop):
                if not (day is None or c['from'] <= day < c['to']):
                    continue
                if prop == 6:
                    rings, box = store_a.decode_geometry(self.store.blob(c['value']))
                    c = dict(c, value=None, summary=f'{len(rings)} rings, {sum(map(len, rings))} points')
                out.append(c)
        return out

    def name(self, ref, day=None):
        names = [c for c in self.store.claims(ref, 1) if day is None or c['from'] <= day < c['to']]
        return names[-1]['value'] if names else '?'

    def locate(self, lon, lat, kind, day=None):
        x, y = _micro(lon, lat)
        return [r[0] for r in self.store.locate(x, y, kind, day)]

    def find_name(self, text):
        return sorted({r for kind in (model.KIND_TRACT, model.KIND_COUNTY, model.KIND_WATERBODY)
                       for r in self.store.entities_of(kind) for c in self.store.claims(r, 1)
                       if c['value'].lower() == text.lower()})

    def history(self, ref):
        return [(c['from'], c['to'], c['horizon']) for c in self.store.claims(ref, 6)]

    def bbox(self, ref):
        boxes = self.store.bbox_of(ref)
        return (min(b[0] for b in boxes), min(b[1] for b in boxes), max(b[2] for b in boxes), max(b[3] for b in boxes))

    def entities(self, kind):
        return self.store.entities_of(kind)

class BBackend:
    """Glue over the four independent B layers. One small class per layer."""

    def __init__(self, stats, tracts, counties, water, descriptors):
        self.stats, self.tracts, self.counties, self.water = stats, tracts, counties, water
        self.tops = {p: d['top'] for layer in descriptors for p, d in layer['props'].items()}
        self.scale = {p: d['scale'] for layer in descriptors for p, d in layer['props'].items()}

    def kind_of(self, ref):
        if self.tracts.name_of(ref) is not None:
            return model.KIND_TRACT
        versions = self.counties.versions(ref)
        if versions:
            return model.KIND_PROPOSAL if versions[0]['proposal'] else model.KIND_COUNTY
        return model.KIND_WATERBODY if self._water(ref) else None

    def _water(self, ref):
        for i in range(self.water.n):
            if store_b.WaterLayer.REC.unpack_from(self.water.data, self.water.recs_at + i * self.water.REC.size)[0] == ref:
                return True
        return False

    def claims(self, ref, day=None):
        kind = self.kind_of(ref)
        out = []
        if kind == model.KIND_TRACT:
            out.append(self._c(ref, 6, summary=f'{self._ring_summary(self.tracts, ref)}', source=model.SRC_TRACT))
            out.append(self._c(ref, 1, value=self.tracts.name_of(ref), source=model.SRC_TRACT))
            for prop in self.stats.props:
                raw = self.stats.get_raw(ref, prop)
                if raw is None:
                    continue
                value, status, moe, moe_status, censored = store_b.acs_decode(*raw, self.tops[prop])
                out.append(self._c(ref, prop, value=value, status=status, moe=moe, moe_status=moe_status,
                                   censored=censored, source=model.SRC_ACS, lo=self.stats.period[0], hi=self.stats.period[1]))
        elif kind in (model.KIND_COUNTY, model.KIND_PROPOSAL):
            for v in self.counties.versions(ref):
                if day is None or v['from'] <= day < v['to']:
                    out.append(self._c(ref, 1, value=v['name'], source=model.SRC_AHCB, lo=v['from'], hi=v['to'], horizon=v['horizon']))
                    out.append(self._c(ref, 8, value=v['note'], source=model.SRC_AHCB, lo=v['from'], hi=v['to'], horizon=v['horizon']))
                    out.append(self._c(ref, 6, summary=f"version {v['version']}", source=model.SRC_AHCB, lo=v['from'], hi=v['to'], horizon=v['horizon']))
        elif kind == model.KIND_WATERBODY:
            for r in self._water_rows(ref):
                out.append(self._c(ref, 1, value=r['name'], source=model.SRC_DNR))
                out.append(self._c(ref, 7, value=f"WBIC {r['wbic']}", source=model.SRC_DNR))
                out.append(self._c(ref, 6, summary=self._ring_summary(self.water, ref), source=model.SRC_DNR))
        return out

    @staticmethod
    def _c(ref, prop, value=None, status='ok', moe=None, moe_status='ok', censored=False, summary=None,
           source=0, lo=model.UNBOUNDED_FROM, hi=model.UNBOUNDED_TO, horizon=False):
        return {'entity': ref, 'prop': prop, 'value': value, 'status': status, 'moe': moe, 'moe_status': moe_status,
                'censored': censored, 'from': lo, 'to': hi, 'horizon': horizon, 'source': source, 'summary': summary}

    @staticmethod
    def _ring_summary(layer, ref):
        for i in range(layer.n):
            rec = layer.REC.unpack_from(layer.data, layer.recs_at + i * layer.REC.size)
            if rec[0] == ref:
                first, count = (rec[4], rec[5]) if layer.name != 'counties' else (rec[1], rec[2])
                sizes = layer.ring_sizes[first:first + count]
                return f'{count} rings, {sum(sizes)} points'

    def _water_rows(self, ref):
        return [r for r in self.water.find_all() if r['ref'] == ref]

    def name(self, ref, day=None):
        kind = self.kind_of(ref)
        if kind == model.KIND_TRACT:
            return self.tracts.name_of(ref)
        if kind in (model.KIND_COUNTY, model.KIND_PROPOSAL):
            vs = [v for v in self.counties.versions(ref) if day is None or v['from'] <= day < v['to']]
            return vs[-1]['name'] if vs else '?'
        return self._water_rows(ref)[0]['name']

    def locate(self, lon, lat, kind, day=None):
        x, y = _micro(lon, lat)
        if kind == model.KIND_TRACT:
            return sorted(self.tracts.locate(x, y))
        if kind in (model.KIND_COUNTY, model.KIND_PROPOSAL):
            return [r[0] for r in self.counties.locate(x, y, day if day is not None else model.days(model.EPOCH),
                                                       proposals=kind == model.KIND_PROPOSAL)]
        return sorted(r['ref'] for r in self.water.find_all() if self.water.hit(r['bbox'], r['first'], r['count'], x, y))

    def find_name(self, text):
        out = {r['ref'] for r in self.water.find(text)}
        out |= {ref for ref in self.tracts.refs() if self.tracts.name_of(ref).lower() == text.lower()}
        out |= {ref for ref in self.counties.refs() if any(v['name'].lower() == text.lower() for v in self.counties.versions(ref))}
        return sorted(out)

    def history(self, ref):
        return [(v['from'], v['to'], v['horizon']) for v in self.counties.versions(ref)]

    def bbox(self, ref):
        kind = self.kind_of(ref)
        if kind == model.KIND_TRACT:
            return self.tracts.bbox_of(ref)
        return next(r['bbox'] for r in self.water.find_all() if r['ref'] == ref)

    def entities(self, kind):
        return self.tracts.refs() if kind == model.KIND_TRACT else sorted({r['ref'] for r in self.water.find_all()})
