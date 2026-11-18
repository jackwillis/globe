#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Verify reference certificates/resources only. Never opens candidate output."""
import json,gzip,hashlib,sys,time,statistics,csv,collections,resource
from pathlib import Path

def validate(r,index):
 assert r['complete'] is True and r['tile_index']==index and r['tile']==[index%80,index//80]
 assert r['target_den']==2560 and len(r['pixels'])==64
 states=collections.Counter()
 for k,p in enumerate(r['pixels']):
  assert p['pixel']==[(index%80)*8+k%8,(index//80)*8+k//8]
  d=p['denominator'];assert d==255*4**16
  assert all(isinstance(v,int) for v in p['lower_numerator']+p['upper_numerator'])
  assert all(0<=a<=b<=d for a,b in zip(p['lower_numerator'],p['upper_numerator']))
  exact=max(b-a for a,b in zip(p['lower_numerator'],p['upper_numerator']))*2560<=2*d
  assert p['precision_reached']==exact
  assert (p['state']=='precision_reached')==exact
  assert p['cell_visits']<=100000 and p['pixel_cell_quota']==100000 and p['deepest']<=16
  for a,b,m,q in zip(p['lower_numerator'],p['upper_numerator'],[31,63,31],p['quantized_codes']):assert q==[(2*m*a+d)//(2*d),(2*m*b+d)//(2*d)]
  states[p['state']]+=1
 assert r['target_attained']==all(p['precision_reached'] for p in r['pixels'])
 return states

def main(root,prior):
 start=time.process_time();execution=json.loads((root/'execution.json').read_text());manifest=[json.loads(x) for x in (root/'completed.jsonl').read_text().splitlines()];rows=[];allstates=collections.Counter();failures=[];den=255*4**16;widthsum=0;unknownsum=0;ambiguous=0;ambchannels=0
 pop=json.loads((prior/'population.json').read_text());counts=pop['view5-r220-relief']['counts']
 for rec in manifest:
  i=rec['index'];path=root/'tiles'/f'{i:04d}.json.gz';raw=path.read_bytes();assert hashlib.sha256(raw).hexdigest()==rec['sha256']
  r=json.loads(gzip.decompress(raw));states=validate(r,i);allstates.update(states);pp=r['pixels']
  row={'index':i,'tx':i%80,'ty':i//80,'complexity':counts[i],'precision':r['target_attained'],'cpu':r['cpu_seconds'],'wall':r['wall_seconds'],'predicates':r['predicate_calls'],'bboxes':r['bbox_calls'],'cells':r['cell_visits'],'initial_predicates':sum(p['initial_predicate_calls'] for p in pp),'initial_cpu':sum(p['initial_cpu_seconds'] for p in pp),'max_depth':max(p['deepest'] for p in pp),'max_radius':r['max_radius'],'mean_radius':r['tile_mae_uncertainty'],'unresolved_area':sum(p['unresolved_area_numerator']/p['area_denominator'] for p in pp),'rss_kib':r['peak_rss_kib'],'ambiguous_pixels':sum(any(a!=b for a,b in p['quantized_codes']) for p in pp),'ambiguous_channels':sum(a!=b for p in pp for a,b in p['quantized_codes'])}
  rows.append(row);widthsum+=sum(sum(b-a for a,b in zip(p['lower_numerator'],p['upper_numerator'])) for p in pp);unknownsum+=sum(p['unresolved_area_numerator'] for p in pp);ambiguous+=row['ambiguous_pixels'];ambchannels+=row['ambiguous_channels']
  for p in pp:
   if not p['precision_reached']:failures.append({'tile_index':i,**p})
 assert [r['index'] for r in rows]==list(range(len(rows)))
 assert len(rows)==execution['completed_tiles']
 # All final files must be accounted for. Temporary files cannot count as completed.
 assert len(list((root/'tiles').glob('*.json.gz')))==len(rows)
 inflight=json.loads((root/'inflight.json').read_text()) if (root/'inflight.json').exists() else None
 interrupted=execution['interrupted_tile']
 if inflight and inflight['tile_index']<len(rows):inflight_status='stale checkpoint of verified completed tile'
 else:inflight_status='interrupted partial tile' if inflight else 'none'
 totals={k:sum(r[k] for r in rows) for k in ['cpu','wall','predicates','bboxes','cells','initial_predicates','initial_cpu','ambiguous_pixels','ambiguous_channels','unresolved_area']}
 old=json.loads((prior/'resource-projections.json').read_text());pred=next(r for r in old['rows'] if r['fixture']=='view5-r220-relief');complete=len(rows)==4800
 calibration=totals['cpu']/pred['coarse_cpu_seconds_central'] if complete else None
 original=sum(r['coarse_cpu_seconds_central'] for r in old['rows']);replacement=original-pred['coarse_cpu_seconds_central']+execution['cpu_seconds'] if complete else None
 relieftransfer=sum((r['coarse_cpu_seconds_central']*calibration if r['fixture'].endswith('-relief') else r['coarse_cpu_seconds_central']) for r in old['rows']) if complete else None
 groups=[]
 for lo,hi in [(0,0),(1,5),(6,10),(11,20),(21,40),(41,80),(81,160),(161,1000000)]:
  rr=[r for r in rows if lo<=r['complexity']<=hi]
  if rr:groups.append({'density_range':[lo,hi],'tiles':len(rr),'cpu':sum(r['cpu'] for r in rr),'median_cpu':statistics.median(r['cpu'] for r in rr),'max_cpu':max(r['cpu'] for r in rr),'precision_limited':sum(not r['precision'] for r in rr)})
 summary={'execution':execution,'completed_tiles':len(rows),'precision_attained_tiles':sum(r['precision'] for r in rows),'precision_limited_tiles':sum(not r['precision'] for r in rows),'interrupted_tiles':int(interrupted is not None),'unstarted_tiles':execution['unstarted_tiles'],'pixel_states':dict(allstates),'totals':totals,'max_depth':max(r['max_depth'] for r in rows),'max_radius':max(r['max_radius'] for r in rows),'mean_midpoint_radius':widthsum/(6*len(rows)*64*den),'unknown_area_pixels':unknownsum/4**16,'old_prediction':pred,'measured_tile_cpu_to_prediction':calibration,'worker_cpu_to_prediction':execution['cpu_seconds']/pred['coarse_cpu_seconds_central'] if complete else None,'coarse_32_original_hours':original/3600,'coarse_32_replace_only_heldout_hours':replacement/3600 if replacement else None,'coarse_32_transfer_relief_ratio_hours':relieftransfer/3600 if relieftransfer else None,'extrapolation_is_not_guarantee':True,'density_groups':groups,'checkpoint_status':inflight_status,'all_exact_bounds_and_hashes_validated':True,'analysis_cpu_seconds':time.process_time()-start,'analysis_peak_rss_kib':resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,'top_cpu_tiles':sorted(rows,key=lambda r:r['cpu'],reverse=True)[:12]}
 (root/'analysis.json').write_text(json.dumps(summary,indent=2));(root/'precision-failures.json').write_text(json.dumps(failures,indent=2));(root/'tile-summary.json').write_text(json.dumps(rows,separators=(',',':')))
 with (root/'tile-summary.csv').open('w') as f:
  w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
 print(json.dumps({k:v for k,v in summary.items() if k not in ['top_cpu_tiles','density_groups']},indent=2))
if __name__=='__main__':main(Path(sys.argv[1]),Path(sys.argv[2]))
