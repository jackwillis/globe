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
# The inspected response included this French department; it is not a city.
EXCLUDED={3139}
def encode(records):
    table=bytearray();text=bytearray();previous=0
    if not 1<=len(records)<=4096:raise ValueError('count')
    for qid,name,lon,lat in sorted(records):
        title=unicodedata.normalize('NFKD',name).encode('ascii','ignore')
        if not previous<qid<=0xffffffff or not 1<=len(title)<=95 or any(c<32 or c>126 for c in title):raise ValueError('record')
        if not math.isfinite(lon+lat) or not -180<=lon<=180 or not -90<=lat<=90:raise ValueError('coordinate')
        table+=struct.pack('<IiiIII',qid,round(lon*1e6),round(lat*1e6),len(text),len(title),0);text+=title;previous=qid
    header=struct.pack('<4sHHIIII',b'GPLC',1,0,len(records),len(text),0,0)
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
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('snapshot');p.add_argument('output');a=p.parse_args()
    data=encode(source(a.snapshot));open(a.output,'wb').write(data);print(len(data),hashlib.sha256(data).hexdigest())
