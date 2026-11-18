#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Resource/uncertainty analysis only, no candidate or quality metric inputs."""
import json,sys,math,statistics,hashlib,csv
from pathlib import Path
root=Path(sys.argv[1]);states=json.loads((root/'execution-statuses.json').read_text());selection=json.loads((root/'selection.json').read_text());population=json.loads((root/'population.json').read_text())
rows=[]
for status in states['states']:
 i=status['index'];path=root/f'job{i:02d}'/'result.json'
 if not path.exists():rows.append(dict(index=i,status=status));continue
 try:r=json.loads(path.read_text())
 except json.JSONDecodeError:rows.append(dict(index=i,status=status));continue
 if 'pixels' not in r:rows.append(dict(index=i,status=status,error=r.get('error')));continue
 p=r['pixels'];row=dict(index=i,job=r['job'],target_attained=r['target_attained'],cpu=r['cpu_seconds'],total_cpu=r['total_cpu_seconds'],wall=r['total_wall_seconds'],setup_cpu=r['setup_index_cpu_seconds'],initial_cpu=r['initial_classification_cpu_seconds'],rss_kib=r['peak_rss_kib'],cells=r['cell_visits'],refinement_cells=r['cell_visits']-64,predicates=r['predicate_calls'],initial_predicates=r['initial_predicate_calls'],bbox_calls=r['bbox_calls'],max_depth=max(x['deepest'] for x in p),max_radius=r['max_radius'],unresolved_area=sum(x['unresolved_area_numerator']/x['area_denominator'] for x in p),pixel_states={s:sum(x['state']==s for x in p) for s in sorted({x['state'] for x in p})},rgb_ambiguous_pixels=sum(any(a!=b for a,b in x['quantized_codes']) for x in p),rgb_ambiguous_channels=sum(a!=b for x in p for a,b in x['quantized_codes']))
 if r['job'].get('coverage'):
  row['alpha_ambiguous_pixels']=sum(x['alpha_codes'][0]!=x['alpha_codes'][1] for x in p)
  row['potential_partial_pixels']=sum(x['upper_numerator'][0]>0 and x['lower_numerator'][0]<x['denominator'] for x in p)
 rows.append(row)
(root/'job-summary.json').write_text(json.dumps(rows,indent=2)+'\n')
coarse=[r for r in rows if r.get('job',{}).get('stratum') in ('empty','low','median','p95','maximum')]
fine=[r for r in rows if r.get('job',{}).get('stratum','').endswith('-fine')]
matched=[]
for f in fine:
 c=next(c for c in coarse if all(c['job'][k]==f['job'][k] for k in ('fixture','tx','ty')))
 matched.append(dict(fixture=f['job']['fixture'],stratum=c['job']['stratum'],cpu_ratio=f['cpu']/c['cpu'] if c['cpu'] else None,coarse_cpu=c['cpu'],fine_cpu=f['cpu'],both_precision=c['target_attained'] and f['target_attained']))
(root/'precision-cost.json').write_text(json.dumps(matched,indent=2)+'\n')
# Piecewise-linear density model per measured condition. This is not a statistical CI.
curves={}
for r in coarse:curves.setdefault(r['job']['fixture'],[]).append((r['job']['complexity'],r['cpu']))
def predict(points,x):
 unique={}
 for n,t in points:unique.setdefault(n,[]).append(t)
 q=sorted((n,statistics.median(v)) for n,v in unique.items())
 if x<=q[0][0]:return q[0][1]
 for (a,u),(b,v) in zip(q,q[1:]):
  if x<=b:return u+(v-u)*(x-a)/(b-a)
 # Outside observed complexity support: hold last value, flag limitations in report.
 return q[-1][1]
projections=[]
ratios=[x['cpu_ratio'] for x in matched if x['both_precision']]
ratio_mid=statistics.median(ratios);ratio_lo=min(ratios);ratio_hi=max(ratios)
for stem,pop in population.items():
 style=stem.rsplit('-',1)[-1];pool={s:c for s,c in curves.items() if s.endswith('-'+style)}
 scenarios={s:sum(predict(c,n) for n in pop['counts']) for s,c in pool.items()}
 own=scenarios.get(stem);middle=own if own is not None else statistics.median(scenarios.values())
 outside=sum(n>max(a for c in pool.values() for a,b in c) for n in pop['counts'])
 projections.append(dict(fixture=stem,nonempty_tiles=pop['nonempty'],coarse_cpu_seconds_central=middle,coarse_cpu_seconds_scenario_min=min(scenarios.values()),coarse_cpu_seconds_scenario_max=max(scenarios.values()),fine_cpu_seconds_central=middle*ratio_mid,fine_cpu_seconds_scenario_min=min(scenarios.values())*ratio_lo,fine_cpu_seconds_scenario_max=max(scenarios.values())*ratio_hi,tiles_beyond_style_complexity_support=outside,method='own five-anchor curve when available, otherwise style-pool median; per-condition scenarios not confidence intervals'))
(root/'resource-projections.json').write_text(json.dumps(dict(rows=projections,coarse_32_cpu_hours=sum(r['coarse_cpu_seconds_central'] for r in projections)/3600,coarse_32_scenario_hours=[sum(r['coarse_cpu_seconds_scenario_min'] for r in projections)/3600,sum(r['coarse_cpu_seconds_scenario_max'] for r in projections)/3600],fine_32_cpu_hours=sum(r['fine_cpu_seconds_central'] for r in projections)/3600,fine_32_scenario_hours=[sum(r['fine_cpu_seconds_scenario_min'] for r in projections)/3600,sum(r['fine_cpu_seconds_scenario_max'] for r in projections)/3600],fine_coarse_ratio_median=ratio_mid,fine_coarse_ratio_range=[ratio_lo,ratio_hi],unsupported_density_tiles=sum(r['tiles_beyond_style_complexity_support'] for r in projections),censored_jobs=sum(not r.get('target_attained',False) for r in rows),model_is_not_measured_full_frame=True),indent=2)+'\n')
with (root/'job-summary.csv').open('w') as out:
 keys=['index','cpu','wall','rss_kib','cells','predicates','initial_predicates','max_depth','max_radius','target_attained','rgb_ambiguous_pixels']
 w=csv.writer(out);w.writerow(['fixture','stratum','target_den']+keys)
 for r in rows:
  if 'job' in r:w.writerow([r['job']['fixture'],r['job']['stratum'],r['job']['target_den']]+[r.get(k) for k in keys])
print('summarized',len(rows),'jobs,precision complete',sum(r.get('target_attained',False) for r in rows))
