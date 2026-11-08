#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Reference-only M49 comparison and revision-pinned, targeted Wikidata concordance diagnostics."""
import argparse
from collections import Counter, defaultdict
import hashlib
from html.parser import HTMLParser
import json
import math
from pathlib import Path
import re
import urllib.request

from audit import number, report, valid_qid
from sources import ATLAS, locked_sources, read_source, verified_path


def reference_sources():
    return json.loads((ATLAS/'references.lock.json').read_text())['sources']


def acquire_reference(source, cache):
    path=Path(cache)/source['path']
    if path.exists():
        return verified_path(source,cache)
    if not (source['url'].startswith('https://www.wikidata.org/wiki/Special:EntityData/') or
            source['url']=='https://unstats.un.org/unsd/methodology/m49/overview/'):
        raise ValueError('unexpected reference URL')
    path.parent.mkdir(parents=True,exist_ok=True)
    url=source.get('revision_url') or source['url']
    if not url.startswith(source['url']):
        raise ValueError('unexpected revision URL')
    req=urllib.request.Request(url,headers={'User-Agent':'globe-atlas-research/0.2 (build-time concordance audit)'})
    with urllib.request.urlopen(req,timeout=120) as response:
        raw=response.read(source['bytes']+1)
    if len(raw)!=source['bytes'] or hashlib.sha256(raw).hexdigest()!=source['sha256']:
        raise ValueError('reference changed; retain the pinned snapshot, do not silently update its lock')
    temporary=path.with_suffix('.partial')
    temporary.write_bytes(raw)
    temporary.replace(path)
    return path


class M49Table(HTMLParser):
    """Read the explicitly named English table, never other language or historical tables."""
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.depth=0
        self.rows=[]
        self.row=None
        self.cell=None
        self.matches=0

    def handle_starttag(self,tag,attrs):
        if tag=='table':
            if self.depth:
                self.depth+=1
            elif dict(attrs).get('id')=='downloadTableEN':
                self.depth=1
                self.matches+=1
        if not self.depth:
            return
        if tag=='tr':
            self.row=[]
        elif tag in ('td','th') and self.row is not None:
            self.cell=[]

    def handle_data(self,text):
        if self.depth and self.cell is not None:
            self.cell.append(text)

    def handle_endtag(self,tag):
        if not self.depth:
            return
        if tag in ('td','th') and self.cell is not None:
            self.row.append(' '.join(''.join(self.cell).split()))
            self.cell=None
        elif tag=='tr' and self.row is not None:
            self.rows.append(self.row)
            self.row=None
        elif tag=='table':
            self.depth-=1


def m49_rows(raw):
    parser=M49Table()
    parser.feed(raw.decode('utf-8-sig'))
    if parser.matches!=1 or not parser.rows:
        raise ValueError('missing/ambiguous English M49 table')
    header=parser.rows[0]
    expected={'Country or Area','M49 Code','ISO-alpha2 Code','ISO-alpha3 Code','Region Name','Sub-region Name'}
    if not expected.issubset(header) or len(header)!=len(set(header)):
        raise ValueError('M49 table schema changed')
    records=[]
    for row in parser.rows[1:]:
        if len(row)!=len(header):
            raise ValueError('M49 row width changed')
        record=dict(zip(header,row))
        if not re.fullmatch(r'\d{3}',record['M49 Code']) or not re.fullmatch(r'[A-Z]{3}',record['ISO-alpha3 Code']):
            raise ValueError('M49 code malformed')
        records.append(record)
    for field in ('M49 Code','ISO-alpha3 Code'):
        if len({r[field] for r in records})!=len(records):
            raise ValueError('duplicate M49 reference code')
    return records


def active_claims(entity,prop):
    """Keep best-rank, value-bearing statements; never choose an arbitrary first row."""
    claims=[c for c in entity.get('claims',{}).get(prop,[]) if c.get('rank')!='deprecated' and c.get('mainsnak',{}).get('snaktype')=='value']
    preferred=[c for c in claims if c.get('rank')=='preferred']
    return preferred or claims


def claim_values(entity,prop):
    return [c['mainsnak']['datavalue']['value'] for c in active_claims(entity,prop)]


def earth_coordinates(entity):
    out=[]
    for value in claim_values(entity,'P625'):
        if value.get('globe')!='http://www.wikidata.org/entity/Q2':
            continue
        x,y=value['longitude'],value['latitude']
        if not math.isfinite(x) or not math.isfinite(y) or abs(x)>180 or abs(y)>90:
            raise ValueError('invalid Wikidata Earth coordinate')
        out.append((x,y,value.get('precision')))
    return out


def distance_km(a,b):
    x,y=a[:2]
    u,v=b[:2]
    dy=math.radians(v-y)
    dx=math.radians((u-x+180)%360-180)
    h=math.sin(dy/2)**2+math.cos(math.radians(y))*math.cos(math.radians(v))*math.sin(dx/2)**2
    return 2*6371.0088*math.asin(min(1,math.sqrt(max(0,h))))


def entity_ids(entity,prop):
    return sorted({v['id'] for v in claim_values(entity,prop) if isinstance(v,dict) and valid_qid(v.get('id'))})


def compare_m49(rows,political,code_field="ADM0_A3"):
    by_iso={r['ISO-alpha3 Code']:r for r in rows}
    matched=[]
    unmatched=[]
    duplicate_iso=defaultdict(list)
    for row,parts in political:
        iso=row.get('ISO_A3_EH')
        if not re.fullmatch(r'[A-Z]{3}',iso or ''):
            iso=row.get('ISO_A3')
        record={'ne_adm0_a3':row['ADM0_A3'],'ne_feature_code':row[code_field],'ne_name':row.get('NAME') or row['ADMIN'],'ne_iso_a3':iso,'ne_un_a3':row.get('UN_A3')}
        if iso in by_iso:
            reference=by_iso[iso]
            record.update(reference_m49=reference['M49 Code'],code_agrees=row.get('UN_A3')==reference['M49 Code'])
            matched.append(record)
            duplicate_iso[iso].append(row[code_field])
        else:
            unmatched.append(record)
    used={r['ne_iso_a3'] for r in matched}
    return {'scope':'Reference-only ISO code comparison, not a sovereignty/recognition decision; no M49 table or region labels emitted into editions.',
            'm49_reference_records':len(rows),'ne_records':len(political),'iso_joined_ne_records':len(matched),
            'distinct_reference_codes_represented':len(used),'unrepresented_reference_iso_codes':sorted(set(by_iso)-used),
            'ne_unjoined_records':unmatched,
            'ne_un_code_missing':[r for r in matched if not re.fullmatch(r'\d{3}',r['ne_un_a3'] or '')],
            'ne_un_code_conflicts':[r for r in matched if re.fullmatch(r'\d{3}',r['ne_un_a3'] or '') and not r['code_agrees']],
            'ne_multiple_records_for_iso':{k:v for k,v in duplicate_iso.items() if len(v)>1},
            'join_policy':'Natural Earth ISO_A3_EH when a valid three-letter code, else ISO_A3; sentinel codes are never guessed from names.'}


def analyze(cache,references):
    ne={s['id']:s for s in locked_sources()}
    _,places=read_source(ne['ne_10m_populated_places'],cache)
    _,countries=read_source(ne['ne_10m_admin_0_countries'],cache)
    _,units=read_source(ne['ne_10m_admin_0_map_units'],cache)
    refs={s['id']:s for s in reference_sources()}
    un=refs['un-m49-overview']
    un_rows=m49_rows(verified_path(un,references).read_bytes())
    m49=compare_m49(un_rows,countries)
    m49_units=compare_m49(un_rows,units,'GU_A3')
    union_codes={r.get('ISO_A3_EH') if re.fullmatch(r'[A-Z]{3}',r.get('ISO_A3_EH','')) else r.get('ISO_A3') for r,p in countries+units}
    m49_union={'reference_iso_codes_covered_by_country_or_map_unit':len({r['ISO-alpha3 Code'] for r in un_rows}.intersection(union_codes)),
               'uncovered_reference_iso_codes':sorted({r['ISO-alpha3 Code'] for r in un_rows}-union_codes),
               'warning':'Coverage of a code is not a one-to-one identity join. Layers and aggregates remain source-scoped.'}
    cases=[]
    for source in reference_sources():
        if not source['id'].startswith('wikidata-'):
            continue
        raw=json.loads(verified_path(source,references).read_bytes())
        entity=raw['entities'][source['entity_id']]
        if entity.get('lastrevid')!=source['lastrevid']:
            raise ValueError('Wikidata revision mismatch')
        qid=entity['id']
        coords=earth_coordinates(entity)
        rows=[]
        for kind,records in [('place',places),('map-unit',units)]:
            for row,parts in records:
                if row.get('WIKIDATAID')!=qid:
                    continue
                if kind=='place':
                    p=parts[0][0]
                else:
                    p=(number(row,'LABEL_X'),number(row,'LABEL_Y'))
                distance=min((distance_km(p,c) for c in coords),default=None)
                rows.append({'layer':kind,'ne_id':row['NE_ID'],'ne_name':row.get('NAME') or row.get('ADMIN'),
                             'ne_adm0_a3':row.get('ADM0_A3'),'coordinate':list(p),
                             'minimum_reference_coordinate_distance_km':round(distance,3) if distance is not None else None,
                             'coordinate_review_flag':distance is not None and distance>25,
                             'ne_capital_flag':number(row,'ADM0CAP')==1 if kind=='place' else None})
        populations=[]
        for claim in active_claims(entity,'P1082'):
            value=claim['mainsnak']['datavalue']['value']
            populations.append({'value':value.get('amount'),'rank':claim['rank'],
                                'dates':[q['datavalue']['value'].get('time') for q in claim.get('qualifiers',{}).get('P585',[]) if 'datavalue' in q],
                                'qualifier_properties':sorted(claim.get('qualifiers',{})),
                                'reference_count':len(claim.get('references',[]))})
        cases.append({'qid':qid,'revision':entity['lastrevid'],'modified':entity.get('modified'),
                      'english_label':entity.get('labels',{}).get('en',{}).get('value'),
                      'earth_coordinates':coords,'country_claims':entity_ids(entity,'P17'),
                      'instance_claims':entity_ids(entity,'P31'),'capital_of_claims':entity_ids(entity,'P1376'),
                      'population_statements':populations,'natural_earth_records':rows})
    return {'schema':1,'reference_lock_sha256':hashlib.sha256((ATLAS/'references.lock.json').read_bytes()).hexdigest(),
            'm49':m49,'m49_map_units':m49_units,'m49_country_map_unit_union':m49_union,'wikidata_cases':cases,
            'summary':{'entity_snapshots':len(cases),'matched_ne_records':sum(len(c['natural_earth_records']) for c in cases),
                       'coordinate_flags_over_25km':[{'qid':c['qid'],**r} for c in cases for r in c['natural_earth_records'] if r['coordinate_review_flag']]},
            'limitations':['Targeted sample, not a full Wikidata crosswalk.','Wikidata coordinates are claims, not a survey oracle; a 25km review flag is heuristic and does not authorize replacement.',
                           'Map-unit label points are not entity centroids; point distance alone does not establish identity mismatch.',
                           'Best-rank population statements retain dates/qualifier property IDs; definitions and current capital/country roles need editorial review.',
                           'UN webpage snapshot is a research reference; no redistribution permission for its full table is asserted.']}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache',type=Path)
    parser.add_argument('references',type=Path)
    parser.add_argument('output',type=Path,nargs='?')
    parser.add_argument('--download',action='store_true',help='explicitly acquire missing pinned reference snapshots')
    args=parser.parse_args()
    if args.download:
        for source in reference_sources():
            acquire_reference(source,args.references)
    if args.output:
        report(args.output,analyze(args.cache,args.references))
    else:
        for source in reference_sources():
            verified_path(source,args.references)
            print(source['id'],'verified')
