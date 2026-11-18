#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,sys,resource,time,signal,struct,hashlib,os
from pathlib import Path
from enclose import Geometry,rgb565
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-experiment'))
from reference import read_geometry

def main():
    jobpath,outpath=map(Path,sys.argv[1:]);job=json.loads(jobpath.read_text());outpath.mkdir(parents=True,exist_ok=True)
    start=time.monotonic();cpu=time.process_time()
    def abort(sig,frame):raise RuntimeError('process resource deadline')
    signal.signal(signal.SIGXCPU,abort);signal.signal(signal.SIGALRM,abort)
    resource.setrlimit(resource.RLIMIT_AS,(384*1024**2,)*2);resource.setrlimit(resource.RLIMIT_CPU,(85,90));signal.alarm(89)
    stem=job['fixture'];root=Path('/tmp/pe-aa-phase3b-fixtures')
    gp=root/(stem+'.geometry');bp=root/(stem+'-background.rgb565')
    allowed={str(gp),str(bp)};opened=[]
    def audit(event,args):
        if event=='open' and isinstance(args[0],(str,bytes)):
            path=os.fsdecode(args[0]);mode=args[1];flags=args[2]
            writing=(isinstance(mode,str) and any(c in mode for c in 'wax+')) or (isinstance(flags,int) and bool(flags&(os.O_WRONLY|os.O_RDWR)))
            if writing and Path(path).is_relative_to(outpath):return
            if path not in allowed:raise PermissionError('candidate-blind input rejected '+path)
            opened.append(path)
    sys.addaudithook(audit)
    raw=gp.read_bytes();features=read_geometry(gp);data=bp.read_bytes()
    if job.get('feature') is not None:features=[features[job['feature']]]
    if job.get('coverage'):features=[(i,0xffff,c) for i,_,c in features]
    t=time.process_time();g=Geometry(features);setup_cpu=time.process_time()-t
    initial_cpu=time.process_time();initial_calls=g.predicate_calls
    progress=[]
    def checkpoint(pixel):
        progress.append(pixel)
        temp=outpath/'checkpoint.tmp'
        temp.write_text(json.dumps(dict(job=job,completed_pixels=len(progress),pixels=progress,complete=False)))
        temp.replace(outpath/'checkpoint.json')
    g.checkpoint=checkpoint
    def background(x,y):return (0,0,0) if job.get('coverage') else rgb565(struct.unpack_from('<H',data,2*(y*640+x))[0])
    result=g.tile(job['tx'],job['ty'],background,max_visits=6400000,seconds=75,target_den=job['target_den'])
    for p in result['pixels']:
        den=p['denominator'];p['quantized_codes']=[[(2*m*a+den)//(2*den),(2*m*b+den)//(2*den)] for a,b,m in zip(p['lower_numerator'],p['upper_numerator'],[31,63,31])]
        if job.get('coverage'):p['alpha_codes']=[(510*p['lower_numerator'][0]+den)//(2*den),(510*p['upper_numerator'][0]+den)//(2*den)]
    result.update(job=job,setup_index_cpu_seconds=setup_cpu,total_cpu_seconds=time.process_time()-cpu,total_wall_seconds=time.monotonic()-start,peak_rss_kib=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
                  initial_predicate_calls=sum(p['initial_predicate_calls'] for p in result['pixels']),
                  initial_classification_cpu_seconds=sum(p.get('initial_cpu_seconds',0) for p in result['pixels']),
                  primitive_count=sum(len(pp) for _,_,pp in g.features),input_sha256={gp.name:hashlib.sha256(raw).hexdigest(),bp.name:hashlib.sha256(data).hexdigest()},opened_inputs=sorted(set(opened)),candidate_results_opened=False)
    print(json.dumps(result),flush=True)
if __name__=='__main__':
    try:main()
    except Exception as e:print(json.dumps(dict(status='failed',error=type(e).__name__+': '+str(e))),flush=True);raise SystemExit(1)
