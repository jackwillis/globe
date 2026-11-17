#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""EXP-WI-DATA-01 driver: build every model, run the preregistered queries, measure.

    python3 experiment.py --synthetic OUT.json
    python3 experiment.py --real CACHE OUT.json

Real mode needs the research-only cache from acquire.py. Raw and per-record real data never
leave that cache; the output holds aggregates only. Host measurements, not G4 evidence.
"""
import argparse
import datetime
import gzip
import hashlib
import json
from pathlib import Path
import struct
import sys
import time
import tracemalloc

import codec
import iface_adapters
import iface_c
import model
import oracle
import readers
import store_a
import store_b

def gz(data):
    return len(gzip.compress(data, 9, mtime=0))

def load_real(cache):
    cache = Path(cache)
    tracts = readers.read_tracts(cache / 'cb_2022_55_tract_500k.zip')
    acs = {t: readers.read_acs_psv(cache / f'acs2022_{t.lower()}_dane_tracts.psv', t, 10 if t == 'B01002' else 1)
           for t in model.ACS_PROPS}
    counties = readers.read_ahcb(cache / 'WI_AtlasHCB.zip')
    lakes = readers.read_dnr(cache / 'wdnr_24k_waterbody_dane.json')
    return {'tracts': tracts, 'acs': acs, 'counties': counties, 'lakes': lakes, 'mode': 'real'}

class Bundle:
    def __init__(self, records, claims=None, registry=None):
        self.records = records
        if registry is None:
            registry, claims, self.bookkeeping = model.build_claims(
                records['tracts'], records['acs'], records['counties'], records['lakes'])
        self.registry, self.claims = registry, claims
        self.keys = {v: k for k, v in registry.refs.items()}
        kinds = registry.kinds
        self.kinds = kinds
        by_kind = lambda k: {r for r, kk in kinds.items() if kk == k}
        self.sets = {'tract': by_kind(model.KIND_TRACT), 'county': by_kind(model.KIND_COUNTY) | by_kind(model.KIND_PROPOSAL), 'water': by_kind(model.KIND_WATERBODY)}
        self.a_wide = store_a.WideStore(claims, kinds)
        self.a_lean = store_a.LeanStore(claims, kinds)
        self._build_b()
        self.desc = iface_c.encode_descriptors([
            (1, model.KIND_TRACT, 0, 1, 1, [1, 2, 3, 4, 5, 6]),
            (2, model.KIND_COUNTY, 1, 1, 0, [1, 6, 8]),
            (3, model.KIND_WATERBODY, 0, 1, 0, [1, 6, 7])])
        self.prov = iface_c.encode_provenance(model.SOURCES)
        self.ident = b''.join(struct.pack('<I', r) + iface_c._pstr(k) for r, k in sorted(self.keys.items()))
        ids = {}
        pos = 0
        while pos < len(self.ident):
            (ref,) = struct.unpack_from('<I', self.ident, pos)
            key, pos = iface_c._read_pstr(self.ident, pos + 4)
            ids[ref] = key
        self.identities = ids
        layers = iface_c.decode_descriptors(self.desc)
        self.inspectors = {
            'A-wide': iface_c.Inspector(iface_adapters.ABackend(self.a_wide), self.desc, self.prov, ids),
            'A-lean': iface_c.Inspector(iface_adapters.ABackend(self.a_lean), self.desc, self.prov, ids),
            'C': iface_c.Inspector(iface_adapters.BBackend(self.stats, self.tracts, self.counties, self.water, layers),
                                   self.desc, self.prov, ids)}

    def _build_b(self):
        by = {}
        for c in self.claims:
            by.setdefault(c['entity'], []).append(c)
        stats = {r: {c['prop']: c for c in by[r] if c['prop'] in (2, 3, 4, 5)} for r in self.sets['tract']}
        self.stats = store_b.StatsLayer(stats, [2, 3, 4, 5])
        self.tracts = store_b.TractLayer([(r, self._one(by[r], 1), self._one(by[r], 6)) for r in sorted(self.sets['tract'])])
        items = []
        for r in sorted(self.sets['county']):
            names = {c['from']: c for c in by[r] if c['prop'] == 1}
            notes = {c['from']: c for c in by[r] if c['prop'] == 8}
            for c in by[r]:
                if c['prop'] == 6:
                    version = int(c['evidence'].rsplit(' v', 1)[1])
                    items.append((r, version, c['from'], c['to'], c['horizon'], names[c['from']]['value'],
                                  notes[c['from']]['value'], c['value'], self.kinds[r] == model.KIND_PROPOSAL))
        self.counties = store_b.CountyLayer(items)
        self.water = store_b.WaterLayer([(r, int(self.keys[r].rsplit(':', 1)[1]),
                                          int(self._one(by[r], 7).split()[1]), self._one(by[r], 1), self._one(by[r], 6))
                                         for r in sorted(self.sets['water'])])

    @staticmethod
    def _one(claims, prop):
        values = [c['value'] for c in claims if c['prop'] == prop]
        if len(values) != 1:
            raise ValueError('expected exactly one claim')
        return values[0]

    def subset_stores(self):
        """A stores over the same claims restricted to a data category (for P1/P2 byte comparison)."""
        out = {}
        specs = {'stats': ('tract', {2, 3, 4, 5}), 'tract-geometry': ('tract', {1, 6}),
                 'counties': ('county', {1, 6, 8}), 'water': ('water', {1, 6, 7})}
        for name, (kind, props) in specs.items():
            claims = [c for c in self.claims if c['entity'] in self.sets[kind] and c['prop'] in props]
            kinds = {r: self.kinds[r] for r in self.sets[kind]}
            out[name] = (store_a.WideStore(claims, kinds), store_a.LeanStore(claims, kinds))
        return out

    def packed_totals(self):
        b = {'stats': self.stats.packed_bytes, 'tract-geometry': self.tracts.packed_bytes,
             'counties': self.counties.packed_bytes, 'water': self.water.packed_bytes}
        return b

def run_queries(bundle, probes):
    """Each inspector answers every query; results are normalised for cross-model comparison."""
    out = {}
    for name, ins in bundle.inspectors.items():
        r = {}
        for label, point, day in probes['county']:
            r[f'q3:{label}'] = sorted((bundle.keys[ref].split(':')[-1], model.from_days(lo).isoformat())
                                      for ref, lo, hi in _county_hits(ins, point, day))
        r['q1'] = ins.describe(probes['q1'])
        r['q2'] = [(bundle.keys[x].split(':')[-1]) for x in ins.locate(*probes['capitol'], model.KIND_TRACT)]
        r['q4'] = [(bundle.keys[x], [l for l in ins.describe(x)['lines'] if l.startswith('source feature id')]) for x in ins.find_name(probes['lake_name'])]
        r['q5'] = sorted(bundle.keys[x].split(':')[-1] for x in ins.intersecting(probes['lake'], model.KIND_TRACT))
        r['q6'] = ins.history(probes['dane'])
        r['q7'] = ins.value(probes['missing'], 3)
        r['q7b'] = ins.value(probes['censored'], probes['censored_prop']) if probes.get('censored') else None
        out[name] = r
    return out

def _county_hits(ins, point, day):
    x, y = codec.quantize(point[0]), codec.quantize(point[1])
    # The inspector exposes locate(lon, lat, kind, at); counties return (ref) only, so recover intervals.
    refs = ins.locate(point[0], point[1], model.KIND_COUNTY, model.days(day))
    out = []
    for ref in refs:
        for lo, hi, _ in ins.history(ref):
            if lo <= model.days(day) < hi:
                out.append((ref, lo, hi))
    return out

CAPITOL = (-89.3841, 43.0747)   # Wisconsin State Capitol, Madison (public coordinates; used as a query point only)
CITIES = {'madison': (-89.3841, 43.0747), 'milwaukee': (-87.9065, 43.0389), 'green-bay': (-88.0133, 44.5133),
          'superior': (-92.1041, 46.7208), 'prairie-du-chien': (-91.1426, 43.0525), 'eau-claire': (-91.4985, 44.8113)}
PREREG_DATES = [datetime.date(1840, 1, 1), datetime.date(1848, 6, 1), datetime.date(1850, 1, 1), datetime.date(2000, 1, 1)]

def make_probes(bundle, cities=CITIES, capitol=CAPITOL, lake_name='Lake Mendota', dane_id='wis_dane', dates=PREREG_DATES,
                boundary_city='madison'):
    rec = bundle.records
    ref_of = lambda suffix: next(r for k, r in bundle.registry.refs.items() if k.endswith(suffix))
    county_probes = [(f'{c}@{d}', p, d) for c, p in cities.items() for d in dates]
    m = cities[boundary_city]
    boundary = set()
    for c in rec['counties']:
        if c['cnty_type'] == 'County' and oracle.inside(c['rings'], *m):
            for d in (c['start'], c['end']):
                boundary.update({d - datetime.timedelta(days=1), d, d + datetime.timedelta(days=1)})
    county_probes += [(f'{boundary_city}-boundary@{d}', m, d) for d in sorted(boundary) if d.year <= 2000]
    tract_ok = next(r for r in sorted(bundle.sets['tract']) if all(bundle.a_wide.claims(r, p)[0]['status'] == 'ok' for p in (2, 3, 4, 5)))
    missing = next((r for r in sorted(bundle.sets['tract']) if any(bundle.a_wide.claims(r, p)[0]['status'] != 'ok' for p in (3, 4))), tract_ok)
    cens = next(((r, p) for r in sorted(bundle.sets['tract']) for p in (2, 3, 4, 5) if bundle.a_wide.claims(r, p)[0]['censored']), None)
    lake = next(r for r in bundle.sets['water'] if bundle.a_wide.claims(r, 1)[0]['value'] == lake_name)
    return {'county': county_probes, 'lake_name': lake_name, 'q1': tract_ok, 'capitol': capitol, 'lake': lake, 'dane': ref_of(f'ahcb-id@:{dane_id}'),
            'missing': missing, 'censored': cens[0] if cens else None, 'censored_prop': cens[1] if cens else None}

def oracle_answers(bundle, probes):
    rec = bundle.records
    out = {f'q3:{label}': oracle.county_at(rec['counties'], p[0], p[1], d) for label, p, d in probes['county']}
    out['q3'] = {k: [(i, s.isoformat()) for i, s in v] for k, v in out.items()}
    out = {k: [(i, s.isoformat()) for i, s in v] for k, v in out.items() if k != 'q3'}
    out['q2'] = oracle.tract_at(rec['tracts'], *probes['capitol'])
    lake = next(l for l in rec['lakes'] if bundle.keys[probes['lake']].endswith(str(l['hydroid'])))
    out['q5'] = [g for g in oracle.tracts_near(rec['tracts'], lake)]
    return out

def compare(bundle, results, truth):
    """P5 and the cross-model equality checks. Returns (n_county_probes, disagreements)."""
    names = list(results)
    bad = []
    for key in results[names[0]]:
        if key == 'q1':
            continue
        vals = [json.dumps(results[n][key], sort_keys=True, default=str) for n in names]
        if len(set(vals)) != 1:
            bad.append(('models-differ', key))
    for n in names:
        for key, expected in truth.items():
            if json.dumps(results[n][key], default=str) != json.dumps(expected, default=str):
                bad.append(('oracle-differs', n, key))
    for n in names[1:]:
        if results[n]['q1']['lines'] != results[names[0]]['q1']['lines']:
            bad.append(('describe-differs', n))
    return bad

def lookups(bundle):
    """P3: probes per (entity, property) statistic lookup. Deterministic counts."""
    sub = bundle.subset_stores()['stats']
    rows = {}
    for label, store in (('A-wide stats-only', sub[0]), ('A-lean stats-only', sub[1]), ('A-wide full', bundle.a_wide),
                         ('A-lean full', bundle.a_lean)):
        store.probes.__init__()
        n = 0
        for r in sorted(bundle.sets['tract']):
            for p in (2, 3, 4, 5):
                store.claims(r, p)
                n += 1
        rows[label] = {'lookups': n, 'compare_per_lookup': round(store.probes.compare / n, 2),
                       'decode_per_lookup': round(store.probes.decode / n, 2)}
    bundle.stats.counter.__init__()
    n = 0
    for r in sorted(bundle.sets['tract']):
        for p in (2, 3, 4, 5):
            bundle.stats.get_raw(r, p)
            n += 1
    rows['B stats (binary search on refs)'] = {'lookups': n, 'compare_per_lookup': round(bundle.stats.counter.compare / n, 2),
                                               'decode_per_lookup': 0.0}
    rows['B stats (dense refs, direct index; assumption)'] = {'lookups': n, 'compare_per_lookup': 1.0, 'decode_per_lookup': 0.0}
    return rows

def memory_and_time(bundle, probes, repeats=2):
    out = {}
    for name, ins in bundle.inspectors.items():
        tracemalloc.start()
        before = tracemalloc.get_traced_memory()[0]
        t0 = time.perf_counter()
        for _ in range(repeats):
            res = run_queries_single(bundle, ins, probes)
        elapsed = (time.perf_counter() - t0) / repeats
        peak = tracemalloc.get_traced_memory()[1] - before
        tracemalloc.stop()
        out[name] = {'query_suite_ms_host': round(elapsed * 1000, 1), 'python_peak_bytes_host': peak}
    return out

def run_queries_single(bundle, ins, probes):
    for label, point, day in probes['county']:
        _county_hits(ins, point, day)
    ins.describe(probes['q1'])
    ins.locate(*probes['capitol'], model.KIND_TRACT)
    ins.find_name(probes['lake_name'])
    ins.intersecting(probes['lake'], model.KIND_TRACT)

def geometry_integrity(bundle):
    rec = bundle.records
    sources = {'tracts': [t['rings'] for t in rec['tracts']], 'counties': [c['rings'] for c in rec['counties']],
               'lakes': [l['rings'] for l in rec['lakes']]}
    out = {}
    for name, items in sources.items():
        worst, vertices, collapsed, rings_total, checked, new_cross, raw_cross, skipped = 0.0, 0, 0, 0, 0, 0, 0, 0
        for rings in items:
            for ring in rings:
                rings_total += 1
                ring = codec.drop_closing(ring)
                q = [(codec.quantize(x), codec.quantize(y)) for x, y in ring]
                vertices += len(ring)
                for (x, y), (qx, qy) in zip(ring, q):
                    worst = max(worst, abs(x - qx / codec.MICRO), abs(y - qy / codec.MICRO))
                dedup = [p for i, p in enumerate(q) if i == 0 or p != q[i - 1]]
                collapsed += len(q) - len(dedup)
                checked += 1
                raw_simple = oracle.simple_ring(ring)
                q_simple = oracle.simple_ring([(a / codec.MICRO, b / codec.MICRO) for a, b in dedup]) if len(dedup) >= 3 else False
                raw_cross += (not raw_simple)
                new_cross += (raw_simple and not q_simple)
        out[name] = {'rings': rings_total, 'vertices': vertices, 'max_abs_error_degrees': worst,
                     'collapsed_vertices': collapsed, 'rings_checked_simple': checked,
                     'rings_nonsimple_in_source': raw_cross, 'rings_made_nonsimple_by_quantization': new_cross}
    return out

def sentinel_census(bundle):
    """P6: which uncertainty/missing classes occur, and does collapsing them change a query answer?"""
    classes = {}
    for c in bundle.claims:
        if c['prop'] in (2, 3, 4, 5):
            key = (c['status'], c['moe_status'], c['censored'])
            classes[key] = classes.get(key, 0) + 1
    out = {f'{s}|{m}|{"censored" if c else "-"}': n for (s, m, c), n in sorted(classes.items())}
    collapsed_wrong = sum(1 for c in bundle.claims if c['prop'] in (2, 3, 4, 5) and c['censored'])
    return {'classes': out, 'estimates_that_would_print_a_false_exact_value_if_collapsed': collapsed_wrong}

def describe_all(bundle):
    ins = bundle.inspectors['C']
    worst_bytes = worst_lines = truncated = missing_source = 0
    for ref in bundle.registry.refs.values():
        d = ins.describe(ref)
        worst_bytes, worst_lines = max(worst_bytes, d['bytes']), max(worst_lines, len(d['lines']))
        truncated += d['truncated']
        missing_source += not any(l.startswith('source:') for l in d['lines'])
    return {'entities': len(bundle.registry.refs), 'max_bytes': worst_bytes, 'max_lines': worst_lines,
            'truncated': truncated, 'missing_provenance_line': missing_source}

def decode_accounting(bundle, probes):
    """Points decoded by the whole query suite, and the largest single feature a query can decode."""
    out = {}
    for name, ins in bundle.inspectors.items():
        stores = [bundle.a_wide if name == 'A-wide' else bundle.a_lean] if name != 'C' else [bundle.tracts, bundle.counties, bundle.water]
        for s in stores:
            for field in ('probes', 'counter'):
                if hasattr(s, field):
                    getattr(s, field).__init__()
        run_queries_single(bundle, ins, probes)
        out[name] = {'points_decoded_one_suite': sum((s.probes.points if hasattr(s, 'probes') else s.counter.points) for s in stores),
                     'edges_tested_one_suite': sum((s.probes.edges if hasattr(s, 'probes') else s.counter.edges) for s in stores)}
    rec = bundle.records
    biggest = max([sum(len(r) for r in c['rings']) for c in rec['counties']] + [sum(len(r) for r in t['rings']) for t in rec['tracts']] +
                  [sum(len(r) for r in l['rings']) for l in rec['lakes']])
    out['largest_single_feature_points'] = biggest
    out['largest_single_feature_bytes_if_decoded_as_int32_pairs'] = 8 * biggest
    return out

def code_lines():
    def count(path, start=None, stop=None):
        lines = Path(__file__).with_name(path).read_text().splitlines()
        if start:
            a = next(i for i, l in enumerate(lines) if l.startswith(start))
            b = next((i for i, l in enumerate(lines) if i > a and l.startswith(stop)), len(lines)) if stop else len(lines)
            lines = lines[a:b]
        return sum(1 for l in lines if l.strip() and not l.strip().startswith('#') and not l.strip().startswith('"""'))
    shared = count('iface_c.py')
    a_glue = count('iface_adapters.py', 'class ABackend', 'class BBackend')
    b_glue = count('iface_adapters.py', 'class BBackend')
    return {'shared_inspector_lines': shared, 'A_backend_lines': a_glue, 'C_backend_lines': b_glue,
            'A_shared_fraction': round(shared / (shared + a_glue), 3), 'C_shared_fraction': round(shared / (shared + b_glue), 3),
            'B_native_shared_fraction': 0.0,
            'store_a_lines': count('store_a.py'), 'store_b_lines': count('store_b.py'),
            'store_b_polygon_layouts_lines': count('store_b.py', 'class TractLayer'),
            'codec_lines': count('codec.py'), 'readers_lines': count('readers.py'), 'model_lines': count('model.py')}

def run_all(loader, probes_kw, timing=True, integrity=True):
    readers.EXCEPTIONS.clear()
    records = loader()
    bundle = Bundle(records)
    probes = make_probes(bundle, **probes_kw)
    results = run_queries(bundle, probes)
    truth = oracle_answers(bundle, probes)
    bad = compare(bundle, results, truth)
    sub = bundle.subset_stores()
    b_bytes = bundle.packed_totals()
    ratios = {}
    for key, store in sub.items():
        wide, lean = store
        kind_rows = len(bundle.sets[{'stats': 'tract', 'tract-geometry': 'tract', 'counties': 'county', 'water': 'water'}[key]])
        ratios[key] = {'A-wide_without_entity_kind_rows/B': round((wide.packed_bytes - 32 * kind_rows) / b_bytes[key], 3),
                       'A-wide': wide.packed_bytes, 'A-lean': lean.packed_bytes, 'B': b_bytes[key],
                       'A-wide/B': round(wide.packed_bytes / b_bytes[key], 3), 'A-lean/B': round(lean.packed_bytes / b_bytes[key], 3),
                       'gzip': {'A-wide': gz(wide.data), 'A-lean': gz(lean.data),
                                'B': gz({'stats': bundle.stats, 'tract-geometry': bundle.tracts,
                                         'counties': bundle.counties, 'water': bundle.water}[key].data)}}
    inspect_overhead = {'descriptors': len(bundle.desc), 'provenance': len(bundle.prov), 'identity_table': len(bundle.ident)}
    totals = {'A-wide': bundle.a_wide.packed_bytes, 'A-lean': bundle.a_lean.packed_bytes,
              'B_layers_only': sum(b_bytes.values()),
              'A-wide_with_inspection_blocks': bundle.a_wide.packed_bytes + sum(inspect_overhead.values()),
              'A-lean_with_inspection_blocks': bundle.a_lean.packed_bytes + sum(inspect_overhead.values()),
              'C_total': sum(b_bytes.values()) + sum(inspect_overhead.values())}
    bulk_change = round(100.0 * sum(inspect_overhead.values()) / sum(b_bytes.values()), 4)
    return {
        'mode': records['mode'],
        'counts': {'tracts': len(records['tracts']), 'tract_vertices': sum(len(r) for t in records['tracts'] for r in t['rings']),
                   'county_versions': len(records['counties']), 'county_entities': len({c['id'] for c in records['counties']}),
                   'county_vertices': sum(len(r) for c in records['counties'] for r in c['rings']),
                   'waterbody_features': len(records['lakes']), 'waterbody_vertices': sum(len(r) for l in records['lakes'] for r in l['rings']),
                   'entities': len(bundle.registry.refs), 'claims': len(bundle.claims),
                   'acs_cells': 4 * len(records['tracts'])},
        'sizes_bytes': {'by_category': ratios, 'inspection_blocks': inspect_overhead, 'totals': totals,
                        'inspection_blocks_as_percent_of_B_bulk': bulk_change},
        'lookups': lookups(bundle),
        'queries': {'county_probes': len(probes['county']), 'disagreements': bad,
                    'models_compared': list(results), 'oracle': 'raw float winding + closed source dates'},
        'sentinels': sentinel_census(bundle),
        'geometry_integrity': geometry_integrity(bundle) if integrity else None,
        'source_quirks_recorded': dict(sorted(readers.EXCEPTIONS.items())),
        'temporal_gaps': len(bundle.bookkeeping['temporal_gaps']) // 3,
        'code': code_lines(),
        'describe': describe_all(bundle),
        'decode_accounting': decode_accounting(bundle, probes),
        'host_timing': memory_and_time(bundle, probes) if timing else None,
        '_fingerprints': {'A-wide': hashlib.sha256(bundle.a_wide.data).hexdigest(), 'A-lean': hashlib.sha256(bundle.a_lean.data).hexdigest(),
                          'stats': hashlib.sha256(bundle.stats.data).hexdigest(), 'tracts': hashlib.sha256(bundle.tracts.data).hexdigest(),
                          'counties': hashlib.sha256(bundle.counties.data).hexdigest(), 'water': hashlib.sha256(bundle.water.data).hexdigest(),
                          'descriptors': hashlib.sha256(bundle.desc).hexdigest()},
    }

def main():
    import catalog_fit
    import synthetic
    import tempfile
    parser = argparse.ArgumentParser()
    parser.add_argument('output')
    parser.add_argument('--real', help='cache directory from acquire.py')
    args = parser.parse_args()
    if args.real:
        results = [run_all(lambda: load_real(args.real), {}), run_all(lambda: load_real(args.real), {}, timing=False, integrity=False)]
    else:
        with tempfile.TemporaryDirectory() as tmp:
            results = [run_all(lambda: synthetic.records(tmp), synthetic.SYN_PROBES), run_all(lambda: synthetic.records(tmp), synthetic.SYN_PROBES, timing=False, integrity=False)]
    first = results[0]
    first['reproducible_byte_identical'] = results[0]['_fingerprints'] == results[1]['_fingerprints']
    first['catalog_fit'] = [{'attempt': a, 'result': r} for a, r in catalog_fit.attempts()]
    Path(args.output).write_text(json.dumps(first, indent=1, sort_keys=True, default=str) + '\n')
    print(json.dumps({k: first[k] for k in ('mode', 'queries', 'reproducible_byte_identical')}, indent=1, default=str))

if __name__ == '__main__':
    main()
