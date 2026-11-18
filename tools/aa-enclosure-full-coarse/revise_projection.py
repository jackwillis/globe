#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Censored-prefix resource sensitivity, not a measured full-frame completion cost."""
import json,sys,statistics,time
from pathlib import Path
start=time.process_time();p=Path(sys.argv[1]);old=Path(sys.argv[2]);a=json.loads((p/'analysis.json').read_text());pop=json.loads((old/'population.json').read_text());prior=json.loads((old/'resource-projections.json').read_text());jobs=json.loads((old/'job-summary.json').read_text());curves={}
for r in jobs:
 if r.get('job',{}).get('stratum') in ['empty','low','median','p95','maximum'] and r['job']['fixture'].endswith('-relief'):curves.setdefault(r['job']['fixture'],[]).append((r['job']['complexity'],r['cpu']))
def predict(points,n):
 unique={}
 for x,t in points:unique.setdefault(x,[]).append(t)
 q=sorted((x,statistics.median(t)) for x,t in unique.items())
 if n<=q[0][0]:return q[0][1]
 for (lo,a),(hi,b) in zip(q,q[1:]):
  if n<=hi:return a+(b-a)*(n-lo)/(hi-lo)
 return q[-1][1]
n=a['completed_tiles'];counts=pop['view5-r220-relief']['counts'];scenarios=[]
for stem,curve in curves.items():
 before=sum(predict(curve,x) for x in counts[:n]);after=sum(predict(curve,x) for x in counts[n:]);ratio=a['totals']['cpu']/before
 scenarios.append({'curve':stem,'old_prefix_prediction':before,'old_remaining_prediction':after,'prefix_calibration':ratio,'conditional_tail_seconds':after*ratio,'conditional_full_seconds':a['totals']['cpu']+after*ratio})
prefix=statistics.median(s['old_prefix_prediction'] for s in scenarios);tail=statistics.median(s['old_remaining_prediction'] for s in scenarios);calibration=a['totals']['cpu']/prefix;full=a['totals']['cpu']+tail*calibration;oldfixture=a['old_prediction']['coarse_cpu_seconds_central'];ratio=full/oldfixture
replace=sum((full if r['fixture']=='view5-r220-relief' else r['coarse_cpu_seconds_central']) for r in prior['rows'])/3600
transfer=sum(r['coarse_cpu_seconds_central']*(ratio if r['fixture'].endswith('-relief') else 1) for r in prior['rows'])/3600
out={'completed_prefix_tiles':n,'remaining_tiles':4800-n,'remaining_nonempty':sum(x>0 for x in counts[n:]),'remaining_empty':sum(x==0 for x in counts[n:]),'old_full_prediction':oldfixture,'old_prefix_prediction_style_median':prefix,'old_tail_prediction_style_median':tail,'measured_prefix_tile_cpu':a['totals']['cpu'],'measured_prefix_to_old_prefix':calibration,'conditional_full_tile_cpu':full,'conditional_full_cpu_scenario_range':[min(s['conditional_full_seconds'] for s in scenarios),max(s['conditional_full_seconds'] for s in scenarios)],'conditional_32_coarse_hours_replace_only_heldout':replace,'conditional_32_coarse_hours_transfer_relief':transfer,'scenarios':scenarios,'cpu_seconds':time.process_time()-start,'not_a_completion_guarantee':True,'limitations':'Row-major censoring leaves a structurally different tail. Transfer prefix ratio to unstarted tail and other Relief conditions is only a sensitivity model. Setup/checkpoint serialization outside tile clock, partial replay, sequential/sidecar/scoring cost excluded. No new fine estimate.'}
(p/'conditional-projection.json').write_text(json.dumps(out,indent=2));print(json.dumps({k:v for k,v in out.items() if k!='scenarios'},indent=2))
