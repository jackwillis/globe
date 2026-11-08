#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Small semantic/parser fixtures; no downloads and no CMake integration."""
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest

from audit import identity, names, select_places, valid_qid
from baseline import inventory
from sources import shp_records, verified_path


def point_shape(x, y):
    header = bytearray(100)
    struct.pack_into('>I', header, 0, 9994)
    struct.pack_into('>I', header, 24, 64)
    struct.pack_into('<II', header, 28, 1000, 1)
    return bytes(header) + struct.pack('>II',1,10) + struct.pack('<Idd',1,x,y)


class AuditTests(unittest.TestCase):
    def test_antimeridian_and_invalid_coordinates(self):
        self.assertEqual(shp_records(point_shape(180, -17))[1], [[[(180,-17)]]])
        for x,y in [(181,0),(0,91),(float('nan'),0),(0,float('inf'))]:
            with self.assertRaisesRegex(ValueError,'coordinate'):
                shp_records(point_shape(x,y))

    def test_truncation_and_extra_bytes(self):
        raw=point_shape(12,13)
        for cut in range(len(raw)):
            with self.assertRaises((ValueError,struct.error)):
                shp_records(raw[:cut])
        with self.assertRaises(ValueError):
            shp_records(raw+b'\0')

    def test_null_record_preserves_dbf_alignment(self):
        raw=bytearray(point_shape(1,2)[:100])
        struct.pack_into('>I',raw,24,56)
        raw+=struct.pack('>III',1,2,0)
        self.assertEqual(shp_records(raw)[1],[[]])

    def test_source_checksum_mismatch(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory)/'source.zip';p.write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError,'checksum'):
                verified_path({'path':p.name,'id':'fixture','bytes':7,'sha256':hashlib.sha256(b'correct').hexdigest()},directory)

    def test_capital_survives_population_and_label_threshold(self):
        def row(key,capital,country):
            return ({'NE_ID':key,'ADM0CAP':str(capital),'CAPALT':'0','MIN_ZOOM':'9','POP_MAX':'10','ADM0_A3':country,'FEATURECLA':'Populated place'}, [[(10,20)]])
        selected=select_places([row('1',0,'AAA'),row('2',1,'AAA'),row('3',0,'BBB')],
            {'place_min_zoom':1,'regional_min_zoom':1,'coverage_cell_degrees':30})
        result={r['NE_ID']:reasons for r,p,reasons in selected}
        self.assertIn('capital-source-flag',result['2'])
        self.assertIn('country-representative',result['3'])
        self.assertEqual(result,{r['NE_ID']:reasons for r,p,reasons in select_places(
            [row('3',0,'BBB'),row('2',1,'AAA'),row('1',0,'AAA')],
            {'place_min_zoom':1,'regional_min_zoom':1,'coverage_cell_degrees':30})})

    def test_identity_is_not_name_or_geometry_index(self):
        self.assertEqual(identity({'ADM0_A3':'SMR','NAME':'San Marino'},'admin0'),'ne:admin0:SMR')
        self.assertTrue(valid_qid('Q238'))
        self.assertFalse(valid_qid('-99'))
        self.assertFalse(valid_qid('Q0'))
        with self.assertRaises(ValueError):
            identity({'NE_ID':'-99'},'place')

    def test_names_preserve_unicode_and_language_provenance(self):
        result=names({'NAME':'Reykjavík','NAME_EN':'Reykjavik','NAME_ZHT':'雷克雅維克'})
        self.assertEqual(result[0]['value'],'Reykjavík')
        self.assertEqual(result[0]['language'],'und')
        self.assertIn({'value':'雷克雅維克','language':'zh-Hant','role':'localized-source','source_field':'NAME_ZHT'},result)
        self.assertEqual('Reykjavík'.encode('mac_roman').decode('mac_roman'),'Reykjavík')
        with self.assertRaises(UnicodeEncodeError):
            'Łódź'.encode('mac_roman')

    def test_production_pack_inventory_matches_recorded_baseline(self):
        expected=json.loads((Path(__file__).resolve().parents[1]/'reports/baseline.json').read_text())
        self.assertEqual(inventory(),expected)

if __name__=='__main__':
    unittest.main()
