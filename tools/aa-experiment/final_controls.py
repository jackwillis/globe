#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Analytical controls and full-frame independent sample-membership parity, no geography."""
import json, math
import numpy as np
from oracle_rows import Oracle, interval, quantize
from reference import reference_pixel


def run():
    bg=np.zeros((8,8),dtype=np.uint16)
    results={}
    for name,p in [('horizontal',[(1.5,3.5),(5.5,3.5)]),('vertical',[(3.5,1.5),(3.5,5.5)])]:
        f=[(0,0xffff,[(p,False)])];rgb,q,_=Oracle(f,8,8).render(bg,64)
        expected=np.zeros((8,8))
        if name=='horizontal': expected[3,1:6]=[.5,1,1,1,.5]
        else: expected[1:6,3]=[.5,1,1,1,.5]
        assert np.array_equal(rgb,np.repeat(expected[:,:,None],3,axis=2))
        assert np.array_equal(q,quantize(rgb))
        assert (q[3,1] if name=='horizontal' else q[1,3])==0x8410
        assert (q[3,0] if name=='horizontal' else q[0,3])==0
        rev=Oracle([(0,0xffff,[(p[::-1],False)])],8,8).render(bg,64)
        assert np.array_equal(rgb,rev[0]) and np.array_equal(q,rev[1])
        results[name]='exact rectangle area, butt endpoints, adjacent empty pixels, reversal, packed half=0x8410'
    for p in [[(0,0.5),(4,0.5)],[(-2,0.5),(2,0.5)],[(0,0),(4,0)],[(0,8),(4,8)],[(2,2),(2,2)]]:
        f=[(0,0xffff,[(p,False)])];rgb,q,_=Oracle(f,8,8).render(bg,64)
        for y in range(8):
            for x in range(8):
                slow,packed=reference_pixel(f,0,x,y,64)
                assert np.array_equal(rgb[y,x],slow) and q[y,x]==packed
    results['boundaries']='exact slow/row equality: viewport edge, clipped butt, half-clipped width, tangent outside, degenerate'
    # Test circle row primitive directly, independently of chain primitive construction.
    o=Oracle([(0,0xffff,[])],4,4)
    o.primitives=[(0,0xffff,'disc',(2.,2.))];o.bins={(0,0):[0]}
    rgb,q,_=o.render(np.zeros((4,4),dtype=np.uint16),256)
    area=float(rgb[:,:,0].sum())
    assert abs(area-math.pi/4)<=1/1024
    assert np.array_equal(rgb,np.flip(rgb,axis=0)) and np.array_equal(rgb,np.flip(rgb,axis=1))
    assert interval(o.primitives[0],2.5)==(2.,2.)
    assert interval(o.primitives[0],math.nextafter(2.5,math.inf)) is None
    assert interval(o.primitives[0],2.)==(1.5,2.5)
    results['disc']=dict(area=area,analytic=math.pi/4,absolute_error=abs(area-math.pi/4),symmetry='exact',tangent='exact')
    # Complete frames: random packed backgrounds; no candidate output is used.
    rng=np.random.default_rng(20261118)
    frames=[[(0,0xf800,[([(1.2,1.3),(7.1,4.7),(4.3,7.2)],False)]),
             (1,0x07e0,[([(2.1,6.8),(7.3,1.4)],False)])],
            [(0,0xffff,[([(4+2.1*math.cos(i*.9),4+2.3*math.sin(i*.9)) for i in range(7)],True)]),
             (1,0x001f,[([(0.7,2.2),(3.3,2.2),(3.3,2.2),(7.2,6.1)],False)]),
             (2,0xf800,[([(1.5,5.5),(6.5,5.5)],False)])]]
    comparisons=[]
    for i,features in enumerate(frames):
        background=rng.integers(0,65536,size=(8,8),dtype=np.uint16)
        rgb,q,_=Oracle(features,8,8).render(background,64)
        maximum=0.;packed_mismatches=0;different=0
        for y in range(8):
            for x in range(8):
                slow,packed=reference_pixel(features,int(background[y,x]),x,y,64)
                d=float(np.max(np.abs(rgb[y,x]-slow)))
                maximum=max(maximum,d);different+=bool(d);packed_mismatches+=bool(q[y,x]!=packed)
        # One sample's maximal color contribution is the explicit boundary-tie tolerance.
        assert maximum<=1/(64*64) and packed_mismatches==0
        comparisons.append(dict(frame=i,pixels=64,max_rgb_delta=maximum,differing_pixels=different,packed_mismatches=packed_mismatches))
    results['whole_frames']=comparisons
    results['tie_tolerance']='continuous <=1/4096 normalized per channel at 64x64; quantized RGB565 must be exact. Axis-aligned analytic/tangent controls have no tolerance.'
    for invalid in (0,-1,float('nan'),float('inf')):
        try: Oracle([],8,8).converged(bg,invalid)
        except ValueError: pass
        else: raise AssertionError('invalid margin accepted')
    # A permanently unstable tile must fail after the fixed 1024 ceiling.
    unstable=Oracle([(0,0xffff,[([(1,1),(2,1)],False)])],8,8)
    def unstable_tile(tx,ty,background,n):
        return np.full((8,8,3),math.log2(n)/10),bg,bg
    unstable.tile_reference=unstable_tile
    try: unstable.converged(bg)
    except RuntimeError as error: assert 'inconclusive' in str(error)
    else: raise AssertionError('unconverged reference accepted')
    results['failure_handling']='nonfinite/nonpositive margins rejected; forced nonconvergence returns inconclusive at 1024'
    print(json.dumps(results,indent=2))
    return results

if __name__=='__main__': run()
