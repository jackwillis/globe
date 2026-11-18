#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,os,fcntl,time,math
from pathlib import Path

def atomic(path,data):
 path=Path(path)
 if len(data)>4*1024**2:raise RuntimeError('per-file evidence guard')
 tmp=path.with_name(path.name+'.tmp')
 with tmp.open('wb') as f:f.write(data);f.flush();os.fsync(f.fileno())
 os.replace(tmp,path)
 fd=os.open(path.parent,os.O_RDONLY)
 try:os.fsync(fd)
 finally:os.close(fd)

def storage(root):
 total=0
 for p in Path(root).rglob('*'):
  try:
   if p.is_file():total+=p.stat().st_size
  except FileNotFoundError:pass
 return total

class Ledger:
 def __init__(self,path):self.path=Path(path)
 def update(self,fn):
  with self.path.with_suffix('.lock').open('a') as lock:
   fcntl.flock(lock,fcntl.LOCK_EX)
   d=json.loads(self.path.read_text());fn(d)
   d['charged_upper_seconds']=sum(x['charge'] for x in d['entries'].values())
   if d['charged_upper_seconds']>d['cap']:raise RuntimeError('aggregate CPU exhausted')
   d['updated_epoch']=time.time();atomic(self.path,json.dumps(d,indent=2).encode());return d
 def reserve(self,name,amount):
  def change(d):
   if name in d['entries']:raise RuntimeError('reservation already exists; restart cannot reset')
   d['entries'][name]={'charge':amount,'reserved':amount,'state':'reserved','measured_cpu':None}
  return self.update(change)
 def measured(self,name,value,settle=False):
  def change(d):
   x=d['entries'][name]
   if value>x['reserved']:raise RuntimeError('process allowance exceeded')
   x['measured_cpu']=value
   if settle:x.update(state='settled',charge=math.ceil(value*1000)/1000)
  return self.update(change)

def initialize(path):
 path=Path(path)
 if path.exists():raise RuntimeError('existing ledger must not reset')
 atomic(path,json.dumps({'cap':300,'entries':{},'elapsed_anchor':json.loads((path.parent/'start.json').read_text()) if (path.parent/'start.json').exists() else None,'original_accounting_preserved':{'worker_cpu_range':[1094.0197164909998,1326.699908742045],'total_accounting_upper_with_allowance':1626.699908742045,'missing_first_terminal_telemetry':True},'incremental_budget_only':True}).encode())
 l=Ledger(path)
 for n,v in [('preparation',50),('validation_reporting',30),('supervision',10)]:l.reserve(n,v)
 return l
