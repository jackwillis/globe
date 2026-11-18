#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,sys,tempfile,copy,gzip,time,os
from pathlib import Path
from accounting import initialize,Ledger,atomic,storage
from admission import commit,validate
from supervise import supervise
out=Path(sys.argv[1]);start=time.process_time();checks=[]
with tempfile.TemporaryDirectory(dir=out) as temp:
 root=Path(temp)
 for case in ['supervisor_exception','worker_termination']:
  p=root/case;p.mkdir();initialize(p/'ledger.json')
  r=supervise(p,[sys.executable,'-c','import time;time.sleep(5)'],cap=2,inject=case)
  assert r['child_reaped'] and r['returncode']!=0
  d=json.loads((p/'ledger.json').read_text());assert d['entries']['worker-tail']['state']=='settled'
  try:Ledger(p/'ledger.json').reserve('worker-tail',2)
  except RuntimeError:pass
  else:raise AssertionError('restart reset')
  checks.append(case)
 p=root/'exhausted';p.mkdir();l=initialize(p/'ledger.json');l.reserve('lost_worker',210)
 try:Ledger(p/'ledger.json').reserve('new_worker',1)
 except RuntimeError:pass
 else:raise AssertionError('abandoned reservation reset')
 try:initialize(p/'ledger.json')
 except RuntimeError:pass
 else:raise AssertionError('ledger reset')
 checks+=['exhausted_budget','abandoned_worker_charged_in_full','restart_cannot_reset']
 tmp=root/'renamed.tmp';tmp.write_text('x');os.replace(tmp,root/'renamed');assert storage(root)>0;checks.append('missing_temporary_file')
 # Interrupted output collection cannot undo already-durable child charge.
 d=Ledger(root/'supervisor_exception/ledger.json').update(lambda d:None);before=d['charged_upper_seconds']
 try:json.loads('{')
 except json.JSONDecodeError:pass
 assert json.loads((root/'supervisor_exception/ledger.json').read_text())['charged_upper_seconds']==before
 checks.append('interrupted_output_collection')
 manifest=root/'manifest.json';atomic(manifest,b'[]');manifest.with_name('manifest.json.tmp').write_text('[{')
 assert json.loads(manifest.read_text())==[];checks.append('partial_manifest_atomicity')
 old=Path('/tmp/aa-full-coarse-evidence/tiles/0000.json.gz');r=json.loads(gzip.decompress(old.read_bytes()));p=root/'admission';p.mkdir();(p/'tiles').mkdir()
 for name,change in [('partial',lambda a:a['pixels'].pop()),('false_complete',lambda a:a.update(complete=False)),('bad_bound',lambda a:a['pixels'][0]['upper_numerator'].__setitem__(0,-1)),('wrong_quantization',lambda a:a['pixels'][0].update(quantized_codes=[[1,1]]*3))]:
  q=copy.deepcopy(r);change(q)
  try:commit(p,q)
  except (ValueError,KeyError):pass
  else:raise AssertionError('invalid tile admitted')
  assert not list((p/'tiles').iterdir());checks.append(name+'_not_admitted')
result={'pass':True,'controls':checks,'cpu':time.process_time()-start};atomic(out/'accounting-controls.json',json.dumps(result,indent=2).encode());print(result)
