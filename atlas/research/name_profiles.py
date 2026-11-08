#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Data-only display-name candidates; no runtime encoding, font or placement decision."""
import argparse
from collections import defaultdict
import gzip
import hashlib
import json
from pathlib import Path
import unicodedata

from audit import json_bytes, report, select_places
from sources import ATLAS, locked_sources, read_source


def display_candidate(row,codec):
    original=row['NAME']
    preferred=row.get('NAME_EN') or original
    preferred=unicodedata.normalize('NFC',preferred)
    result={'source_name':original,'preferred_candidate':preferred,
            'preferred_language':'en' if row.get('NAME_EN') else 'und',
            'preferred_source_field':'NAME_EN' if row.get('NAME_EN') else 'NAME',
            'codec_profile':codec,'font_coverage':'unverified; installed-font probe required',
            'identity_unchanged':True}
    try:
        encoded=preferred.encode(codec,errors='strict')
        if any(ord(c)<32 or ord(c)==127 for c in preferred):
            raise ValueError('control character in display name')
        result.update(display_candidate=preferred,encoded_hex=encoded.hex(),encoded_bytes=len(encoded),
                      fallback=False,loss=False,review_required=False)
    except UnicodeEncodeError:
        fallback=row.get('NAMEASCII') or ''
        # This is source-supplied candidate data, not a universal language transliterator.
        safe=bool(fallback) and all(32<=ord(c)<=126 for c in fallback)
        if safe:
            encoded=fallback.encode(codec,errors='strict')
            result.update(display_candidate=fallback,encoded_hex=encoded.hex(),encoded_bytes=len(encoded),
                          fallback=True,loss=True,review_required=True,fallback_source_field='NAMEASCII')
        else:
            result.update(display_candidate=None,encoded_hex=None,encoded_bytes=0,
                          fallback=True,loss=True,review_required=True,fallback_source_field=None)
    return result


def analyze(cache,output):
    output.mkdir(parents=True,exist_ok=True)
    source=next(s for s in locked_sources() if s['id']=='ne_10m_populated_places')
    _,places=read_source(source,cache)
    recipes=json.loads((ATLAS/'editions/recipes.json').read_text())
    result={}
    for edition in ('compact','standard','expanded'):
        chosen=select_places(places,recipes[edition])
        profiles={}
        for codec in ('utf-8','mac_roman','iso8859-1','ascii'):
            records=[{'id':'ne:place:'+row['NE_ID'],**display_candidate(row,codec)} for row,p,reasons in chosen]
            records.sort(key=lambda row:row['id'])
            groups=defaultdict(list)
            for record in records:
                if record['display_candidate'] is not None:
                    groups[record['display_candidate']].append(record['id'])
            raw=b''.join(json_bytes(r) for r in records)
            compressed=gzip.compress(raw,compresslevel=9,mtime=0)
            filename=edition+'-'+codec+'.names.jsonl'
            (output/filename).write_bytes(raw)
            profiles[codec]={'records':len(records),'fallback_candidates':sum(r['fallback'] for r in records),
                'unresolved_candidates':sum(r['display_candidate'] is None for r in records),
                'preferred_utf8_text_bytes':sum(len(r['preferred_candidate'].encode('utf-8')) for r in records),
                'profile_text_bytes':sum(r['encoded_bytes'] for r in records),
                'dual_text_payload_estimate_bytes':sum(len(r['preferred_candidate'].encode('utf-8'))+r['encoded_bytes'] for r in records),
                'metadata_scenario_bytes':20*len(records),
                'metadata_scenario_basis':'Two u32 offsets and two u32 byte lengths plus one u32 profile/loss field per selected name; illustrative schema, not an approved format.',
                'same_display_candidate_groups':{name:ids for name,ids in groups.items() if len(ids)>1},
                'jsonl_bytes':len(raw),'gzip_comparison_bytes':len(compressed),'sha256':hashlib.sha256(raw).hexdigest(),
                'review_cases':[r for r in records if r['review_required']][:None if codec=='mac_roman' else 30],
                'review_cases_complete':codec=='mac_roman' or sum(r['review_required'] for r in records)<=30,
                'full_records_file':filename}
        result[edition]=profiles
    return {'schema':1,'policy_status':'Research candidates only; fallback rows require editorial review, and codec success does not guarantee installed glyph coverage.',
        'preferred_policy':'NAME_EN where present, otherwise source NAME; exact original retained, NFC preferred candidate; English is a test profile, not universal naming policy.',
        'no_runtime_changes':True,'editions':result,
        'limits':['No font/glyph data bundled, measured or selected.','No text renderer, layout, collision or final runtime encoding implementation.',
                  'Duplicate display strings do not imply duplicate identities. Do not deduplicate by a fallback string.',
                  'No automatic NFKD/diacritic stripping is used in these candidates; source NAMEASCII is an explicitly lossy fallback proposal.',
                  'Byte counts cover chosen settlement strings only; country/physical names and font-dependent layout storage are additional.']}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache',type=Path)
    parser.add_argument('output',type=Path)
    parser.add_argument('report',type=Path)
    args=parser.parse_args()
    report(args.report,analyze(args.cache,args.output))
