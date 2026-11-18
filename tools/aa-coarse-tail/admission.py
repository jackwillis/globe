#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,gzip,hashlib
from pathlib import Path

def validate(r,i):
 if not(r.get('complete') is True and r.get('tile_index')==i and r.get('tile')==[i%80,i//80] and r.get('target_den')==2560 and len(r.get('pixels',[]))==64):raise ValueError('tile identity/count/completion')
 for k,p in enumerate(r['pixels']):
  if p['pixel']!=[(i%80)*8+k%8,(i//80)*8+k//8]:raise ValueError('pixel identity')
  lo,hi,d=p['lower_numerator'],p['upper_numerator'],p['denominator']
  if d!=255*4**16 or len(lo)!=3 or len(hi)!=3 or any(type(x)!=int for x in lo+hi) or not all(0<=a<=b<=d for a,b in zip(lo,hi)):raise ValueError('integer enclosure')
  if p['state'] not in ['precision_reached','cell_budget','time_budget','depth_budget']:raise ValueError('pixel state')
  precise=max(b-a for a,b in zip(lo,hi))*2560<=2*d
  if p['precision_reached']!=precise or (p['state']=='precision_reached')!=precise or not 1<=p['cell_visits']<=100000 or p['pixel_cell_quota']!=100000 or not 0<=p['deepest']<=16:raise ValueError('precision/quota')
  if p['quantized_codes']!=[[(2*m*a+d)//(2*d),(2*m*b+d)//(2*d)] for a,b,m in zip(lo,hi,[31,63,31])]:raise ValueError('quantization')
 if r['target_attained']!=all(p['precision_reached'] for p in r['pixels']):raise ValueError('tile precision')
 return r

def commit(out,r):
 from accounting import atomic
 i=r['tile_index'];validate(r,i);path=Path(out)/'tiles'/f'{i:04d}.json.gz'
 if path.exists():raise RuntimeError('refuse completed replacement')
 raw=gzip.compress(json.dumps(r,separators=(',',':')).encode(),compresslevel=1,mtime=0);atomic(path,raw)
 return {'index':i,'sha256':hashlib.sha256(raw).hexdigest(),'bytes':len(raw),'precision':r['target_attained'],'cpu':r['cpu_seconds']}
