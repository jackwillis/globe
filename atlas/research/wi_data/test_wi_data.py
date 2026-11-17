#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""EXP-WI-DATA-01 tests. Synthetic fixtures only; real-data checks need WI_DATA_CACHE (research-only)."""
import datetime
import os
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parent))
import catalog_fit  # noqa: E402
import codec  # noqa: E402
import experiment as X  # noqa: E402
import iface_c  # noqa: E402
import model  # noqa: E402
import oracle  # noqa: E402
import readers  # noqa: E402
import store_a  # noqa: E402
import store_b  # noqa: E402
import synthetic as S  # noqa: E402

D = datetime.date

def build(tmp, records=None):
    readers.EXCEPTIONS.clear()
    records = records or S.records(tmp)
    bundle = X.Bundle(records)
    probes = X.make_probes(bundle, **S.SYN_PROBES)
    return bundle, probes

def disagreements(bundle, probes):
    return X.compare(bundle, X.run_queries(bundle, probes), X.oracle_answers(bundle, probes))

class Fixture(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.tmp = self._tmp.name
        self.bundle, self.probes = build(self.tmp)

class AcsParsing(unittest.TestCase):
    def write(self, text, name='acs.psv'):
        path = Path(tempfile.mkdtemp()) / name
        path.write_text(text)
        return path

    def test_sentinels_and_decimals(self):
        rows = readers.read_acs_psv(self.write('GEO_ID|B01002_E001|B01002_M001\n1400000US99001000101|39.5|-222222222\n'), 'B01002', 10)
        self.assertEqual(rows['99001000101'], {'E': (395, None), 'M': (None, 'moe-insufficient-sample')})

    def test_unknown_sentinel_fails_closed(self):
        with self.assertRaises(ValueError):
            readers.read_acs_psv(self.write('GEO_ID|B01003_E001|B01003_M001\n1400000US99001000101|5|-444444444\n'), 'B01003')

    def test_inexact_decimal_rejected(self):
        with self.assertRaises(ValueError):
            readers.read_acs_psv(self.write('GEO_ID|B01003_E001|B01003_M001\n1400000US99001000101|5.25|1\n'), 'B01003')

    def test_header_row_kind_and_duplicates(self):
        with self.assertRaises(ValueError):
            readers.read_acs_psv(self.write('GEO_ID|B01003_E002|B01003_M002\n'), 'B01003')
        with self.assertRaises(ValueError):
            readers.read_acs_psv(self.write('GEO_ID|B01003_E001|B01003_M001\n0500000US55025|5|1\n'), 'B01003')
        with self.assertRaises(ValueError):
            readers.read_acs_psv(self.write('GEO_ID|B01003_E001|B01003_M001\n1400000US99001000101|5|1\n1400000US99001000101|6|1\n'), 'B01003')

class Identity(unittest.TestCase):
    def test_malformed_ids(self):
        for kind, value in (('geoid', '5502500070'), ('geoid', '55025O00700'), ('ahcb-id', 'Wis_Dane'), ('hydroid', '')):
            with self.assertRaises(ValueError, msg=(kind, value)):
                model.Registry.key('x', kind, '', value)

    def test_allocation_is_order_independent_and_stable(self):
        keys = [(model.Registry.key('census', 'geoid', '2020', f'9900100010{i}'), 1) for i in range(5)]
        a, b = model.Registry(), model.Registry()
        a.allocate(keys)
        b.allocate(list(reversed(keys)))
        self.assertEqual(a.refs, b.refs)
        before = dict(a.refs)
        a.allocate(keys + [(model.Registry.key('census', 'geoid', '2020', '99001000000'), 1)])
        self.assertTrue(all(a.refs[k] == v for k, v in before.items()), 'existing references never move or get reused')
        self.assertEqual(len(set(a.refs.values())), len(a.refs))

    def test_geoid_reuse_across_vintages_does_not_collide(self):
        r = model.Registry()
        r.allocate([(model.Registry.key('census', 'geoid', '2010', '99001000101'), 1),
                    (model.Registry.key('census', 'geoid', '2020', '99001000101'), 1)])
        self.assertEqual(len(r.refs), 2)
        naive = {}
        for vintage in ('2010', '2020'):
            naive['99001000101'] = vintage
        self.assertEqual(len(naive), 1, 'a bare GEOID key silently merges two vintages')

    def test_ambiguous_kind_rejected(self):
        key = model.Registry.key('ahcb', 'ahcb-id', '', 'syn_alpha')
        with self.assertRaises(ValueError):
            model.Registry().allocate([(key, 2), (key, 4)])

class Claims(unittest.TestCase):
    def c(self, **kw):
        base = dict(entity=1, prop=2, value=1, source=0, evidence='x')
        base.update(kw)
        return model.claim(**base)

    def test_rejections(self):
        with self.assertRaises(ValueError):
            self.c(valid_from=10, valid_to=10)                  # empty interval
        with self.assertRaises(ValueError):
            self.c(valid_from=11, valid_to=10)                  # inverted
        with self.assertRaises(ValueError):
            self.c(value=None, moe=5)                           # MOE without an estimate
        with self.assertRaises(ValueError):
            self.c(value=5, status='insufficient-sample')       # value with a missing reason
        with self.assertRaises(ValueError):
            self.c(prop=99)

    def test_temporal_overlap_gap_and_contradiction(self):
        a = self.c(prop=6, value=[[(0, 0)]], valid_from=0, valid_to=10)
        b = self.c(prop=6, value=[[(0, 0)]], valid_from=5, valid_to=20)
        with self.assertRaises(ValueError):
            model.check_temporal([a, b])
        gap = self.c(prop=6, value=[[(0, 0)]], valid_from=15, valid_to=20)
        self.assertEqual(model.check_temporal([a, gap]), [((1, 6), 10, 15)])
        with self.assertRaises(ValueError):
            model.check_contradictions([self.c(value=1), self.c(value=2)])
        model.check_contradictions([self.c(value=1), self.c(value=1)])

class Codecs(unittest.TestCase):
    def test_geometry_strictness(self):
        ring = [(0, 0), (0, 10), (10, 10)]
        blob = codec.pack_geometry([ring])
        self.assertEqual(codec.unpack_geometry(blob)[0], [ring])
        for cut in range(len(blob)):
            with self.assertRaises((ValueError, struct.error), msg=cut):
                codec.unpack_geometry(blob[:cut])
        with self.assertRaises(ValueError):
            codec.unpack_geometry(codec.pack_geometry([[(0, 0), (1, 1)]]))

    def test_non_finite_and_range(self):
        for bad in (float('nan'), float('inf')):
            with self.assertRaises(ValueError):
                codec.quantize_rings([[(bad, 0.0), (1.0, 1.0), (2.0, 0.0)]])
        with self.assertRaises(ValueError):
            codec.quantize(181.0)

    def test_varint_round_trip_and_bounds(self):
        for n in (0, 1, 127, 128, 2**31, 2**63 - 1):
            self.assertEqual(codec.read_varint(codec.varint(n), 0)[0], n)
        with self.assertRaises(ValueError):
            codec.read_varint(b'\x80', 0)
        for n in (-5, 0, 7, -2**31):
            self.assertEqual(codec.unzigzag(codec.zigzag(n)), n)

    def test_grid_simplicity_matches_the_quadratic_reference(self):
        import random
        rng = random.Random(7)
        for _ in range(200):
            ring = [(rng.random(), rng.random()) for _ in range(rng.randint(4, 40))]
            self.assertEqual(oracle.simple_ring_grid(ring), oracle.simple_ring_quadratic(ring))
        circle = [(codec.math.cos(i / 500 * 6.2831853), codec.math.sin(i / 500 * 6.2831853)) for i in range(500)]
        self.assertTrue(oracle.simple_ring(circle))
        self.assertFalse(oracle.simple_ring(circle[:100] + circle[250:300] + circle[100:250] + circle[300:]))

    def test_bowtie_and_quantization(self):
        self.assertFalse(oracle.simple_ring([(0, 0), (1, 1), (1, 0), (0, 1)]))
        self.assertTrue(oracle.simple_ring([(0, 0), (0, 1), (1, 1), (1, 0)]))
        q = [(codec.quantize(x), codec.quantize(y)) for x, y in [(0.0, 0.0), (1e-7, 0.0), (1.0, 1.0)]]
        self.assertEqual(q[0], q[1], 'sub-microdegree vertices collapse')
        for x in (-89.3841, 43.0747, 0.0000004, -179.9999996):
            self.assertLessEqual(abs(x - codec.quantize(x) / codec.MICRO), 5e-7 + 1e-12)

class Stores(Fixture):
    def test_wide_and_lean_agree_everywhere(self):
        for ref in self.bundle.registry.refs.values():
            for prop in model.PROPS:
                a = [dict(c, value=None if c['prop'] == 6 else c['value']) for c in self.bundle.a_wide.claims(ref, prop)]
                b = [dict(c, value=None if c['prop'] == 6 else c['value']) for c in self.bundle.a_lean.claims(ref, prop)]
                self.assertEqual(a, b)

    def test_truncated_packs_rejected(self):
        for store in (self.bundle.a_wide, self.bundle.stats):
            for cut in (0, 8, len(store.data) - 1):
                clone = object.__new__(type(store))
                clone.data = store.data[:cut]
                with self.assertRaises((ValueError, struct.error)):
                    (clone._load if hasattr(clone, '_load') else clone._open)()

    def test_stats_layer_keeps_census_codes_in_band(self):
        ref = next(r for r in self.bundle.sets['tract'] if self.bundle.tracts.name_of(r) == 'Synthetic Tract 3')
        est, moe = self.bundle.stats.get_raw(ref, 3)
        self.assertEqual((est, moe), (-666666666, -222222222))

class Queries(Fixture):
    def test_all_models_match_each_other_and_the_oracle(self):
        self.assertEqual(disagreements(self.bundle, self.probes), [])
        self.assertGreaterEqual(len(self.probes['county']), 20)

    def test_proposal_never_answers_county_queries(self):
        res = X.run_queries(self.bundle, self.probes)
        for name in res:
            hit = res[name]['q3:alpha@1848-06-01']
            self.assertEqual(hit, [('syn_alpha', '1840-01-01')], name)

    def test_point_inside_bbox_but_outside_polygon_is_not_a_hit(self):
        res = X.run_queries(self.bundle, self.probes)
        for name in res:
            self.assertEqual(res[name]['q3:notch@1848-06-01'], [('syn_beta', '1840-01-01')], name)
            self.assertEqual(res[name]['q3:notch@1851-01-01'], [], name)   # in alpha v2's bbox, in its notch

    def test_hole_is_subtracted(self):
        for name, ins in self.bundle.inspectors.items():
            self.assertEqual(len(ins.locate(-89.397, 43.056, model.KIND_WATERBODY)), 1, name)
            self.assertEqual(ins.locate(-89.3935, 43.0585, model.KIND_WATERBODY), [], name)

    def test_gap_returns_nothing_not_the_nearest_version(self):
        res = X.run_queries(self.bundle, self.probes)
        for name in res:
            self.assertEqual(res[name]['q3:gamma@1846-01-01'], [], name)
            self.assertEqual(res[name]['q3:gamma@1845-12-31'], [('syn_gamma', '1840-01-01')], name)
        self.assertEqual(len(self.bundle.bookkeeping['temporal_gaps']), 3)

    def test_interval_edges_are_inclusive_at_source_and_half_open_inside(self):
        res = X.run_queries(self.bundle, self.probes)['C']
        self.assertEqual(res['q3:alpha@1850-12-31'], [('syn_alpha', '1840-01-01')])
        self.assertEqual(res['q3:alpha@1851-01-01'], [('syn_alpha', '1851-01-01')])

    def test_same_wbic_on_two_features_stays_two_entities(self):
        ins = self.bundle.inspectors['C']
        refs = ins.find_name('Synthetic Lake One')
        self.assertEqual(len(refs), 2)
        self.assertEqual(len({self.bundle.keys[r] for r in refs}), 2)

    def test_ambiguous_names_return_every_candidate(self):
        records = S.records(self.tmp)
        records['tracts'][1] = dict(records['tracts'][1], name=records['tracts'][0]['name'])
        bundle, _ = build(self.tmp, records)
        for ins in bundle.inspectors.values():
            self.assertEqual(len(ins.find_name('Synthetic Tract 1')), 2)

    def test_missing_and_censored_values_are_not_numbers(self):
        for name, ins in self.bundle.inspectors.items():
            v = ins.value(self.probes['missing'], 3)
            self.assertEqual((v['status'], v['value']), ('insufficient-sample', None), name)
            c = ins.value(self.probes['censored'], self.probes['censored_prop'])
            self.assertTrue(c['censored'] and c['moe'] is None, name)
            text = '\n'.join(ins.describe(self.probes['censored'])['lines'])
            self.assertIn('at least 250,000', text)
            self.assertNotIn('250,001', text)

    def test_describe_is_bounded_and_keeps_provenance(self):
        ins = self.bundle.inspectors['C']
        for budget, lines in ((1024, 24), (200, 24), (1024, 6)):
            d = ins.describe(self.probes['q1'], budget=budget, max_lines=lines)
            self.assertLessEqual(d['bytes'], max(budget, 0) + 200, (budget, lines))   # head and provenance are never dropped
            self.assertLessEqual(len(d['lines']), max(lines, 6) + 3)
            self.assertTrue(any(l.startswith('source:') for l in d['lines']))
            self.assertEqual(d['truncated'], budget < 600 or lines < 12)
        self.assertEqual(ins.describe(0xF1000000)['status'], 'unknown')

    def test_reference_period_is_stated_once_and_survives_in_every_model(self):
        for name, ins in self.bundle.inspectors.items():
            lines = ins.describe(self.probes['q1'])['lines']
            self.assertIn('period: 2018-01-01..2022-12-31', lines, name)
            self.assertEqual(sum(l.startswith('period:') for l in lines), 1)
        self.assertEqual(self.bundle.stats.period, model.ACS_PERIOD)

    def test_describe_is_identical_across_models(self):
        lines = [i.describe(self.probes['q1'])['lines'] for i in self.bundle.inspectors.values()]
        self.assertEqual(lines[0], lines[1])
        self.assertEqual(lines[0], lines[2])

    def test_cross_layer_join_matches_oracle(self):
        truth = X.oracle_answers(self.bundle, self.probes)['q5']
        self.assertEqual(sorted(self.bundle.inspectors['C'].intersecting(self.probes['lake'], model.KIND_TRACT)) and
                         X.run_queries(self.bundle, self.probes)['C']['q5'], truth)

class SeededDefects(Fixture):
    """A passing suite proves little unless known-bad variants fail it."""

    def detected(self, patcher):
        with patcher:
            try:
                bundle, probes = build(self.tmp)
                return bool(disagreements(bundle, probes))
            except Exception:
                return True

    def test_off_by_one_closed_interval_conversion(self):
        real = model.claim
        def shifted(*a, **kw):
            if kw.get('valid_to') is not None and kw.get('horizon') is not None:
                kw['valid_to'] -= 1
            return real(*a, **kw)
        self.assertTrue(self.detected(mock.patch.object(model, 'claim', shifted)))

    def test_proposals_treated_as_counties(self):
        self.assertTrue(self.detected(mock.patch.object(model, '_county_kind', lambda c: model.KIND_COUNTY)))

    def test_point_in_polygon_always_true(self):
        self.assertTrue(self.detected(mock.patch.object(codec, 'contains', lambda rings, x, y: (True, 0))))

    def test_open_interval_censoring_ignored(self):
        real = store_b.acs_decode
        mutant = lambda est, moe, top: real(est, moe, None)
        with mock.patch.object(store_b, 'acs_decode', mutant):
            bundle, probes = build(self.tmp)
            self.assertTrue(disagreements(bundle, probes))

    def test_a_store_ignoring_entity_kind(self):
        with mock.patch.object(store_a.WideStore, 'kind_of', lambda self, e: model.KIND_COUNTY):
            bundle, probes = build(self.tmp)
            self.assertTrue(disagreements(bundle, probes))

class CatalogFit(unittest.TestCase):
    def test_existing_contract_limits(self):
        got = dict(catalog_fit.attempts())
        self.assertEqual(got['identity, time-qualified names, successor assertion, once the lock lists the sources'], 'accepted')
        self.assertTrue(got['same document against the real lock directory (sources not locked there)'].startswith('rejected'))
        self.assertTrue(got['entity kind "tract"'].startswith('rejected'))
        self.assertTrue(got['an ACS estimate carried as a name with role "estimate"'].startswith('rejected'))
        self.assertTrue(got['a statistic as an assertion with relation "statistic"'].startswith('rejected'))
        self.assertEqual(got['a new top-level "statistics" table with invalid content'], 'accepted')   # hazard: unvalidated extras pass
        self.assertEqual(got['geometry carried by reference (entity to source record)'], 'accepted')

class Reproducibility(unittest.TestCase):
    def test_two_builds_are_byte_identical(self):
        with tempfile.TemporaryDirectory() as tmp:
            a, _ = build(tmp)
            b, _ = build(tmp)
        for name in ('a_wide', 'a_lean', 'stats', 'tracts', 'counties', 'water'):
            self.assertEqual(getattr(a, name).data, getattr(b, name).data, name)
        self.assertEqual((a.desc, a.prov, a.ident), (b.desc, b.prov, b.ident))

    def test_descriptor_and_provenance_round_trip(self):
        with tempfile.TemporaryDirectory() as tmp:
            b, _ = build(tmp)
        layers = iface_c.decode_descriptors(b.desc)
        self.assertEqual([l['kind'] for l in layers], [1, 2, 3])
        self.assertEqual([s['id'] for s in iface_c.decode_provenance(b.prov)], [s[0] for s in model.SOURCES])
        self.assertLess(max(len(iface_c.encode_descriptors([(1, 1, 0, 1, 1, [1, 2, 3, 4, 5, 6])])), 0), 2048)
        self.assertLess(len(b.prov), 1024)

@unittest.skipUnless(os.environ.get('WI_DATA_CACHE'), 'research-only real data cache not provided')
class RealData(unittest.TestCase):
    def test_real_sample_agrees_with_oracle(self):
        readers.EXCEPTIONS.clear()
        bundle = X.Bundle(X.load_real(os.environ['WI_DATA_CACHE']))
        probes = X.make_probes(bundle)
        self.assertGreaterEqual(len(probes['county']), 20)
        self.assertEqual(disagreements(bundle, probes), [])

if __name__ == '__main__':
    unittest.main()
