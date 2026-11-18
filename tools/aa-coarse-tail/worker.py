#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import sys,json,time,signal,resource,hashlib,os
from pathlib import Path
from accounting import atomic
from admission import commit
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-enclosure-full-coarse'))
from retained import Retained,read_geometry
out=Path(sys.argv[1]).resolve();(out/'tiles').mkdir(exist_ok=True);start=time.monotonic();cpu=time.process_time();progress=[];finished=4138;current=None
# Existing numerical code and quotas unchanged. Hard process CPU210s set before exec.
def abort(sig,frame):raise RuntimeError('resource signal '+str(sig))
signal.signal(signal.SIGXCPU,abort);signal.signal(signal.SIGALRM,abort);signal.alarm(265)
root=Path('/tmp/pe-aa-phase3b-fixtures');gp=root/'view5-r220-relief.geometry';bp=root/'view5-r220-relief-background.rgb565';opened=set()
def audit(event,args):
 if event=='open' and isinstance(args[0],(str,bytes)):
  path=Path(os.fsdecode(args[0]));mode=args[1];flags=args[2];writing=(isinstance(mode,str) and any(c in mode for c in 'wax+')) or(isinstance(flags,int) and bool(flags&(os.O_WRONLY|os.O_RDWR)))
  if path.is_relative_to(out):return
  if str(path) not in {str(gp),str(bp)} or writing:raise PermissionError('candidate-blind read rejected '+str(path))
  opened.add(str(path))
sys.addaudithook(audit);status='unknown';error=None
try:
 t=time.process_time();raw=gp.read_bytes();data=bp.read_bytes();a=Retained(read_geometry(gp),data)
 atomic(out/'setup.json',json.dumps({'cpu':time.process_time()-t,'input_sha256':{gp.name:hashlib.sha256(raw).hexdigest(),bp.name:hashlib.sha256(data).hexdigest()}}).encode())
 records=[]
 for i in range(4138,4800):
  if time.process_time()>=205 or time.monotonic()-start>255:raise RuntimeError('clean worker sublimit')
  current=i;progress=[]
  def checkpoint(p):
   progress.append(p);atomic(out/'inflight.json',json.dumps({'tile_index':i,'complete':False,'pixels':progress},separators=(',',':')).encode())
  r=a.tile(i%80,i//80,checkpoint)
  for p in r['pixels']:
   d=p['denominator'];p['quantized_codes']=[[(2*m*l+d)//(2*d),(2*m*h+d)//(2*d)] for l,h,m in zip(p['lower_numerator'],p['upper_numerator'],[31,63,31])]
  r.update(tile_index=i,complete=True,peak_rss_kib=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)
  rec=commit(out,r);records.append(rec)
  atomic(out/'completed.json',json.dumps(records,separators=(',',':')).encode())
  finished=i+1;current=None
  if finished%80==0:print(json.dumps({'completed':finished,'cpu':time.process_time()-cpu}),flush=True)
 status='complete'
except BaseException as e:status='stopped';error=type(e).__name__+': '+str(e)
finally:
 signal.alarm(0);atomic(out/'execution.json',json.dumps({'status':status,'error':error,'prefix_count':4138,'final_completed':finished,'interrupted_tile':current,'interrupted_pixels':len(progress) if current is not None else 0,'unstarted':4800-finished-int(current is not None),'cpu':time.process_time()-cpu,'wall':time.monotonic()-start,'peak_rss_kib':resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,'opened_inputs':sorted(opened),'candidate_images_opened':False},indent=2).encode())
