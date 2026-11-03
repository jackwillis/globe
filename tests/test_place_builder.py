#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('builder',ROOT/'tools/places-builder/build_places.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
assert m.encode([(1,'Front',0,0),(2,'Back',150,0)])==(ROOT/'tests/data/places.gplc').read_bytes()
records=m.source(ROOT/'data/wikidata-cities-20261101.json')
assert len(records)==19 and all(r[0]!=3139 for r in records)
assert m.encode(records)==(ROOT/'tests/data/cities-starter19.gplc').read_bytes()
# The bundled pack: population >= 1,000,000, at most 30 per country, from the pinned large snapshot.
large=m.large_source(ROOT/'data/wikidata-cities-large-20261102.json')
assert 300<=len(large)<=500 and len(large)<=4096, len(large)
assert m.encode(large)==(ROOT/'data/cities.gplc').read_bytes()
assert len({r[0] for r in large})==len(large)
names={r[1] for r in large}
assert {'Shanghai','London','Cairo','Sao Paulo'} & {m.ascii_name(n).decode() for n in names}
# No country contributes more than the cap: China would otherwise supply about half the pack.
raw=__import__('json').loads((ROOT/'data/wikidata-cities-large-20261102.json').read_bytes())['results']['bindings']
china=[b for b in raw if b['country']['value'].endswith('/Q148') and float(b['pop']['value'])>=m.MIN_POPULATION]
assert len(china)>m.PER_COUNTRY
chosen={r[0] for r in large}
assert sum(1 for b in china if int(b['item']['value'].rsplit('/Q',1)[1]) in chosen)==m.PER_COUNTRY
# The largest city of a capped country is kept, the smallest dropped.
china.sort(key=lambda b:(-float(b['pop']['value']),int(b['item']['value'].rsplit('/Q',1)[1])))
assert int(china[0]['item']['value'].rsplit('/Q',1)[1]) in chosen and int(china[-1]['item']['value'].rsplit('/Q',1)[1]) not in chosen
# Transliteration is explicit: letters NFKD leaves behind are mapped, anything else is an error.
assert m.ascii_name('Diyarbak\u0131r')==b'Diyarbakir' and m.ascii_name('Th\u1ee7 \u0110\u1ee9c')==b'Thu Duc'
assert m.ascii_name('S\u00e3o Paulo')==b'Sao Paulo' and m.ascii_name('\u00c5lesund')==b'Alesund'
for bad in ['\u5317\u4eac','\u0e01']:
    try:m.ascii_name(bad)
    except ValueError:pass
    else:raise AssertionError('untransliterated name accepted')
for records in [[(1,'Bad',181,0)],[(0,'Bad',0,0)],[(1,'Bad\0',0,0)],[(1,'A',0,0),(1,'B',1,1)]]:
    try:m.encode(records)
    except ValueError:pass
    else:raise AssertionError('accepted invalid input')
def row(q,name,pop,country=None,coord='Point(1 2)'):
    r={'item':{'value':'http://www.wikidata.org/entity/Q%d'%q},'itemLabel':{'value':name},'coord':{'value':coord},'pop':{'value':str(pop)}}
    if country:r['country']={'value':'http://www.wikidata.org/entity/Q%d'%country}
    return r
M=m.MIN_POPULATION
picked=m.select_large([row(10,'AtMin',M,1),row(11,'BelowMin',M-1,2),row(3139,'Dept',5*M,3),
    row(20,'Dup',M+1,4),row(20,'Dup',4*M,4),row(30,'NoCountryA',M),row(31,'NoCountryB',M),row(12,'Mid',2*M,5)])
assert [r[0] for r in picked]==[20,12,10,30,31],picked
assert [r[1] for r in picked][0]=='Dup'
# A repeated Q keeps its largest population; and the one record per Q is what is counted against the cap.
capped=m.select_large([row(100+i,'C%d'%i,M+i,7) for i in range(m.PER_COUNTRY+3)])
assert len(capped)==m.PER_COUNTRY and capped[0][0]==100+m.PER_COUNTRY+2 and capped[-1][0]==103
assert m.ascii_name(' Pad ')==b'Pad'
# Cities without a country each count as their own country, so the cap never merges them.
loose=m.select_large([row(500+i,'L%d'%i,M+i) for i in range(m.PER_COUNTRY+3)])
assert len(loose)==m.PER_COUNTRY+3
print('ok')
