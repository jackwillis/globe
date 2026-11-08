#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Small independent fixtures for source concordance and name fallback boundaries."""
import unittest

from concordance import active_claims, compare_m49, distance_km, earth_coordinates, m49_rows
from name_profiles import display_candidate


def claim(value,rank='normal',snaktype='value'):
    return {'rank':rank,'mainsnak':{'snaktype':snaktype,'datavalue':{'value':value}}}


def table(rows,table_id='downloadTableEN'):
    header=['Country or Area','M49 Code','ISO-alpha2 Code','ISO-alpha3 Code','Region Name','Sub-region Name']
    tr=lambda cells:'<tr>'+''.join('<td>'+c+'</td>' for c in cells)+'</tr>'
    return ('<table id="'+table_id+'">'+tr(header)+''.join(tr(row) for row in rows)+'</table>').encode()


class ConcordanceTests(unittest.TestCase):
    def test_m49_uses_only_named_english_table(self):
        english=table([['Test &amp; Islands','001','AA','AAA','Region','Subregion']])
        other=table([['Other','002','BB','BBB','Other','Other']],'downloadTableFR')
        result=m49_rows(other+english)
        self.assertEqual(len(result),1)
        self.assertEqual(result[0]['Country or Area'],'Test & Islands')
        with self.assertRaisesRegex(ValueError,'ambiguous'):
            m49_rows(english+english)

    def test_m49_rejects_duplicate_and_malformed_codes(self):
        with self.assertRaisesRegex(ValueError,'duplicate'):
            m49_rows(table([['A','001','AA','AAA','R','S'],['B','001','BB','BBB','R','S']]))
        with self.assertRaisesRegex(ValueError,'malformed'):
            m49_rows(table([['A','-99','AA','AAA','R','S']]))

    def test_sentinels_never_become_identity_joins(self):
        reference=m49_rows(table([['A','001','AA','AAA','R','S']]))
        political=[({'ADM0_A3':'XXX','NAME':'A','ADMIN':'A','ISO_A3':'-99','ISO_A3_EH':'-99','UN_A3':'-099'},[])]
        result=compare_m49(reference,political)
        self.assertEqual(result['iso_joined_ne_records'],0)
        self.assertEqual(len(result['ne_unjoined_records']),1)

    def test_missing_un_code_is_distinct_from_conflicting_code(self):
        reference=m49_rows(table([['A','001','AA','AAA','R','S']]))
        rows=[({'ADM0_A3':'X','NAME':'A','ADMIN':'A','ISO_A3_EH':'AAA','UN_A3':'-99'},[]),
              ({'ADM0_A3':'Y','NAME':'B','ADMIN':'B','ISO_A3_EH':'AAA','UN_A3':'002'},[])]
        result=compare_m49(reference,rows)
        self.assertEqual(len(result['ne_un_code_missing']),1)
        self.assertEqual(len(result['ne_un_code_conflicts']),1)
        self.assertEqual(result['ne_multiple_records_for_iso'],{'AAA':['X','Y']})

    def test_preferred_rank_and_earth_globe(self):
        earth={'longitude':179,'latitude':0,'globe':'http://www.wikidata.org/entity/Q2','precision':0.1}
        moon={**earth,'globe':'http://www.wikidata.org/entity/Q405'}
        entity={'claims':{'P625':[claim(earth),claim(moon,'preferred'),claim(earth,'deprecated')]}}
        self.assertEqual(len(active_claims(entity,'P625')),1)
        self.assertEqual(earth_coordinates(entity),[])
        entity['claims']['P625']=[claim(earth),claim(moon,'deprecated'),claim(earth,'normal','novalue')]
        self.assertEqual(earth_coordinates(entity),[(179,0,0.1)])

    def test_geodesic_distance_wraps_antimeridian(self):
        self.assertLess(abs(distance_km((179,0),(-179,0))-222.39016),0.001)
        self.assertEqual(distance_km((180,0),(-180,0)),0)

    def test_mac_roman_retains_representable_diacritics(self):
        row={'NAME':'Reykjavík','NAME_EN':'Reykjavík','NAMEASCII':'Reykjavik'}
        result=display_candidate(row,'mac_roman')
        self.assertFalse(result['fallback'])
        self.assertEqual(bytes.fromhex(result['encoded_hex']).decode('mac_roman'),'Reykjavík')
        self.assertEqual(result['source_name'],'Reykjavík')

    def test_lossy_fallback_is_explicit_and_never_changes_original(self):
        row={'NAME':'Łódź','NAME_EN':'Łódź','NAMEASCII':'Lodz'}
        result=display_candidate(row,'mac_roman')
        self.assertEqual(result['display_candidate'],'Lodz')
        self.assertEqual(result['source_name'],'Łódź')
        self.assertTrue(result['loss'])
        self.assertTrue(result['review_required'])
        self.assertEqual(result['fallback_source_field'],'NAMEASCII')

    def test_no_invented_transliteration_for_unresolved_name(self):
        result=display_candidate({'NAME':'東京','NAMEASCII':''},'mac_roman')
        self.assertIsNone(result['display_candidate'])
        self.assertTrue(result['identity_unchanged'])
        self.assertEqual(result['source_name'],'東京')

if __name__=='__main__':
    unittest.main()
