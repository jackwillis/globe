#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Spherical grid boundaries and deterministic role-preserving selection."""
import math
import unittest

from audit import select_places
from selection import equal_budget, grid_cell, nearest_gaps, select_equal_area


def place(key, lon, lat, zoom=9, capital=0, code='AAA'):
    return ({'NE_ID': str(key), 'NAME': str(key), 'MIN_ZOOM': str(zoom), 'POP_MAX': '100',
             'ADM0CAP': str(capital), 'CAPALT': '0', 'ADM0_A3': code, 'FEATURECLA': 'Populated place'}, [[(lon, lat)]])


class SelectionTests(unittest.TestCase):
    recipe = {'place_min_zoom': 3, 'regional_min_zoom': 4, 'coverage_cell_degrees': 30}

    def test_poles_and_dateline_have_bounded_cells(self):
        for area in (False, True):
            self.assertEqual(grid_cell(-180, 90, 30, area), grid_cell(180, 90, 30, area))
            self.assertEqual(grid_cell(0, -90, 30, area)[1], 0)
            self.assertEqual(grid_cell(0, 90, 30, area)[1], 5)

    def test_equal_area_latitude_boundaries(self):
        # Analytic strip area is proportional to sin(north)-sin(south).
        edges = [math.asin(-1 + i/3) for i in range(7)]
        areas = [math.sin(b)-math.sin(a) for a, b in zip(edges, edges[1:])]
        self.assertLess(max(areas)-min(areas), 1e-14)
        self.assertNotEqual(grid_cell(0, 45, 30), grid_cell(0, 45, 30, True))

    def test_invalid_coordinate_and_grid_rejected(self):
        for args in [(181, 0, 30), (0, 91, 30), (0, 0, 7), (0, 0, 0), (float('nan'), 0, 30)]:
            with self.assertRaises(ValueError):
                grid_cell(*args)

    def test_capitals_and_country_representatives_survive(self):
        rows = [place(1, 0, 0, zoom=1), place(2, 1, 1, capital=1), place(3, 2, 2, code='BBB')]
        selected = select_equal_area(rows, self.recipe)
        self.assertEqual({r['NE_ID'] for r, p, reasons in selected}, {'1', '2', '3'})

    def test_no_population_or_qid_merge(self):
        rows = [place(1, 0, 0, capital=1), place(2, 1, 1, capital=1)]
        self.assertEqual(len(select_equal_area(rows, self.recipe)), 2)

    def test_input_order_does_not_change_chosen_ids_or_reasons(self):
        rows = [place(3, 70, 50), place(2, 1, 1), place(1, 0, 0)]
        normalize = lambda selected: {r['NE_ID']: reasons for r, p, reasons in selected}
        self.assertEqual(normalize(select_equal_area(rows, self.recipe)), normalize(select_equal_area(rows[::-1], self.recipe)))

    def test_only_grid_reasons_change(self):
        rows = [place(1, 0, 0, zoom=1), place(2, 1, 1, capital=1), place(3, 70, 50)]
        old = {r['NE_ID']: reasons - {'occupied-grid-representative'} for r, p, reasons in select_places(rows, self.recipe)}
        new = {r['NE_ID']: reasons - {'occupied-equal-area-representative'} for r, p, reasons in select_equal_area(rows, self.recipe)}
        self.assertEqual({k: v for k, v in old.items() if v}, {k: v for k, v in new.items() if v})

    def test_equal_count_supplement_prefers_distant_settlement(self):
        rows = [place(1, 0, 0), place(2, 1, 1), place(3, 90, 0)]
        seed = [(rows[0][0], rows[0][1], {'seed'})]
        chosen = equal_budget(rows, seed, 2)
        self.assertEqual({r['NE_ID'] for r, p, reasons in chosen}, {'1', '3'})
        self.assertIn('seed', chosen[0][2])
        reverse = equal_budget(rows[::-1], seed, 2)
        self.assertEqual({r['NE_ID']: reasons for r, p, reasons in chosen},
                         {r['NE_ID']: reasons for r, p, reasons in reverse})
        with self.assertRaises(ValueError):
            equal_budget(rows, seed, 0)

    def test_distance_handles_dateline(self):
        rows = [place(1, 179, 0), place(2, -179, 0)]
        selected = [(rows[0][0], rows[0][1], {'test'})]
        gap = nearest_gaps(rows, selected)['largest_gaps'][0]
        self.assertAlmostEqual(gap['km'], 222.390, places=3)


if __name__ == '__main__':
    unittest.main()
