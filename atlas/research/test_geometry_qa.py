#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Synthetic topology oracle cases; optional pinned Shapely environment only."""
import unittest
from shapely import Point
from geometry_qa import expected_geometry_codes, polygon_from_parts, quantize


class GeometryTests(unittest.TestCase):
    def test_hole_before_shell_is_associated_by_containment(self):
        shell=[(0,0),(0,4),(4,4),(4,0),(0,0)]
        hole=[(1,1),(3,1),(3,3),(1,3),(1,1)]
        geometry,issues=polygon_from_parts([hole,shell])
        self.assertEqual(issues,[])
        self.assertTrue(geometry.covers(Point(0.5,0.5)))
        self.assertFalse(geometry.covers(Point(2,2)))

    def test_split_antimeridian_islands_do_not_fill_middle_of_globe(self):
        east=[(179,-1),(179,1),(180,1),(180,-1),(179,-1)]
        west=[(-180,-1),(-180,1),(-179,1),(-179,-1),(-180,-1)]
        geometry,issues=polygon_from_parts([east,west])
        self.assertTrue(geometry.is_valid)
        self.assertFalse(geometry.covers(Point(0,0)))
        self.assertTrue(geometry.covers(Point(179.5,0)))
        self.assertTrue(geometry.covers(Point(-179.5,0)))

    def test_collapsed_microstate_ring_is_reported(self):
        tiny=[(0,0),(0,0.001),(0.001,0.001),(0.001,0),(0,0)]
        geometry,issues=polygon_from_parts(quantize([tiny],0.01))
        self.assertIsNone(geometry)
        self.assertEqual(issues,[{'part':0,'kind':'collapsed-ring'}])

    def test_iso_alias_is_distinct_from_cartographic_parent(self):
        countries=[({'ADM0_A3':'SDS','ISO_A3':'SSD','ISO_A3_EH':'SSD'},[]),
                   ({'ADM0_A3':'NZL','ISO_A3':'NZL','ISO_A3_EH':'NZL'},[])]
        units=[({'ADM0_A3':'NZL','GU_A3':'TKL','ISO_A3':'TKL','ISO_A3_EH':'TKL'},[])]
        self.assertEqual(expected_geometry_codes('SSD',countries,units),({'SDS'},'iso-alias-to-admin0'))
        self.assertEqual(expected_geometry_codes('TKL',countries,units),({'NZL'},'cartographic-parent-via-map-unit'))
        self.assertEqual(expected_geometry_codes('XXX',countries,units),(set(),'unresolved-source-code'))

if __name__=='__main__':
    unittest.main()
