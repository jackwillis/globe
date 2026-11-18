#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Exploratory resource descriptors, no candidate comparisons or causal claims."""
import json,sys,statistics,math,time
from pathlib import Path
start=time.process_time();p=Path(sys.argv[1]);rows=json.loads((p/'tile-summary.json').read_text());desc=json.loads((p/'geometry-descriptors.json').read_text())['rows']
for r in rows:r.update(desc[r['index']])
def corr(xs,ys):
 a,b=statistics.mean(xs),statistics.mean(ys);d=sum((x-a)**2 for x in xs)*sum((y-b)**2 for y in ys)
 return sum((x-a)*(y-b) for x,y in zip(xs,ys))/math.sqrt(d) if d else None
nonempty=[r for r in rows if r['complexity']>0]
cc={k:corr([r[k] for r in nonempty],[r['cpu'] for r in nonempty]) for k in ['complexity','feature_count','near_axis_segments','near_diagonal_segments','initial_predicates','predicates','cells']}
spread=[]
for n in sorted({r['complexity'] for r in rows}):
 rr=[r for r in rows if r['complexity']==n]
 if len(rr)>=10 and n>0:spread.append({'density':n,'tiles':len(rr),'min_cpu':min(r['cpu'] for r in rr),'median_cpu':statistics.median(r['cpu'] for r in rr),'max_cpu':max(r['cpu'] for r in rr)})
result={'nonempty_tiles':len(nonempty),'descriptive_pearson_cpu_correlations':cc,'same_density_spread':spread,'top_cpu_tiles':sorted(rows,key=lambda r:r['cpu'],reverse=True)[:12],'top_density_tiles':sorted(rows,key=lambda r:r['complexity'],reverse=True)[:12],'primitive_box_census_disagreements':sum(r['complexity']!=r['segment_boxes']+r['join_boxes'] for r in rows),'cpu_seconds':time.process_time()-start,'limitations':'Descriptive bbox overlap and raw-chain orientation. Not causal separation of overlap, source ordering, contrast or orientation. Floating descriptors are not conservative certificates.'}
(p/'cost-diagnostics.json').write_text(json.dumps(result,indent=2));print(json.dumps({k:v for k,v in result.items() if k not in ['same_density_spread','top_cpu_tiles','top_density_tiles']}))
