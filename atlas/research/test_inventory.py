#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Stable allocation, namespace joins, and contradictory capital evidence fixtures."""
from copy import deepcopy
import json
import unittest

from catalog import validate
from inventory import LAYERS, allocations, build, code_targets, summarize
from sources import ATLAS, locked_sources


class InventoryTests(unittest.TestCase):
    def setUp(self):
        self.fixture = json.loads((ATLAS/'catalog/fixture.json').read_text())
        self.locks = {s['id']: s for s in locked_sources()}
        links = {a['subject']: a['object'] for a in self.fixture['assertions'] if a['relation'] == 'same-entity'}
        points = {p['subject']: p['coordinate'] for p in self.fixture['points']}
        self.selected = [(r['source'], r['ordinal'], deepcopy(r['fields']), [[tuple(points[links[r['id']]])]]) for r in self.fixture['records']]

    def registry(self):
        return allocations(self.locks, self.selected, self.fixture, allow_new=True)

    def test_new_allocation_requires_explicit_mode(self):
        with self.assertRaisesRegex(ValueError, 'explicit --allocate'):
            allocations(self.locks, self.selected, self.fixture)

    def test_existing_fixture_ids_survive_and_input_order_is_irrelevant(self):
        registry = self.registry()
        expected = {e['id'] for e in self.fixture['entities']}
        self.assertEqual({a['entity_id'] for a in registry['allocations']}, expected)
        replay = allocations(self.locks, self.selected[::-1], self.fixture, existing=registry)
        self.assertEqual(registry, replay)

    def test_same_source_identifier_in_different_layers_does_not_merge(self):
        selected = [(LAYERS[0], 1, {'NE_ID': '1'}, []), (LAYERS[1], 1, {'NE_ID': '1'}, [])]
        registry = allocations(self.locks, selected, {'records': [], 'assertions': []}, allow_new=True)
        self.assertEqual(len({r['entity_id'] for r in registry['allocations']}), 2)

    def test_changed_source_revision_requires_reconciliation(self):
        registry = self.registry()
        registry['sources'][LAYERS[0]] = '0'*64
        with self.assertRaisesRegex(ValueError, 'source revision'):
            allocations(self.locks, self.selected, self.fixture, existing=registry)

    def test_changed_ordinal_is_not_silently_rebound(self):
        registry = self.registry()
        registry['allocations'][0]['ordinal'] += 1
        with self.assertRaisesRegex(ValueError, 'allocation changed'):
            allocations(self.locks, self.selected, self.fixture, existing=registry)

    def test_disappearing_source_records_are_not_dropped(self):
        with self.assertRaisesRegex(ValueError, 'never drop IDs'):
            allocations(self.locks, self.selected[:-1], self.fixture, existing=self.registry())

    def test_next_id_cannot_recycle_an_existing_allocation(self):
        registry = self.registry()
        # Fixture IDs live below the next-allocation range, so first extend the roster.
        selected = self.selected + [(LAYERS[0], 999, {'NE_ID': '9999999999'}, [])]
        registry = allocations(self.locks, selected, self.fixture, registry, allow_new=True)
        registry['next_numeric_id'] = 1000
        with self.assertRaisesRegex(ValueError, 'recycle'):
            allocations(self.locks, selected, self.fixture, registry)

    def test_duplicate_ids_rejected(self):
        registry = self.registry()
        registry['allocations'][1]['entity_id'] = registry['allocations'][0]['entity_id']
        with self.assertRaisesRegex(ValueError, 'duplicate allocation'):
            allocations(self.locks, self.selected, self.fixture, existing=registry)

    def test_alias_and_map_unit_parent_are_distinct_from_exact_country(self):
        countries = [{'id': 'country:SDS', 'fields': {'ADM0_A3': 'SDS', 'ISO_A3': 'SSD'}}]
        units = [{'id': 'unit:TKL', 'fields': {'GU_A3': 'TKL', 'ADM0_A3': 'NZL'}}]
        self.assertEqual(code_targets('SDS', countries, units)[1], 'exact-country-source-code')
        self.assertEqual(code_targets('SSD', countries, units)[1], 'iso-code-alias-candidate')
        targets, resolution = code_targets('TKL', countries, units)
        self.assertEqual(resolution, 'map-unit-source-code-candidate')
        self.assertEqual([r['id'] for r in targets], ['unit:TKL'])
        self.assertEqual(code_targets('-99', countries, units)[0], [])
        sentinels = [{'id': 'invalid', 'fields': {'ADM0_A3': '-99', 'ISO_A3': '-99'}}]
        self.assertEqual(code_targets('-99', sentinels, units)[0], [])

    def test_ambiguous_code_targets_are_all_retained(self):
        countries = [{'id': str(i), 'fields': {'ADM0_A3': str(i), 'ISO_A3': 'AAA'}} for i in (1, 2)]
        self.assertEqual(len(code_targets('AAA', countries, [])[0]), 2)

    def test_fixture_assertion_ids_are_preserved_in_expansion(self):
        catalog, capitals = build(self.locks, self.selected, self.fixture, self.registry())
        self.assertTrue({a['id'] for a in self.fixture['assertions']} <= {a['id'] for a in catalog['assertions']})
        validate(catalog)

    def test_juba_class_flag_disagreement_is_preserved_not_repaired(self):
        catalog, capitals = build(self.locks, self.selected, self.fixture, self.registry())
        juba = next(r for r in capitals if r['name'] == 'Juba')
        self.assertEqual(juba['selection_basis'], 'class-only')
        self.assertEqual(juba['resolution'], 'iso-code-alias-candidate')
        self.assertFalse(juba['national_flag'])
        assertion = next(a for a in catalog['assertions'] if a['subject'] == juba['source_record'] and a['relation'] == 'source-capital-classification')
        self.assertEqual(assertion['qualifiers']['FEATURECLA'], 'Admin-0 capital')
        assertion['qualifiers']['FEATURECLA'] = 'invented'
        with self.assertRaisesRegex(ValueError, 'capital source classification'):
            validate(catalog)

    def test_full_inventory_has_source_capital_and_class_disagreements(self):
        summary = json.loads((ATLAS/'reports/catalog-inventory.json').read_text())
        self.assertEqual(summary['source_records'][LAYERS[0]], 258)
        self.assertEqual(summary['source_records'][LAYERS[1]], 298)
        self.assertEqual(sum(r['national_flag'] or r['alternate_flag'] for r in summary['capital_roster']), 214)
        rows = {r['name']: r for r in summary['capital_roster']}
        for name in ('Juba', 'Yangon', 'Johannesburg'):
            self.assertEqual(rows[name]['selection_basis'], 'class-only')
        self.assertTrue(all(r['target_source_records'] for r in summary['capital_roster']))


if __name__ == '__main__':
    unittest.main()
