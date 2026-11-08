#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Contract rejection and geographic semantic fixtures, independent of network/GIS."""
from copy import deepcopy
import json
import unittest

from catalog import RELATIONS, validate
from sources import ATLAS


class CatalogTests(unittest.TestCase):
    def setUp(self):
        self.catalog = json.loads((ATLAS/'catalog/fixture.json').read_text())

    def rejects(self, mutate, text):
        mutate(self.catalog)
        with self.assertRaisesRegex(ValueError, text):
            validate(self.catalog)

    def test_fixture_is_explicitly_unreviewed(self):
        result = validate(self.catalog)
        self.assertEqual(result['tables']['entities'], 35)
        self.assertTrue(result['warnings'])
        self.assertTrue(all(a['review']['status'] == 'unreviewed' for a in self.catalog['assertions']))

    def test_digest_must_reference_an_existing_lock(self):
        self.rejects(lambda c: c['sources'][0].update(sha256='0'*64), 'differs from lock')

    def test_duplicate_identity_rejected(self):
        self.rejects(lambda c: c['entities'].append(c['entities'][0]), 'duplicate ID')

    def test_missing_evidence_rejected(self):
        self.rejects(lambda c: c['assertions'][0].update(evidence=[]), 'evidence')

    def test_dangling_reference_rejected(self):
        self.rejects(lambda c: c['assertions'][0].update(object='missing'), 'dangling')

    def test_source_scope_includes_layer_and_ordinal(self):
        row = deepcopy(self.catalog['records'][0])
        row['id'] += ':duplicate'
        self.rejects(lambda c: c['records'].append(row), 'duplicate source record')

    def test_review_acceptance_requires_named_decision(self):
        self.rejects(lambda c: c['assertions'][0].update(review={'status': 'accepted', 'reviewer': None, 'decision': None}), 'review decision')

    def test_accepted_decision_has_a_date_separate_from_effective_time(self):
        a = self.catalog['assertions'][0]
        a['review'] = {'status': 'accepted', 'reviewer': 'fixture test only', 'decision': 'synthetic validation', 'decided_on': '2026-11-08'}
        validate(self.catalog)
        self.assertIsNone(a['time']['from'])

    def test_unknown_temporal_basis_does_not_invent_effective_date(self):
        self.rejects(lambda c: c['assertions'][0]['time'].update({'from': '2026-11-08'}), 'unknown time')

    def test_reversed_validity_rejected(self):
        self.rejects(lambda c: c['assertions'][0].update(time={'basis': 'effective-interval', 'from': '2026-11-08', 'to': '2025-01-01'}), 'reversed')

    def test_time_qualified_successor_is_not_equality(self):
        assertion = self.catalog['assertions'][0]
        assertion.update(relation='historical-successor', time={'basis': 'effective-interval', 'from': '2020-01-01', 'to': None})
        validate(self.catalog)
        self.assertEqual(assertion['relation'], 'historical-successor')

    def test_relation_types_remain_distinct(self):
        for relation in RELATIONS:
            with self.subTest(relation=relation):
                if relation == 'capital-role':
                    self.assertTrue(any(a['relation'] == relation for a in self.catalog['assertions']))
                else:
                    self.catalog['assertions'][0]['relation'] = relation
                validate(self.catalog)

    def test_candidate_cannot_be_laundered_to_accepted_equality(self):
        self.rejects(lambda c: c['assertions'][0].update(relation='unresolved-candidate', review={'status': 'accepted', 'reviewer': 'fixture', 'decision': 'test', 'decided_on': '2026-11-08'}), 'candidate')

    def test_duplicate_qid_never_merges_entities(self):
        rows = [r for r in self.catalog['records'] if r['fields'].get('WIKIDATAID') == 'Q18808']
        self.assertEqual(len(rows), 2)
        links = [a for a in self.catalog['assertions'] if a['subject'] in {r['id'] for r in rows} and a['relation'] == 'same-entity']
        self.assertEqual(len({a['object'] for a in links}), 2)
        candidates = [a for a in self.catalog['assertions'] if a['relation'] == 'unresolved-candidate']
        self.assertTrue(any(a['subject'] in {r['id'] for r in rows} for a in candidates))

    def test_disputed_classification_retains_source_fields(self):
        records = {r['fields'].get('ADM0_A3'): r for r in self.catalog['records'] if 'TYPE' in r['fields']}
        self.assertEqual(records['PSX']['fields']['TYPE'], 'Indeterminate')
        self.assertEqual(records['PSX']['fields']['SOV_A3'], 'IS1')
        self.assertFalse(any(a['relation'] == 'part-of' for a in self.catalog['assertions']))

    def test_microstate_existence_independent_of_geometry(self):
        self.assertTrue({'gcat:000001', 'gcat:000002', 'gcat:000003', 'gcat:000004', 'gcat:000005'} <= {e['id'] for e in self.catalog['entities']})
        self.assertFalse(any('geometry' in e for e in self.catalog['entities']))

    def test_alias_does_not_assert_same_country_settlement_identity(self):
        aliases = [a for a in self.catalog['assertions'] if a['relation'] == 'source-code-alias']
        self.assertEqual(len(aliases), 1)
        self.assertIn('SSD', aliases[0]['rationale'])

    def test_unicode_and_lossy_fallback_both_survive(self):
        names = {n['id']: n for n in self.catalog['names']}
        self.assertEqual(names['gcat:000110:NAME']['value'], 'Łódź')
        fallback = names['gcat:000110:mac-roman-candidate']
        self.assertEqual(fallback['value'], 'Lodz')
        self.assertTrue(fallback['loss'])
        self.assertEqual(fallback['review']['status'], 'unreviewed')

    def test_label_point_is_not_assumed_inside_polygon(self):
        point = next(p for p in self.catalog['points'] if p['subject'] == 'gcat:000017')
        self.assertEqual(point['role'], 'source-label')
        self.assertEqual(point['containment'], 'unverified')

    def test_name_cannot_silently_repair_source(self):
        self.rejects(lambda c: c['names'][0].update(value='invented'), 'differs from source')

    def test_capital_flags_must_match_evidence(self):
        capital = next(a for a in self.catalog['assertions'] if a['relation'] == 'capital-role')
        self.rejects(lambda c: capital.update(qualifiers={'ADM0CAP': '0', 'CAPALT': '0'}), 'capital source flags')

    def test_spratly_wake_are_separate_unresolved_records(self):
        records = [r for r in self.catalog['records'] if r['fields'].get('WIKIDATAID') == 'Q43296']
        self.assertEqual(len(records), 2)
        self.assertEqual({r['fields']['GU_A3'] for r in records}, {'PGA', 'WQI'})

    def test_nan_point_rejected(self):
        self.rejects(lambda c: c['points'][0].update(coordinate=[float('nan'), 0]), 'bounds')

    def test_fallback_without_lineage_rejected(self):
        row = next(n for n in self.catalog['names'] if n['role'] == 'display-fallback-candidate')
        self.rejects(lambda c: row.update(derived_from='missing'), 'lineage')


if __name__ == '__main__':
    unittest.main()
