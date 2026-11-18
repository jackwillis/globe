#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,subprocess,sys,time,resource,os,signal
from pathlib import Path
out=Path(sys.argv[1]);b=json.loads((out/'budget-start.json').read_text());start=time.monotonic();r0=resource.getrusage(resource.RUSAGE_CHILDREN)
with (out/'worker.log').open('w') as log,(out/'worker.stderr').open('w') as err:
 p=subprocess.Popen([sys.executable,str(Path(__file__).with_name('run.py')),str(out)],stdout=log,stderr=err,start_new_session=True)
 reason=None
 while p.poll() is None:
  if time.monotonic()-start>2100 or time.monotonic()>b['start_monotonic']+2370:reason='parent elapsed limit'
  # Includes all outputs in this experiment, not just compressed bounds.
  size=sum(f.stat().st_size for f in out.rglob('*') if f.is_file())
  if size>950*1024**2:reason='parent storage limit'
  if reason:
   os.killpg(p.pid,signal.SIGALRM)
   try:p.wait(timeout=3)
   except subprocess.TimeoutExpired:os.killpg(p.pid,signal.SIGKILL);p.wait()
   break
  time.sleep(.5)
 r1=resource.getrusage(resource.RUSAGE_CHILDREN)
(out/'supervision.json').write_text(json.dumps({'returncode':p.returncode,'parent_stop':reason,'wall_seconds':time.monotonic()-start,'child_cpu_seconds':r1.ru_utime+r1.ru_stime-r0.ru_utime-r0.ru_stime,'supervisor_cpu_seconds':time.process_time(),'stored_bytes':sum(f.stat().st_size for f in out.rglob('*') if f.is_file())},indent=2))
