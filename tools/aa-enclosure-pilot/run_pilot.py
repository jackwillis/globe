#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Exactly four candidate-blind tile jobs; resource observations, no rendering timings."""
import hashlib,json,os,resource,signal,struct,sys,time
from pathlib import Path
from enclose import Geometry,rgb565
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-experiment'))
from reference import read_geometry
JOBS=[('view4-r440-relief',None,0,32),('view0-r220-map',None,16,36),
      ('view4-r440-map',1,2,57),('view0-r220-map',2,39,43)]

def main():
    index=int(sys.argv[1]);output=Path(sys.argv[2]);stem,feature,tx,ty=JOBS[index]
    root=Path('/tmp/pe-aa-phase3b-fixtures');gp=root/(stem+'.geometry');bp=root/(stem+'-background.rgb565')
    def abort(sig,frame):raise RuntimeError('pilot process resource deadline')
    signal.signal(signal.SIGXCPU,abort);signal.signal(signal.SIGALRM,abort)
    resource.setrlimit(resource.RLIMIT_AS,(384*1024**2,384*1024**2))
    resource.setrlimit(resource.RLIMIT_CPU,(105,110));signal.alarm(120)
    allowed={str(gp),str(bp)};opened=[]
    def audit(event,args):
        if event=='open' and isinstance(args[0],(str,bytes)):
            p=os.fsdecode(args[0])
            if p not in allowed:raise PermissionError('candidate-blind audit rejected '+p)
            opened.append(p)
    sys.addaudithook(audit)
    start=time.monotonic();features=read_geometry(gp);data=bp.read_bytes()
    if len(data)!=640*480*2:raise ValueError('background size')
    if feature is not None:features=[features[feature]]
    geometry=Geometry(features,max_depth=16)
    def background(x,y):return rgb565(struct.unpack_from('<H',data,2*(y*640+x))[0])
    result=geometry.tile(tx,ty,background,max_visits=2000000,seconds=90)
    # Monotone nearest RGB565 quantization produces discrete code OUTCOME bounds.
    ambiguous=0
    for p in result['pixels']:
        den=p['denominator'];q=[]
        for lo,hi,m in zip(p['lower_numerator'],p['upper_numerator'],[31,63,31]):
            q.append([(2*m*lo+den)//(2*den),(2*m*hi+den)//(2*den)])
        p['quantized_channel_code_bounds']=q
        ambiguous+=any(a!=b for a,b in q)
    result.update(index=index,fixture=stem,isolated_feature_index=feature,input_hashes={gp.name:hashlib.sha256(gp.read_bytes()).hexdigest(),bp.name:hashlib.sha256(data).hexdigest()},
                  opened_inputs=sorted(set(opened)),candidate_images_opened=False,
                  limits=dict(address_bytes=384*1024**2,cpu_soft_seconds=105,cpu_hard_seconds=110,wall_hard_seconds=120),
                  peak_rss_kib=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
                  total_seconds=time.monotonic()-start,quantization_ambiguous_pixels=ambiguous,
                  proof_model='exact dyadic endpoints; exact width1; exact integer predicates and rational area bounds',
                  source_feature_count=len(features),primitives=sum(len(p) for _,_,p in geometry.features))
    # Audit permits only inputs; results go to inherited stdout, parent persists JSON.
    print(json.dumps(result,indent=2),flush=True)

if __name__=='__main__':
    try:main()
    except Exception as e:
        print(json.dumps(dict(status='FAILED',error=type(e).__name__+': '+str(e))),flush=True);raise SystemExit(1)
