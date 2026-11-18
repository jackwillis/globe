#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Descriptive raw-chain bbox/orientation census, no image/reference computation."""
import json,sys,math,time
from pathlib import Path
from retained import read_geometry
start=time.process_time();ff=read_geometry(Path('/tmp/pe-aa-phase3b-fixtures/view5-r220-relief.geometry'));rows=[{'segment_boxes':0,'join_boxes':0,'near_axis_segments':0,'near_diagonal_segments':0,'features':set(),'colors':set()} for _ in range(4800)]
def add(a,b,feature,color,join=False):
 x0,y0=min(a[0],b[0])-.5,min(a[1],b[1])-.5;x1,y1=max(a[0],b[0])+.5,max(a[1],b[1])+.5
 dx,dy=abs(b[0]-a[0]),abs(b[1]-a[1]);large=max(dx,dy);small=min(dx,dy)
 # Descriptive floats only; these counts are NOT enclosure certificates.
 for y in range(max(0,math.ceil(y0/8)-1),min(59,math.floor(y1/8))+1):
  for x in range(max(0,math.ceil(x0/8)-1),min(79,math.floor(x1/8))+1):
   r=rows[80*y+x];r['join_boxes' if join else 'segment_boxes']+=1;r['features'].add(feature);r['colors'].add(color)
   if not join:r['near_axis_segments']+=small<=.01*large;r['near_diagonal_segments']+=abs(dx-dy)<=.01*large
for feature,(_,color,chains) in enumerate(ff):
 for points,closed in chains:
  p=[]
  for q in points:
   if not p or q!=p[-1]:p.append(q)
  if closed and len(p)>1 and p[-1]==p[0]:p.pop()
  if len(p)<2:continue
  for a,b in zip(p,p[1:]+([p[0]] if closed else [])):
   if a!=b:add(a,b,feature,color)
  for a in (p if closed else p[1:-1]):add(a,a,feature,color,True)
for r in rows:r['feature_count']=len(r.pop('features'));r['color_count']=len(r.pop('colors'))
out=Path(sys.argv[1]);(out/'geometry-descriptors.json').write_text(json.dumps({'rows':rows,'cpu_seconds':time.process_time()-start,'descriptive_only':True},separators=(',',':')))
