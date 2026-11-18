#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json, math, subprocess, sys
from pathlib import Path
import numpy as np
from reference import reference_pixel, self_test
from oracle_rows import Oracle, expand


def cases():
    for k in range(8):
        a=(k+.5)*math.pi/4
        for length in (.25,.5,1,3,16):
            yield f'octant{k}-length{length}',[(20.25,20.5),(20.25+length*math.cos(a),20.5+length*math.sin(a))],False
    for deg in (15,45,90,150):
        a=math.radians(deg)
        yield f'join{deg}',[(12.25,20.5),(20.25,20.5),(20.25+8*math.cos(a),20.5+8*math.sin(a))],False
    yield 'closed-square',[(12.25,12.5),(24.25,12.5),(24.25,24.5),(12.25,24.5)],True
    yield 'closed-ring',[(24+10*math.cos(i*math.pi/8),24+10*math.sin(i*math.pi/8)) for i in range(16)],True
    for chord in (.25,.5,1,3):
        radius=12; step=2*math.asin(chord/(2*radius))
        yield f'arc20-chord{chord}',[(24+radius*math.cos(-1+i*step),24+radius*math.sin(-1+i*step)) for i in range(21)],False


def controls():
    self_test()
    line=([(3.5,10.5),(19.5,10.5)],False)
    features=[(0,0xf800,[line]),(1,0x001f,[line])]
    o=Oracle(features,24,16); bg=np.zeros((16,24),dtype=np.uint16)
    rgb,q,f=o.render(bg,64)
    assert np.array_equal(rgb[10,3],[0,0,.5]) and q[10,3]==0x10
    assert f[10,3]!=q[10,3] # averaged feature compositing differs intentionally
    # Bitwise equivalence against slow, separately implemented membership at 64x64.
    complex_features=[(0,0xf800,[([(4.2,4.1),(8.4,7.7),(5.3,9.2)],False)]),
                      (1,0x07e0,[([(5,4),(9,8)],False)])]
    o=Oracle(complex_features,16,16)
    bg=np.arange(256,dtype=np.uint16).reshape(16,16)
    rgb,q,f=o.render(bg,64)
    for x,y in [(4,4),(6,6),(8,7),(5,8),(0,0),(9,9)]:
        a,b=reference_pixel(complex_features,int(bg[y,x]),x,y,64)
        assert np.array_equal(rgb[y,x],a) and q[y,x]==b,(x,y,rgb[y,x],a)
    # Empty/offscreen preserve exact original packed colors.
    for ff in ([],[(0,0xffff,[([(-20,-20),(-10,-10)],False)])]):
        _,q,f=Oracle(ff,16,16).render(bg)
        assert np.array_equal(q,bg) and np.array_equal(f,bg)
    for bad in [float('nan'),float('inf'),1e7]:
        try: Oracle([(0,0xffff,[([(bad,0),(1,1)],False)])],16,16)
        except ValueError: pass
        else: raise AssertionError('invalid coordinate')
    print('PASS row oracle: independent membership equality, colored order, sequential floor, empty/offscreen, invalid bounds',flush=True)


def main():
    controls()
    output=Path(sys.argv[2]); output.mkdir(parents=True,exist_ok=True)
    frozen=[dict(name=n,points=p,closed=c) for n,p,c in cases()]
    (output/'synthetic-geometry.json').write_text(json.dumps(frozen,indent=2)+'\n')
    rows=[]; logs=[]
    for case in frozen:
        name,p,closed=case['name'],case['points'],case['closed']
        text=f'{len(p)} {int(closed)}\n'+''.join(f'{x:.17g} {y:.17g}\n' for x,y in p)
        raw=subprocess.run([sys.argv[1]],input=text.encode(),stdout=subprocess.PIPE,check=True).stdout
        candidate=expand(np.frombuffer(raw,dtype='<u2').reshape(48,48))/255.
        oracle=Oracle([(0,0xffff,[(p,closed)])],48,48)
        bg=np.zeros((48,48),dtype=np.uint16)
        ref,q,f,log=oracle.converged(bg)
        support=ref.max(axis=2)>0
        error=np.abs(candidate-ref)
        quant_error=np.abs(candidate-expand(q)/255.)
        floor_error=np.abs(candidate-expand(f)/255.)
        rows.append(dict(name=name, support_pixels=int(support.sum()),
                         support_mae=float(error[support].mean()),max_error=float(error.max()),
                         full_mae=float(error.mean()),quant_mae=float(quant_error[support].mean()),
                         sequential_mae=float(floor_error[support].mean()),
                         ink_ratio=float(candidate.sum()/ref.sum()),
                         missing_quarter=int(((ref.max(axis=2)>=.25)&(candidate.max(axis=2)==0)).sum()),
                         max_convergence_delta=max(next(r['delta'] for r in reversed(log) if r['tile']==list(tile)) for tile in oracle.bins) if log else 0,
                         max_samples=max(r['samples'] for r in log)))
        logs.append(dict(name=name,refinement=log))
        print(json.dumps(rows[-1]),flush=True)
    (output/'synthetic-results.json').write_text(json.dumps(rows,indent=2)+'\n')
    (output/'synthetic-convergence.json').write_text(json.dumps(logs,indent=2)+'\n')

if __name__=='__main__': main()
