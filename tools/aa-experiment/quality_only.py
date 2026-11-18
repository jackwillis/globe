#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Future authorized quality-only scoring of frozen outputs; no timers or rendering.
References must already have converged at the owner-approved decision margin.
"""
import argparse,json,math
from pathlib import Path
import numpy as np
from oracle_rows import primitives,bounds,expand
from reference import read_geometry
from statistics_plan import bootstrap_interval,quality_statistic


def neighborhood(features,w,h):
    # Pixel centers at Euclidean distance <=3 from the continuous rectangle/disc union.
    mask=np.zeros((h,w),dtype=bool)
    for primitive in primitives(features):
        x0,y0,x1,y1=bounds(primitive)
        lx=max(0,math.floor(x0-3));hx=min(w,math.ceil(x1+3))
        ly=max(0,math.floor(y0-3));hy=min(h,math.ceil(y1+3))
        if lx>=hx or ly>=hy:continue
        y,x=np.mgrid[ly:hy,lx:hx];x=x+.5;y=y+.5
        _,_,kind,data=primitive
        if kind=='disc':
            hit=(x-data[0])**2+(y-data[1])**2<=3.5**2
        else:
            ds=np.full(x.shape,np.inf);positive=np.ones(x.shape,dtype=bool);negative=positive.copy()
            for a,b in zip(data,data[1:]+data[:1]):
                dx,dy=b[0]-a[0],b[1]-a[1]
                cross=dx*(y-a[1])-dy*(x-a[0])
                positive&=cross>=0;negative&=cross<=0
                l2=dx*dx+dy*dy
                if l2==0:continue
                t=np.clip(((x-a[0])*dx+(y-a[1])*dy)/l2,0,1)
                ds=np.minimum(ds,(x-a[0]-t*dx)**2+(y-a[1]-t*dy)**2)
            hit=positive|negative|(ds<=9)
        mask[ly:hy,lx:hx]|=hit
    return mask


def score(candidate,reference,background,mask):
    error=np.abs(expand(candidate)/255.-reference)
    return dict(mae=float(error[mask].mean()) if mask.any() else 0,
                max_error=float(error.max()),full_frame_mae=float(error.mean()),
                outside_mask_changes=int(((candidate!=background)&~mask).sum()))


def self_test():
    f=[(0,0xffff,[([(3.5,4.5),(8.5,4.5)],False)])]
    m=neighborhood(f,12,10);bg=np.zeros((10,12),dtype=np.uint16)
    assert m[4,0] and m[4,11] and not m[0,4]
    ideal=bg.copy();ideal[4,4:8]=0xffff;ref=expand(ideal)/255.
    assert score(ideal,ref,bg,m)['mae']==0
    assert score(bg,ref,bg,m)['mae']>0
    shifted=np.roll(ideal,2,axis=0)
    assert score(shifted,ref,bg,m)['mae']>0
    assert score(ideal,ref,bg,m)==score(ideal.copy(),ref,bg,m)
    print('PASS geometry-only three-pixel union-neighborhood, identical/erased/displaced controls')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('fixtures','references','output'):p.add_argument(name,type=Path)
    p.add_argument('--decision-margin',type=float,required=True)
    a=p.parse_args()
    if not math.isfinite(a.decision_margin) or a.decision_margin<=0:raise ValueError('positive finite owner-approved margin required')
    rows=[];errors=[];seqerrors=[]
    for view in range(8):
        for radius in (220,440):
            ge=[];gs=[]
            for style in ('map','relief'):
                stem=f'view{view}-r{radius}-{style}';path=a.references/stem
                ref=np.load(path/'continuous.npy');h,w=ref.shape[:2]
                log=json.loads((path/'convergence.json').read_text())
                last={tuple(r['tile']):r for r in log}
                limit=min(1/1024,.05*a.decision_margin)
                if not log or any(r['delta']>limit or r['limit']>limit for r in last.values()):
                    raise ValueError('reference not certified at required margin: '+stem)
                read=lambda suffix:np.fromfile(a.fixtures/(stem+suffix),dtype='<u2').reshape(h,w)
                bg=read('-background.rgb565');mask=neighborhood(read_geometry(a.fixtures/(stem+'.geometry')),w,h)
                ar=read('-A.rgb565');br=read('-B.rgb565')
                qr=np.fromfile(path/'quantized.rgb565',dtype='<u2').reshape(h,w)
                sr=np.fromfile(path/'sequential.rgb565',dtype='<u2').reshape(h,w)
                sa,sb,sq,ss=[score(c,ref,bg,mask) for c in (ar,br,qr,sr)]
                rows.append(dict(fixture=stem,mask_pixels=int(mask.sum()),A=sa,B=sb,quantized_control=sq,sequential_control=ss))
                ge.append([sa['mae'],sb['mae'],sq['mae']]);gs.append([sa['mae'],sb['mae'],ss['mae']])
            errors.append(ge);seqerrors.append(gs)
    points=lambda x:[float(v) if np.isfinite(v) else None for v in quality_statistic(np.asarray(x))]
    result=dict(rows=rows,original_gap_closed=points(errors),sequential_gap_closed=points(seqerrors),original_floor=bootstrap_interval(errors,quality_statistic),
                sequential_control=bootstrap_interval(seqerrors,quality_statistic),
                scope='primary quality and controls only; native review and isolated-feature diagnostics remain separately required',
                decision_margin=a.decision_margin)
    # No automatic adoption verdict; margins supporting a later verdict must be rechecked.
    a.output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')

if __name__=='__main__':
    import sys
    if sys.argv[1:]==['--self-test']:self_test()
    else:main()
