#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,os,sys,time,resource,subprocess,signal
from pathlib import Path
from accounting import Ledger,atomic

def supervise(out,command,cap=209,inject=None):
 out=Path(out);l=Ledger(out/'ledger.json');l.reserve('worker-tail',cap)
 before=resource.getrusage(resource.RUSAGE_CHILDREN);child=None;error=None;result=None;start=time.monotonic();cpu=time.process_time()
 def limits():
  resource.setrlimit(resource.RLIMIT_AS,(384*1024**2,)*2);resource.setrlimit(resource.RLIMIT_CPU,(cap-2,cap));os.setsid()
 try:
  with (out/'worker.log').open('ab') as log,(out/'worker.stderr').open('ab') as err:
   child=subprocess.Popen(command,stdout=log,stderr=err,preexec_fn=limits)
   resource.setrlimit(resource.RLIMIT_CPU,(9,10))
   l.update(lambda d:d['entries']['worker-tail'].update(pid=child.pid,state='running'))
   while child.poll() is None:
    if inject=='supervisor_exception':raise RuntimeError('injected supervisor exception')
    if inject=='worker_termination':os.killpg(child.pid,signal.SIGKILL);inject=None
    if time.monotonic()-start>270:raise RuntimeError('worker elapsed sublimit')
    if (out/'start.json').exists():
     b=json.loads((out/'start.json').read_text())
     if time.monotonic()-b['monotonic']>500:raise RuntimeError('aggregate elapsed guard')
    # Live telemetry is informational; durable210s reservation remains conservative.
    try:
     stat=Path(f'/proc/{child.pid}/stat').read_text().split();live=(int(stat[13])+int(stat[14]))/os.sysconf('SC_CLK_TCK')
     l.update(lambda d:d['entries']['worker-tail'].update(live_cpu=live))
    except (FileNotFoundError,ProcessLookupError):pass
    time.sleep(.1)
 except BaseException as e:error=type(e).__name__+': '+str(e)
 finally:
  if child is not None:
   if child.poll() is None:
    try:os.killpg(child.pid,signal.SIGALRM)
    except ProcessLookupError:pass
    try:child.wait(timeout=2)
    except subprocess.TimeoutExpired:os.killpg(child.pid,signal.SIGKILL);child.wait()
   child.wait();after=resource.getrusage(resource.RUSAGE_CHILDREN);used=after.ru_utime+after.ru_stime-before.ru_utime-before.ru_stime
   # If output collection fails, the child has ALREADY been reaped and charged.
   l.measured('worker-tail',used,settle=True)
   result={'returncode':child.returncode,'error':error,'worker_cpu':used,'supervisor_cpu':time.process_time()-cpu,'wall':time.monotonic()-start,'child_reaped':True}
  else:result={'error':error,'child_reaped':True,'worker_not_started':True}
  l.measured('supervision',time.process_time()-cpu)
  atomic(out/'supervision.json',json.dumps(result,indent=2).encode())
 return result
if __name__=='__main__':
 resource.setrlimit(resource.RLIMIT_AS,(384*1024**2,)*2);resource.setrlimit(resource.RLIMIT_CPU,(9,220))
 out=Path(sys.argv[1]);print(supervise(out,[sys.executable,str(Path(__file__).with_name('worker.py')),str(out)]))
