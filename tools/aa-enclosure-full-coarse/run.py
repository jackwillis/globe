#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""One frozen fixture, one retained index, atomic tile certificates; no scoring."""
import json,sys,time,resource,signal,os,hashlib,gzip
from pathlib import Path
from retained import Retained,read_geometry
out=Path(sys.argv[1]).resolve();out.mkdir(exist_ok=True);(out/'tiles').mkdir(exist_ok=True)
start=time.monotonic();cpu=time.process_time();stored=0;finished=0;current=None;progress=[]
def atomic(path,raw):
 global stored
 if stored+len(raw)>900*1024**2:raise RuntimeError('storage budget stop')
 temp=path.with_suffix(path.suffix+'.tmp')
 with temp.open('wb') as f:f.write(raw);f.flush();os.fsync(f.fileno())
 os.replace(temp,path)
def abort(sig,frame):raise RuntimeError('resource signal '+str(sig))
signal.signal(signal.SIGXCPU,abort);signal.signal(signal.SIGALRM,abort)
resource.setrlimit(resource.RLIMIT_AS,(384*1024**2,)*2);resource.setrlimit(resource.RLIMIT_CPU,(1445,1450));signal.alarm(2090)
root=Path('/tmp/pe-aa-phase3b-fixtures');gp=root/'view5-r220-relief.geometry';bp=root/'view5-r220-relief-background.rgb565';allowed={str(gp),str(bp)};opened=set()
def audit(event,args):
 if event=='open' and isinstance(args[0],(str,bytes)):
  p=os.fsdecode(args[0]);mode=args[1];flags=args[2];writing=(isinstance(mode,str) and any(c in mode for c in 'wax+')) or (isinstance(flags,int) and bool(flags&(os.O_WRONLY|os.O_RDWR)))
  if Path(p).is_relative_to(out):return
  if p not in allowed:raise PermissionError('candidate-blind input rejected '+p)
  if writing:raise PermissionError('read-only input')
  opened.add(p)
sys.addaudithook(audit)
status='unknown';error=None
try:
 t=time.process_time();w=time.monotonic();raw=gp.read_bytes();ff=read_geometry(gp);data=bp.read_bytes();a=Retained(ff,data)
 setup={'cpu_seconds':time.process_time()-t,'wall_seconds':time.monotonic()-w,'primitive_count':sum(len(p) for _,_,p in a.geometry.features),'input_sha256':{gp.name:hashlib.sha256(raw).hexdigest(),bp.name:hashlib.sha256(data).hexdigest()}}
 atomic(out/'setup.json',json.dumps(setup).encode())
 for idx in range(4800):
  # 5 CPU / 10 wall seconds reserved for clean exit. Per-pixel limits unchanged.
  if time.process_time()-cpu>=1440 or time.monotonic()-start>=2080:raise RuntimeError('clean resource budget stop')
  current=idx;progress=[];tx,ty=idx%80,idx//80
  def checkpoint(p):
   progress.append(p)
   atomic(out/'inflight.json',json.dumps({'tile_index':idx,'tile':[tx,ty],'complete':False,'pixels':progress},separators=(',',':')).encode())
  r=a.tile(tx,ty,checkpoint)
  for p in r['pixels']:
   den=p['denominator'];p['quantized_codes']=[[(2*m*l+den)//(2*den),(2*m*h+den)//(2*den)] for l,h,m in zip(p['lower_numerator'],p['upper_numerator'],[31,63,31])]
  r.update(tile_index=idx,complete=True,peak_rss_kib=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)
  encoded=gzip.compress(json.dumps(r,separators=(',',':')).encode(),compresslevel=1,mtime=0)
  path=out/'tiles'/f'{idx:04d}.json.gz';atomic(path,encoded);stored+=len(encoded)
  record={'index':idx,'tile':[tx,ty],'sha256':hashlib.sha256(encoded).hexdigest(),'bytes':len(encoded),'precision':r['target_attained'],'cpu':r['cpu_seconds'],'wall':r['wall_seconds'],'cells':r['cell_visits'],'predicates':r['predicate_calls'],'max_radius':r['max_radius']}
  with (out/'completed.jsonl').open('a') as f:f.write(json.dumps(record)+'\n');f.flush();os.fsync(f.fileno())
  finished+=1;current=None
  if finished%80==0:print(json.dumps({'completed':finished,'cpu':time.process_time()-cpu,'wall':time.monotonic()-start,'stored_bytes':stored}),flush=True)
 status='traversal_complete'
except BaseException as e:status='stopped';error=type(e).__name__+': '+str(e)
finally:
 signal.alarm(0)
 summary={'status':status,'error':error,'completed_tiles':finished,'interrupted_tile':current,'interrupted_pixels':len(progress) if current is not None else 0,'unstarted_tiles':4800-finished-(current is not None),'cpu_seconds':time.process_time()-cpu,'wall_seconds':time.monotonic()-start,'peak_rss_kib':resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,'compressed_tile_bytes':stored,'opened_inputs':sorted(opened),'candidate_results_opened':False}
 atomic(out/'execution.json',json.dumps(summary,indent=2).encode());print(json.dumps(summary),flush=True)
