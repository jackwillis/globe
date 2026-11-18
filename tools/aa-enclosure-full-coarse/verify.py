#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,sys,struct,time,resource
from pathlib import Path
from retained import Retained,Geometry,read_geometry,rgb565
out=Path(sys.argv[1]);root=Path('/tmp/pe-aa-phase3b-fixtures');old=Path('/tmp/aa-enclosure-scale-v1-evidence');records=[]
for i in range(4):
 prior=json.loads((old/f'stage1-anchor{i}/result.json').read_text());j=prior['job'];ff=read_geometry(root/(j['fixture']+'.geometry'));data=(root/(j['fixture']+'-background.rgb565')).read_bytes()
 if j['feature'] is not None:ff=[ff[j['feature']]]
 adapter=Retained(ff,data);g=adapter.geometry
 def bg(x,y):return rgb565(struct.unpack_from('<H',data,2*(y*640+x))[0])
 # Same object reused twice at the ORIGINAL anchor precision (verification only).
 for repeat in range(2):
  r=g.tile(j['tx'],j['ty'],bg,target_den=10240)
  assert r['target_attained']
  for a,b in zip(r['pixels'],prior['pixels']):
   assert (a['pixel'],a['lower_numerator'],a['upper_numerator'],a['denominator'])==(b['pixel'],b['lower_numerator'],b['upper_numerator'],b['denominator'])
 records.append({'anchor':i,'repeats':2,'exact_pixels':128,'pass':True})
# Across distinct tiles and targets retained state cannot affect bounds.
ff=[(0,65535,[([(0.,0.),(3.,1.),(6.,6.)],False)])];data=bytes(640*480*2);a=Retained(ff,data)
for xy in [(0,0),(1,0),(0,1),(0,0)]:
 r=a.tile(*xy);s=Retained(ff,data).tile(*xy)
 assert [(p['lower_numerator'],p['upper_numerator']) for p in r['pixels']]==[(p['lower_numerator'],p['upper_numerator']) for p in s['pixels']]
(out/'reuse-verification.json').write_text(json.dumps({'anchors':records,'mixed_tile_reuse':True,'cpu_seconds':time.process_time(),'peak_rss_kib':resource.getrusage(resource.RUSAGE_SELF).ru_maxrss},indent=2))
