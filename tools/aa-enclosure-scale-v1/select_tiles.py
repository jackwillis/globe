#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Geometry-only complexity census and deterministic selection, no reference outputs."""
import sys,json,hashlib,math
from pathlib import Path
from enclose import Geometry
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-experiment'))
from reference import read_geometry
root=Path(sys.argv[1]);population={};jobs=[];hashes={}
selected_geometries={(0,220),(1,440),(4,440),(7,220)}
for view in range(8):
 for radius in (220,440):
  for style in ('map','relief'):
   stem=f'view{view}-r{radius}-{style}';path=Path('/tmp/pe-aa-phase3b-fixtures')/(stem+'.geometry')
   raw=path.read_bytes();hashes[stem]=hashlib.sha256(raw).hexdigest();g=Geometry(read_geometry(path));counts=[0]*4800;S=g.scale*8
   for _,_,pp in g.features:
    for _,(x0,y0,x1,y1),_ in pp:
     for x in range(max(0,x0//S),min(79,x1//S)+1):
      for y in range(max(0,y0//S),min(59,y1//S)+1):counts[y*80+x]+=1
   positive=sorted(c for c in counts if c>0)
   targets={'empty':0,'low':positive[0],'median':positive[(len(positive)-1)//2],'p95':positive[math.ceil(.95*len(positive))-1],'maximum':positive[-1]}
   population[stem]=dict(counts=counts,targets=targets,nonempty=len(positive))
   if (view,radius) in selected_geometries:
    for label,target in targets.items():
     possible=[i for i,c in enumerate(counts) if c==target]
     pick=min(possible,key=lambda i:hashlib.sha256(f'20261118|{stem}|{label}|{i}'.encode()).digest())
     jobs.append(dict(fixture=stem,tx=pick%80,ty=pick//80,feature=None,target_den=2560,stratum=label,complexity=target,mode='full_color'))
anchors=[('view4-r440-relief',None,0,32),('view0-r220-map',None,16,36),('view4-r440-map',1,2,57),('view0-r220-map',2,39,43)]
for stem,feature,x,y in anchors:jobs.append(dict(fixture=stem,feature=feature,tx=x,ty=y,target_den=2560,stratum='original_anchor',mode='full_color'))
# Eight predetermined harder-stratum repeats, selected independently of measured outputs.
fine=[dict(j,target_den=10240,stratum=j['stratum']+'-fine') for j in jobs[:40] if j['stratum'] in ('median','p95','maximum')][:8]
jobs.extend(fine)
for stem,feature,x,y in anchors[2:]:jobs.append(dict(fixture=stem,feature=feature,tx=x,ty=y,target_den=10240,stratum='coverage_sample',mode='coverage',coverage=True))
assert len(jobs)==54 and len(jobs)+4<=64
(root/'population.json').write_text(json.dumps(population)+'\n')
(root/'selection.json').write_text(json.dumps(dict(rule='32-condition geometry-only conservative primitive/bbox-overlap tile counts;8 fixed fixture conditions from anchor/coast/dateline/ocean geometries xMap/Relief; each empty,min-positive,median-positive,p95-positive,max counts; tie SHA256seed20261118;4 original anchors;8 deterministic harder-stratum fine repeats;2 isolated coverage jobs',jobs=jobs,stage2_jobs=len(jobs),including_stage1_jobs=len(jobs)+4,unique_tile_coordinates=len({(j['fixture'],j['tx'],j['ty']) for j in jobs}),input_geometry_sha256=hashes,limits=dict(stage2_cpu_seconds=900,total_envelope_cpu_seconds=1200,reserved_controls_setup_seconds=300,tile_wall_seconds=90,process_address_bytes=384*1024**2,pixel_cells=100000,pixel_wall_seconds=75/64,max_depth=16),original_protocol_amended=False),indent=2)+'\n')
print('frozen jobs',len(jobs),'plus4 stage1 anchors; unique fixture/tile coordinates',len({(j['fixture'],j['tx'],j['ty']) for j in jobs}))
