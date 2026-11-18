#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Keep missing terminal telemetry distinct from measured resource observations."""
import json,time,sys
from pathlib import Path
p=Path(sys.argv[1]);a=json.loads((p/'analysis.json').read_text());i=json.loads((p/'interruption1/incident.json').read_text());ex=json.loads((p/'execution.json').read_text());sup=json.loads((p/'supervision.json').read_text());prep=json.loads((p/'preparation-resources.json').read_text());partial=json.loads((p/'interruption1/inflight.json').read_text());setup=json.loads((p/'setup.json').read_text());rows=json.loads((p/'tile-summary.json').read_text());b=json.loads((p/'budget-start.json').read_text())
phase1_work_lower=sum(r['cpu'] for r in rows if r['index']<1326)+sum(q['cpu_seconds'] for q in partial['pixels'])+setup['cpu_seconds']
postprocess=sum(json.loads((p/name).read_text()).get(key,0) for name,key in [('analysis.json','analysis_cpu_seconds'),('geometry-descriptors.json','cpu_seconds'),('cost-diagnostics.json','cpu_seconds'),('recovery-controls.json','cpu_seconds'),('conditional-projection.json','cpu_seconds')])
known=phase1_work_lower+sup['child_cpu_seconds']+sum(r['cpu'] for r in prep)+sup['supervisor_cpu_seconds']+postprocess
upper=i['phase1_cpu_upper_bound_seconds']+sup['child_cpu_seconds']+300
elapsed=time.monotonic()-b['start_monotonic']
result={'phase1_terminal_rusage_available':False,'phase1_worker_cpu_lower_bound_from_retained_work':phase1_work_lower,'phase1_worker_cpu_conservative_upper_bound':i['phase1_cpu_upper_bound_seconds'],'continuation_worker_cpu_rusage':sup['child_cpu_seconds'],'continuation_supervisor_cpu':sup['supervisor_cpu_seconds'],'preparation_control_child_cpu':sum(r['cpu'] for r in prep),'recorded_postprocessing_cpu':postprocess,'known_cpu_component_sum_lower_bound':known,'total_cpu_upper_bound_with_300s_preparation_reporting_allowance':upper,'owner_cpu_ceiling':1800,'worker_cpu_upper_bound':[phase1_work_lower+sup['child_cpu_seconds'],i['phase1_cpu_upper_bound_seconds']+sup['child_cpu_seconds']],'elapsed_since_budget_start':elapsed,'conservative_elapsed_with_330s_initial_reporting_allowance':elapsed+330,'owner_elapsed_ceiling':2700,'observed_max_completed_tile_rss_kib':max(r['rss_kib'] for r in rows),'continuation_peak_rss_kib':ex['peak_rss_kib'],'phase1_interrupted_peak_rss_unrecorded':True,'worker_and_supervisor_address_space_limit_bytes':384*1024**2,'current_evidence_bytes':sum(f.stat().st_size for f in p.rglob('*') if f.is_file()),'storage_ceiling_bytes':1024**3,'scope':'resource observations of reference computation, not renderer benchmarking'}
assert upper<1800 and elapsed+330<2700
(p/'resource-accounting.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
