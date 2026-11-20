#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Build GPLC v1 from the pinned Wikidata structured-data snapshot; stdlib only."""
import argparse
import hashlib
import json
import math
import re
import struct
import unicodedata
import zlib
SOURCE_SHA='cbd362a13849bbd00b78d43c29937a383e79796524f87d754f2b8e179cc3c9da'
LARGE_SHA='88958f1d3f969c3d2423f299e2c460b397ff1af7dd3c61273d0a3098d569b98f'
# Selection rule for the large snapshot (query: cities-large.sparql, population >= 1,000,000 there).
MIN_POPULATION=1000000
PER_COUNTRY=30
# GPLC v2 population tiers: the smallest population that still belongs to tier 0, 1 and 2; anything
# else (down to MIN_POPULATION) is tier 3. They decide which cities are named at which zoom.
TIER_FLOORS=(10000000,5000000,2500000)
def tier_of(population):
    for tier,floor in enumerate(TIER_FLOORS):
        if population>=floor:return tier
    return len(TIER_FLOORS)
# Letters that NFKD does not decompose into ASCII plus a combining mark.
TRANSLIT={'\u0131':'i','\u0110':'D','\u0111':'d','\u00d8':'O','\u00f8':'o','\u0141':'L','\u0142':'l','\u00df':'ss','\u00c6':'AE','\u00e6':'ae','\u0152':'OE','\u0153':'oe'}
def ascii_name(name):
    folded=unicodedata.normalize('NFKD',''.join(TRANSLIT.get(c,c) for c in name))
    kept=''.join(c for c in folded if unicodedata.category(c)[0]!='M')
    if not kept.isascii():raise ValueError('untransliterated character in %r'%name)
    return kept.encode('ascii').strip()
# The inspected response included this French department; it is not a city.
EXCLUDED={3139}
def encode(records,version=1):
    """Version 2 records are (qid, name, lon, lat, tier); version 1 records have no tier."""
    if version not in (1,2):raise ValueError('version')
    table=bytearray();text=bytearray();previous=0
    if not 1<=len(records)<=4096:raise ValueError('count')
    for qid,name,lon,lat,*extra in sorted(records):
        if len(extra)!=(version==2):raise ValueError('tier')
        tier=extra[0] if version==2 else 0
        if not 0<=tier<=len(TIER_FLOORS):raise ValueError('tier')
        title=ascii_name(name)
        if not previous<qid<=0xffffffff or not 1<=len(title)<=95 or any(c<32 or c>126 for c in title):raise ValueError('record')
        if not math.isfinite(lon+lat) or not -180<=lon<=180 or not -90<=lat<=90:raise ValueError('coordinate')
        table+=struct.pack('<IiiIII',qid,round(lon*1e6),round(lat*1e6),len(text),len(title),tier);text+=title;previous=qid
    header=struct.pack('<4sHHIIII',b'GPLC',version,0,len(records),len(text),0,0)
    result=bytearray(header+table+text)
    struct.pack_into('<I',result,20,zlib.crc32(result[24:],zlib.crc32(result[:20])))
    return bytes(result)
def source(path):
    raw=open(path,'rb').read()
    if hashlib.sha256(raw).hexdigest()!=SOURCE_SHA:raise ValueError('source hash')
    records={}
    for row in json.loads(raw)['results']['bindings']:
        qid=int(row['item']['value'].rsplit('/Q',1)[1])
        if qid in EXCLUDED:continue
        m=re.fullmatch(r'Point\(([-+0-9.eE]+) ([-+0-9.eE]+)\)',row['coord']['value'])
        if not m:raise ValueError('coordinate syntax')
        record=(qid,row['itemLabel']['value'],*map(float,m.groups()))
        if qid not in records or record<records[qid]:records[qid]=record
    return list(records.values())
def large_source(path):
    """Cities with population >= MIN_POPULATION, at most PER_COUNTRY per country by population."""
    raw=open(path,'rb').read()
    if hashlib.sha256(raw).hexdigest()!=LARGE_SHA:raise ValueError('source hash')
    return select_large(json.loads(raw)['results']['bindings'])
def select_large(bindings):
    rows={}
    for row in bindings:
        qid=int(row['item']['value'].rsplit('/Q',1)[1])
        if qid in EXCLUDED:continue
        m=re.fullmatch(r'Point\(([-+0-9.eE]+) ([-+0-9.eE]+)\)',row['coord']['value'])
        if not m:raise ValueError('coordinate syntax')
        pop=float(row['pop']['value'])
        country=row['country']['value'] if 'country' in row else row['item']['value']
        item=(-pop,qid,row['itemLabel']['value'],*map(float,m.groups()),country)
        if qid not in rows or item<rows[qid]:rows[qid]=item
    taken={};chosen=[]
    for negative,qid,name,lon,lat,country in sorted(rows.values()):
        if -negative<MIN_POPULATION or taken.get(country,0)>=PER_COUNTRY:continue
        taken[country]=taken.get(country,0)+1;chosen.append((qid,name,lon,lat,tier_of(-negative)))
    return chosen
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('snapshot');p.add_argument('output');p.add_argument('--large',action='store_true',help='select from the large population snapshot');a=p.parse_args()
    data=encode((large_source if a.large else source)(a.snapshot),2 if a.large else 1);open(a.output,'wb').write(data);print(len(data),hashlib.sha256(data).hexdigest())
