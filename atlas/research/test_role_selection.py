#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Additive inclusion preserves uncertain role data rather than repairing source flags."""
import unittest

from role_selection import include_role_candidates
from test_selection import place


class RoleSelectionTests(unittest.TestCase):
    def test_class_only_capital_is_included_without_mutating_flags(self):
        juba = place(1, 31, 4, code='SSD')
        juba[0]['FEATURECLA'] = 'Admin-0 capital'
        chosen = include_role_candidates([juba], [])
        self.assertEqual(len(chosen), 1)
        self.assertEqual(chosen[0][0]['ADM0CAP'], '0')
        self.assertIn('source-admin0-role-review', chosen[0][2])

    def test_regional_role_is_not_promoted_to_national_capital(self):
        row = place(1, 0, 0)
        row[0]['FEATURECLA'] = 'Admin-0 region capital'
        chosen = include_role_candidates([row], [])
        self.assertEqual(chosen[0][0]['FEATURECLA'], 'Admin-0 region capital')
        self.assertEqual(chosen[0][0]['ADM0CAP'], '0')

    def test_existing_candidates_and_reasons_survive(self):
        row = place(1, 0, 0)
        original_reasons = {'country-representative'}
        original = [(row[0], row[1], original_reasons)]
        chosen = include_role_candidates([row], original)
        self.assertEqual(chosen[0][2], original_reasons)
        self.assertEqual(original_reasons, {'country-representative'})

    def test_admin1_class_alone_does_not_expand_this_roster(self):
        row = place(1, 0, 0)
        row[0]['FEATURECLA'] = 'Admin-1 capital'
        self.assertEqual(include_role_candidates([row], []), [])


if __name__ == '__main__':
    unittest.main()
