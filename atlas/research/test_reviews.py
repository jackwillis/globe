#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Review decisions bind to exact semantics/source context and cannot repair external data."""
from copy import deepcopy
import json
import unittest

from reviews import apply, fingerprint, queue
from sources import ATLAS


class ReviewTests(unittest.TestCase):
    def setUp(self):
        self.catalog = json.loads((ATLAS/'catalog/fixture.json').read_text())
        self.row = self.catalog['assertions'][0]

    def decision(self, table='assertions', row=None, status='accepted'):
        row = row or self.row
        return {'schema': 1, 'decisions': [{'table': table, 'target_id': row['id'],
            'basis_sha256': fingerprint(self.catalog, table, row), 'status': status,
            'reviewer': 'synthetic test only', 'decision': 'fixture decision, no editorial approval', 'decided_on': '2026-11-08'}]}

    def test_review_changes_only_review_fields_and_is_not_in_place(self):
        updated = apply(self.catalog, self.decision())
        self.assertEqual(self.row['review']['status'], 'unreviewed')
        changed = next(r for r in updated['assertions'] if r['id'] == self.row['id'])
        self.assertEqual(changed['review']['status'], 'accepted')
        self.assertEqual(fingerprint(updated, 'assertions', changed), fingerprint(self.catalog, 'assertions', self.row))
        changed['review'] = self.row['review']
        self.assertEqual(updated, self.catalog)

    def test_relation_change_invalidates_decision(self):
        overlay = self.decision()
        self.row['relation'] = 'historical-successor'
        with self.assertRaisesRegex(ValueError, 'stale review'):
            apply(self.catalog, overlay)

    def test_changed_evidence_field_invalidates_decision(self):
        overlay = self.decision()
        record = next(r for r in self.catalog['records'] if r['id'] == self.row['evidence'][0])
        record['fields']['NAME'] += ' changed'
        with self.assertRaisesRegex(ValueError, 'stale review'):
            apply(self.catalog, overlay)

    def test_source_revision_change_invalidates_decision(self):
        overlay = self.decision()
        for source in self.catalog['sources']:
            source['sha256'] = '0'*64
        with self.assertRaisesRegex(ValueError, 'stale review'):
            apply(self.catalog, overlay)

    def test_point_change_invalidates_decision(self):
        point = self.catalog['points'][0]
        overlay = self.decision('points', point)
        point['coordinate'][0] += 0.01
        with self.assertRaisesRegex(ValueError, 'stale review'):
            apply(self.catalog, overlay)

    def test_candidate_cannot_be_accepted_as_equality(self):
        row = next(r for r in self.catalog['assertions'] if r['relation'] == 'unresolved-candidate')
        with self.assertRaisesRegex(ValueError, 'candidate'):
            apply(self.catalog, self.decision(row=row))
        rejected = apply(self.catalog, self.decision(row=row, status='rejected'))
        self.assertEqual(next(r for r in rejected['assertions'] if r['id'] == row['id'])['relation'], 'unresolved-candidate')

    def test_duplicate_or_missing_targets_rejected(self):
        overlay = self.decision()
        overlay['decisions'] *= 2
        with self.assertRaisesRegex(ValueError, 'duplicate review'):
            apply(self.catalog, overlay)
        overlay = self.decision()
        overlay['decisions'][0]['target_id'] = 'missing'
        with self.assertRaisesRegex(ValueError, 'target missing'):
            apply(self.catalog, overlay)

    def test_empty_review_is_not_blanket_geographic_acceptance(self):
        overlay = json.loads((ATLAS/'catalog/review-decisions.json').read_text())
        self.assertTrue(overlay['checkpoint_acceptance'])
        self.assertEqual(apply(self.catalog, overlay), self.catalog)

    def test_decision_requires_name_rationale_and_date(self):
        for field in ('reviewer', 'decision', 'decided_on'):
            overlay = self.decision()
            overlay['decisions'][0][field] = ''
            with self.assertRaises(ValueError):
                apply(self.catalog, overlay)

    def test_review_cannot_smuggle_a_relation_or_source_edit(self):
        overlay = self.decision()
        overlay['decisions'][0]['relation'] = 'same-entity'
        with self.assertRaisesRegex(ValueError, 'decision fields'):
            apply(self.catalog, overlay)

    def test_fallback_review_binds_to_original_name_metadata(self):
        fallback = next(n for n in self.catalog['names'] if n['role'] == 'display-fallback-candidate')
        overlay = self.decision('names', fallback)
        original = next(n for n in self.catalog['names'] if n['id'] == fallback['derived_from'])
        original['language'] = 'und'
        with self.assertRaisesRegex(ValueError, 'stale review'):
            apply(self.catalog, overlay)

    def test_queue_covers_assertions_points_and_lossy_fallbacks(self):
        rows = queue(self.catalog)
        self.assertTrue(any(r['table'] == 'points' for r in rows))
        names = [r for r in rows if r['table'] == 'names']
        self.assertTrue(names)
        self.assertTrue(all(r['claim']['role'] == 'display-fallback-candidate' for r in names))
        self.assertEqual(len({(r['table'], r['target_id']) for r in rows}), len(rows))


if __name__ == '__main__':
    unittest.main()
