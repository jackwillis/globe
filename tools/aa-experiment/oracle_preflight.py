#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""One candidate-blind real reference preflight; no candidate imports/outputs or quality metrics."""
import hashlib,json,os,resource,signal,sys,time
from pathlib import Path
import numpy as np
from oracle_rows import Oracle
from reference import read_geometry


def main():
    root,out=map(Path,sys.argv[1:])
    geometry=root/'view0-r220-map.geometry'
    background=root/'view0-r220-map-background.rgb565'
    # Preregistered operational bounds; resource accounting is not renderer benchmarking.
    resource.setrlimit(resource.RLIMIT_AS,(512*1024*1024,512*1024*1024))
    def deadline(signum,frame):
        raise RuntimeError('inconclusive preflight resource deadline')
    signal.signal(signal.SIGALRM,deadline)
    signal.signal(signal.SIGXCPU,deadline)
    resource.setrlimit(resource.RLIMIT_CPU,(295,300))
    signal.alarm(360)
    opened=[]
    allowed={str(geometry.resolve()),str(background.resolve())}
    def audit(event,args):
        if event=='open' and isinstance(args[0],(str,bytes)):
            path=os.fsdecode(args[0])
            if str(Path(path).resolve()) not in allowed:
                raise PermissionError('preflight forbidden input '+path)
            opened.append(path)
    # Preload a failure-handling test before the strict two-input audit.
    try: Oracle([],640,481)
    except ValueError: pass
    else: raise AssertionError('dimension resource limit not enforced')
    try: Oracle([(0,0xffff,[([(float('nan'),0),(1,1)],False)])],8,8)
    except ValueError: pass
    else: raise AssertionError('invalid geometry not rejected')
    sys.addaudithook(audit)
    start=time.monotonic()
    hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (geometry,background)}
    features=read_geometry(geometry)
    bg=np.frombuffer(background.read_bytes(),dtype='<u2').reshape(480,640)
    o=Oracle(features,640,480)
    original=o.tile_reference; calls=0
    def progress(*args):
        nonlocal calls
        value=original(*args);calls+=1
        if calls%128==0:
            print(json.dumps(dict(reference_tile_calls=calls)),file=sys.stderr,flush=True)
        return value
    o.tile_reference=progress
    # No positive decision margin is invented: tolerance-only diagnostic, never scoring admission.
    rgb,q,f,log=o.converged(bg,margin=None)
    last={tuple(r['tile']):r for r in log}
    usage=resource.getrusage(resource.RUSAGE_SELF)
    result=dict(fixture=geometry.stem,inputs_sha256=hashes,opened_inputs=sorted(set(opened)),
                candidate_outputs_opened=False,geographic_quality_scores=False,
                tolerance_only=True,decision_margin=None,limit=1/1024,status='converged',
                affected_tiles=len(o.bins),index_entries=o.entries,primitives=len(o.primitives),
                max_final_delta=max(r['delta'] for r in last.values()),max_samples=max(r['samples'] for r in log),
                wall_seconds=time.monotonic()-start,user_seconds=usage.ru_utime,system_seconds=usage.ru_stime,
                peak_rss_kib=usage.ru_maxrss,virtual_memory_cap_bytes=512*1024*1024,cpu_soft_cap_seconds=295,cpu_cap_seconds=300,wall_cap_seconds=360,
                failure_controls='invalid dimensions and NaN rejected before output',refinement=log)
    # Audit forbids writing files as well; emit JSON on inherited stdout only.
    print(json.dumps(result,indent=2),flush=True)

if __name__=='__main__':
    try: main()
    except (ValueError,RuntimeError,MemoryError,OSError) as e:
        print(json.dumps(dict(status='inconclusive_preflight_failure',reason=str(e))),flush=True)
        raise SystemExit(1)
