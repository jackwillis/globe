#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Adversarial invariants for #91's research prototype; synthetic roles are not approvals."""
from dataclasses import replace
import itertools
import math
import unittest

from country_selection import (Parameters, Place, basis, distance, duplicate_audit,
                               population, reviewed_seeds, score, select)


def p(key, lon=0, lat=0, pop=1000000, country='AAA', name=None, **kwargs):
    return Place(str(key), country, name or str(key), lon, lat, pop, **kwargs)


class CountrySelectionTests(unittest.TestCase):
    def test_input_order_and_ties_all_policies(self):
        places = [p('seed', pop=1), p('b', 5), p('a', -5), p('foreign', 9, country='BBB')]
        for policy in ('population', 'inverse-square', 'nearest'):
            args = dict(seeds={'seed': 'approved-test'}, country_limits={'AAA': 2, 'BBB': 1})
            expected = select(places, policy, **args)
            self.assertIn('a', expected)
            self.assertNotIn('b', expected)
            for order in itertools.permutations(places):
                self.assertEqual(expected, select(order, policy, **args))

    def test_multiple_seeds_and_no_eligible_secondary(self):
        places = [p(1, pop=None), p(2, 1, pop=1), p(3, 2, pop=None), p(4, 3, country='BBB', pop=1)]
        seeds = {'1': 'approved-national', '2': 'approved-multiple-function'}
        for policy in ('population', 'inverse-square', 'nearest'):
            self.assertEqual(set(select(places, policy, seeds)), {'1', '2'})

    def test_identical_coordinates_finite_and_different_names_survive(self):
        a, b = p(1), p(2)
        self.assertEqual(distance(a, b), 0)
        self.assertTrue(math.isfinite(score(a, [b], 'inverse-square', Parameters())))
        self.assertEqual(set(select([a, b], 'population')), {'1', '2'})
        self.assertEqual(score(a, [b], 'nearest', Parameters()), 0)

    def test_dateline_poles_and_antipodes(self):
        self.assertAlmostEqual(distance(p(1, 179), p(2, -179)), 222.39016, places=4)
        self.assertAlmostEqual(distance(p(1, 0, 90), p(2, 120, 90)), 0, places=8)
        self.assertAlmostEqual(distance(p(1), p(2, 180)), math.pi*6371.0088, places=6)

    def test_population_validation(self):
        for v in ('', None, '-99', 'nan', 'inf', '0', 'bad'):
            self.assertIsNone(population(v))
        self.assertEqual(population('200'), 200)
        self.assertEqual(select([p(1, pop=None)], 'population'), {})
        for v in (-1, float('nan'), float('inf')):
            with self.assertRaises(ValueError):
                select([p(1, pop=v)], 'population')

    def test_penalty_monotonicity(self):
        candidate = p(1)
        params = Parameters()
        near, far = p(2, .1), p(3, 10)
        for policy in ('inverse-square', 'nearest'):
            self.assertLess(score(candidate, [near], policy, params), score(candidate, [far], policy, params))
        self.assertLess(score(candidate, [near, far], 'inverse-square', params), score(candidate, [near], 'inverse-square', params))
        self.assertLess(score(candidate, [replace(near, population=2000000)], 'inverse-square', params), score(candidate, [near], 'inverse-square', params))

    def test_stop_thresholds_and_separate_budget(self):
        places = [p(1, pop=49999), p(2, 30, pop=50000), p(3, 60, pop=100000)]
        chosen = select(places, 'population')
        self.assertEqual(set(chosen), {'2', '3'})
        self.assertEqual(set(select(places, 'population', params=Parameters(min_score=.051))), {'3'})
        self.assertEqual(len(select(places, 'population', budget_bytes=34)), 1)
        with self.assertRaises(ValueError):
            select(places, 'population', {'1': 'seed', '2': 'seed'}, budget_bytes=34)

    def test_country_and_cross_border_competition(self):
        a, b = p(1), p(2, .1, country='BBB')
        self.assertEqual(score(a, [b], 'inverse-square', Parameters()), 1)
        self.assertLess(score(a, [b], 'inverse-square', Parameters(cross_border=True)), 1)
        self.assertEqual(score(a, [replace(b, population=999999)], 'inverse-square', Parameters(cross_border=True)), 1)

    def test_no_silent_identity_merging(self):
        places = [p(1, name='Springfield'), p(2, 70, name='Springfield')]
        self.assertEqual(duplicate_audit(places, 'hash')['pair_evidence'], [])
        self.assertEqual(len(select(places, 'population')), 2)
        with self.assertRaises(ValueError):
            select([p(1), p(1, 20)], 'population')

    def test_duplicate_classes_shared_qid_and_cross_country(self):
        places = [p(1, name='São City', qid='Q1'), p(2, .01, name='Sao City', qid='Q1', country='BBB'),
                  p(3, 80, name='Other', qid='Q1'), p(4, .001, name='Different')]
        qa = duplicate_audit(places, 'hash')
        self.assertEqual(qa, duplicate_audit(places[::-1], 'hash'))
        self.assertEqual(qa['reason_counts']['shared-source-qid'], 3)
        self.assertGreater(qa['reason_counts']['cross-country-attribution-review'], 0)
        self.assertIn('probable-alias-review', {e['classification'] for e in qa['pair_evidence']})
        self.assertIn('ambiguous-review', {e['classification'] for e in qa['pair_evidence']})
        same = duplicate_audit([p(1), replace(p(1), ordinal=2)], 'hash')
        self.assertEqual(same['pair_evidence'][0]['classification'], 'definite-same-record')

    def test_spatial_audit_dateline_poles_and_metro(self):
        for a, b in [(p(1, 179.999), p(2, -179.999)), (p(1, 0, 89.9999), p(2, 180, 89.9999))]:
            self.assertEqual(len(duplicate_audit([a, b], 'hash')['pair_evidence']), 1)
        evidence = duplicate_audit([p(1, name='Example'), p(2, .01, name='Example metropolitan area')], 'hash')['pair_evidence']
        self.assertIn('possible-city-versus-metro-name', evidence[0]['reasons'])

    def test_reviewed_seed_bound_to_source_and_semantics(self):
        place = p(1)
        doc = {'source_sha256': 'hash', 'seeds': [{'source_id': '1', 'status': 'approved', 'reviewer': 'synthetic test',
               'evidence': ['synthetic reference'], 'date': '2026-11-09', 'role': 'national', 'basis_sha256': basis(place)}]}
        self.assertEqual(reviewed_seeds(doc, [place], 'hash'), {'1': 'national'})
        for changed in (replace(place, lon=1), replace(place, country='BBB')):
            with self.assertRaises(ValueError):
                reviewed_seeds(doc, [changed], 'hash')
        with self.assertRaises(ValueError):
            reviewed_seeds(doc, [place], 'different hash')
        with self.assertRaises(ValueError):
            reviewed_seeds({**doc, 'seeds': doc['seeds']*2}, [place], 'hash')

    def test_incremental_scores_match_direct_formula(self):
        places = [p(i, i*3, pop=(i+1)*100000, country='AAA' if i%2 else 'BBB') for i in range(1, 12)]
        for policy in ('population', 'inverse-square', 'nearest'):
            for cross_border in (False, True):
                params = Parameters(min_score=0, cross_border=cross_border)
                chosen = select(places, policy, {'1': 'seed'}, params)
                for candidate in places:
                    record = chosen[candidate.key]
                    if record['round'] == 0:
                        continue
                    prior = [s for s in places if chosen[s.key]['round'] < record['round']]
                    self.assertAlmostEqual(record['score'], score(candidate, prior, policy, params), places=10)

    def test_comparison_count_is_separate_from_stop_rule(self):
        places = [p(1, pop=10), p(2, 1), p(3, 30), p(4, 40, country='BBB')]
        self.assertEqual(len(select(places, 'population', {'1': 'seed'}, comparison_count=2)), 2)
        self.assertEqual(len(select(places, 'population', {'1': 'seed'})), 4)
        with self.assertRaises(ValueError):
            select(places, 'population', {'1': 'seed'}, comparison_count=0)

    def test_empty_and_bad_configuration(self):
        self.assertEqual(select([], 'population'), {})
        for params in (Parameters(smoothing_km=0), Parameters(alpha=-1), Parameters(min_score=float('nan'))):
            with self.assertRaises(ValueError):
                select([], 'population', params=params)
        with self.assertRaises(ValueError):
            select([p(1, 181)], 'population')


if __name__ == '__main__':
    unittest.main()
