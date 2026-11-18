#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,sys,gzip,hashlib,time,resource,collections
from pathlib import Path
from accounting import Ledger,atomic,storage
# Independent frozen checker, distinct from the continuation writer's admission module.
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-enclosure-full-coarse'))
from analyze import validate
start=time.process_time();out=Path(sys.argv[1]);old=Path('/tmp/aa-full-coarse-evidence');recovery=json.loads((out/'recovery.json').read_text());prefix=[json.loads(x) for x in (old/'completed.jsonl').read_text().splitlines()];tail=json.loads((out/'completed.json').read_text()) if (out/'completed.json').exists() else []
assert len(prefix)==4138 and hashlib.sha256((old/'completed.jsonl').read_bytes()).hexdigest()==recovery['original_manifest_sha256']
rows=[];states=collections.Counter();amb=0;channels=0;unresolved=0;width=0;maxradius=0;maxdepth=0;predicates=0;cells=0;peak=0;roots=0
for i,rec in enumerate(prefix+tail):
 assert rec['index']==i
 path=(old if i<4138 else out)/'tiles'/f'{i:04d}.json.gz';raw=path.read_bytes();assert hashlib.sha256(raw).hexdigest()==rec['sha256']
 if i<4138:assert rec['sha256']==recovery['prefix_hashes'][str(i)]
 r=json.loads(gzip.decompress(raw));validate(r,i);assert len(r['pixels'])==64
 for p in r['pixels']:
  states[p['state']]+=1;amb+=any(a!=b for a,b in p['quantized_codes']);channels+=sum(a!=b for a,b in p['quantized_codes']);unresolved+=p['unresolved_area_numerator']/p['area_denominator'];width+=sum(b-a for a,b in zip(p['lower_numerator'],p['upper_numerator']));maxdepth=max(maxdepth,p['deepest'])
 maxradius=max(maxradius,r['max_radius']);predicates+=r['predicate_calls'];roots+=sum(p['initial_predicate_calls'] for p in r['pixels']);cells+=r['cell_visits'];peak=max(peak,r['peak_rss_kib']);rows.append({'index':i,'sha256':rec['sha256'],'cpu':r['cpu_seconds'],'wall':r['wall_seconds'],'precision':r['target_attained']})
assert len(tail)==len(list((out/'tiles').glob('*.json.gz'))),'orphan tile retained; cannot silently complete'
oldproj=json.loads(Path('/tmp/aa-enclosure-scale-v1-evidence/resource-projections.json').read_text());orig=1037.9953137186326;revised=1240.0079990924528;total=sum(r['cpu'] for r in rows);complete=len(rows)==4800;oldtotal=oldproj['coarse_32_cpu_hours'];replacement=oldtotal+(total-orig)/3600 if complete else None;transfer=sum(r['coarse_cpu_seconds_central']*(total/orig if r['fixture'].endswith('-relief') else 1) for r in oldproj['rows'])/3600 if complete else None
execution=json.loads((out/'execution.json').read_text());sup=json.loads((out/'supervision.json').read_text());ledger=json.loads((out/'ledger.json').read_text());historical=json.loads((old/'resource-accounting.json').read_text());elapsed=time.monotonic()-json.loads((out/'start.json').read_text())['monotonic'];validation_cpu=time.process_time()-start
assert validation_cpu<30 and elapsed+30<600
# Keep terminal measured component distinct from conservative allowances.
Ledger(out/'ledger.json').measured('validation_reporting',validation_cpu)
result={'completed_tiles':len(rows),'tail_tiles':len(tail),'pixels':len(rows)*64,'precision_attained_tiles':sum(r['precision'] for r in rows),'precision_limited_tiles':sum(not r['precision'] for r in rows),'pixel_states':dict(states),'interrupted_tile':execution['interrupted_tile'],'unstarted_tiles':execution['unstarted'],'original_prefix_hashes_unchanged':True,'original_manifest_unchanged':True,'all_hashes_and_exact_admissions_pass':True,'full_tile_cpu':total,'original_prefix_tile_cpu':sum(r['cpu'] for r in rows[:4138]),'tail_tile_cpu':sum(r['cpu'] for r in rows[4138:]),'original_model':orig,'revised_model':revised,'versus_original_percent':100*(total/orig-1) if complete else None,'versus_revised_percent':100*(total/revised-1) if complete else None,'worker_incremental_cpu':sup['worker_cpu'],'supervision_incremental_cpu':sup['supervisor_cpu'],'preparation_measured_preflight_cpu':recovery['cpu_seconds'],'validation_measured_cpu':validation_cpu,'additional_cpu_charge_upper':ledger['charged_upper_seconds'],'reserved_allowances':{'preparation':50,'reporting':30,'supervision':10,'failed_preexec_bound':1},'new_known_measured_cpu_sum':sup['worker_cpu']+sup['supervisor_cpu']+recovery['cpu_seconds']+validation_cpu,'original_worker_range':historical['worker_cpu_upper_bound'],'original_total_accounting_upper':historical['total_cpu_upper_bound_with_300s_preparation_reporting_allowance'],'additional_elapsed_since_anchor':elapsed,'additional_elapsed_with_initial_allowance':elapsed+30,'peak_recorded_fixture_rss_kib':peak,'tail_peak_rss_kib':execution['peak_rss_kib'],'combined_old_new_evidence_bytes':storage(old)+storage(out),'rgb565_ambiguous_pixels':amb,'rgb565_ambiguous_channels':channels,'unresolved_area_pixels':unresolved,'max_radius':maxradius,'mean_radius':width/(6*len(rows)*64*255*4**16),'max_depth':maxdepth,'predicates':predicates,'initial_predicates':roots,'cells':cells,'coarse32_old_hours':oldtotal,'coarse32_replace_measured_fixture_hours':replacement,'coarse32_transfer_relief_hours':transfer,'projection_is_conditional_not_guarantee':True}
atomic(out/'final-validation.json',json.dumps(result,indent=2).encode());atomic(out/'combined-manifest.json',json.dumps(rows,separators=(',',':')).encode());print(json.dumps(result,indent=2))
