#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Independent analytic tests. Fraction polygon clipping is test-only, not a second geographic method."""
from fractions import Fraction as F
import json,math,time
from enclose import Geometry,rgb565


def clip_area(vertices,x,y):
    p=vertices
    for axis,bound,keep_greater in [(0,F(x),True),(0,F(x+1),False),(1,F(y),True),(1,F(y+1),False)]:
        out=[]
        for a,b in zip(p,p[1:]+p[:1]):
            ina=a[axis]>=bound if keep_greater else a[axis]<=bound
            inb=b[axis]>=bound if keep_greater else b[axis]<=bound
            if ina:out.append(a)
            if ina!=inb:
                t=(bound-a[axis])/(b[axis]-a[axis]);out.append(tuple(a[i]+t*(b[i]-a[i]) for i in (0,1)))
        p=out
        if not p:return F(0)
    return abs(sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(p,p[1:]+p[:1])))/2


def pixel_bound(features,x,y):
    g=Geometry(features);stats=dict(cell_visits=1,max_heap_cells=0)
    p=g.pixel(x,y,(0,0,0),stats,time.monotonic()+20,400000)
    assert p['state']=='precision_reached' and max(b-a for a,b in zip(p['lower_numerator'],p['upper_numerator']))*10240<=2*p['denominator']
    return p,stats


def contains(p,truth):
    den=p['denominator']
    for i,value in enumerate(truth):
        assert F(p['lower_numerator'][i],den)<=value<=F(p['upper_numerator'][i],den)


def run():
    records=[]
    def check(name,features,x,y,truth):
        p,s=pixel_bound(features,x,y);contains(p,truth)
        records.append(dict(name=name,bound=p,cells=s['cell_visits'],analytic_rgb=[str(v) for v in truth]))
    # Non-dyadic-grid phase despite exact binary64 input: prior stopping-rule counterexample.
    top=459.5553989016212-458;cy=top+.5
    fraction=F.from_float(cy)+F(1,2)-2
    check('historical_rectangle_phase',[(0,0xffff,[([(1.,cy),(7.,cy)],False)])],3,2,[fraction]*3)
    check('axis_half_butt',[(0,0xffff,[([(3.5,2.5),(7.5,2.5)],False)])],3,2,[F(1,2)]*3)
    check('axis_exact_empty_tangent',[(0,0xffff,[([(3.,2.5),(7.,2.5)],False)])],2,2,[F(0)]*3)
    # Rectangle with Pythagorean direction: exact length65537 and rational corners.
    a=(0.,0.);b=(65535.,512.);nx=F(-256,65537);ny=F(65535,2*65537)
    corners=[(nx,ny),(F(65535)+nx,F(512)+ny),(F(65535)-nx,F(512)-ny),(-nx,-ny)]
    area=clip_area(corners,3,0)
    check('near_horizontal_rational_rectangle',[(0,0xffff,[([a,b],False)])],3,0,[area]*3)
    line=([(3.5,2.5),(7.5,2.5)],False)
    check('coincident_last_writer_blue',[(0,0xf800,[line]),(1,0x001f,[line])],3,2,[F(0),F(0),F(1,2)])
    check('coincident_last_writer_red',[(0,0x001f,[line]),(1,0xf800,[line])],3,2,[F(1,2),F(0),F(0)])
    # Red right-half x[.5,1], blue top-half y[.5,1]: overlap is blue.
    check('different_color_partial_overlap',[(0,0xf800,[([(3.5,2.5),(6.,2.5)],False)]),
          (1,0x001f,[([(3.5,2.5),(3.5,5.)],False)])],3,2,[F(1,4),F(0),F(1,2)])
    # Direct disc primitive controls avoid deriving expected area from the classifier.
    def disc(cx,cy):
        g=Geometry([],16);s=g.scale;r=s//2;x=int(cx*s);y=int(cy*s)
        g.features=[(0,(255,255,255),[('disc',(x-r,y-r,x+r,y+r),(x,y,r*r))])]
        g.pixel_features=lambda x,y:tuple(reversed(g.features))
        return g
    g=disc(3.,2.);stats=dict(cell_visits=1,max_heap_cells=0)
    p=g.pixel(3,2,(0,0,0),stats,time.monotonic()+20,400000)
    assert p['state']=='precision_reached' and p['radius']<=1/10240
    # pi is enclosed by a strict rational decimal bracket independently of libm.
    pi_lo=F(314159265358979323846264338327950288419716939937510,10**50)
    pi_hi=pi_lo+F(1,10**50)
    # Quarter of a radius-.5 disc: area pi/16.
    contains(p,[pi_lo/16]*3);contains(p,[pi_hi/16]*3)
    q=g.pixel(2,1,(0,0,0),dict(cell_visits=1,max_heap_cells=0),time.monotonic()+20,400000)
    contains(q,[pi_lo/16]*3);contains(q,[pi_hi/16]*3)
    assert all(max(p['lower_numerator'][i],q['lower_numerator'][i])<=min(p['upper_numerator'][i],q['upper_numerator'][i]) for i in range(3))
    records.append(dict(name='quarter_disc_area_reflection_consistency',bound=p,reflected_bound=q,pi_lower=str(pi_lo),pi_upper=str(pi_hi),cells=stats['cell_visits']))
    g=disc(3.5,2.5);stats=dict(cell_visits=1,max_heap_cells=0)
    p=g.pixel(4,2,(0,0,0),stats,time.monotonic()+20,400000);contains(p,[F(0)]*3)
    records.append(dict(name='disc_tangent_zero_area',bound=p,cells=stats['cell_visits']))
    # Outward cell classifications verified against independent exact Fraction point predicates.
    g=Geometry([(0,0xffff,[([(0.,0.),(4.,3.),(5.,1.)],False)])]);S=g.scale
    checks=0
    for prim in g.features[0][2]:
        for ix in range(-2,12):
            for iy in range(-2,10):
                cell=(ix*S//2,iy*S//2,(ix+1)*S//2,(iy+1)*S//2);state=g.classify(prim,cell)
                for fx,fy in [(F(0),F(0)),(F(1),F(1)),(F(1,3),F(2,3))]:
                    x=F(cell[0])+fx*(cell[2]-cell[0]);y=F(cell[1])+fy*(cell[3]-cell[1])
                    if prim[0]=='disc':
                        cx,cy,r2=prim[2];inside=(x-cx)**2+(y-cy)**2<=r2
                    else:
                        ax,ay,dx,dy,l2,lim=prim[2];dot=(x-ax)*dx+(y-ay)*dy
                        inside=0<=dot<=l2 and ((x-ax)*dy-(y-ay)*dx)**2<=lim
                    if state==1:assert inside
                    if state==-1:assert not inside
                    checks+=1
    print(json.dumps(dict(controls=records,independent_predicate_checks=checks,status='PASS'),indent=2))

if __name__=='__main__':run()
