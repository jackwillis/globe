#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util,json,random,sys,time
from pathlib import Path
from fractions import Fraction as F
from enclose import Geometry,rgb565
from controls_original import clip_area

PI_LO=F(314159265358979323846264338327950288419716939937510,10**50)
PI_HI=PI_LO+F(1,10**50)

def topology(cls=Geometry):
    cases=[('interior_open_join',[(2.,0.),(0.,0.),(0.,2.)],False,(-1,-1),(PI_LO/16,PI_HI/16)),
           ('closing_square_edge',[(0.,0.),(4.,0.),(4.,4.),(0.,4.)],True,(-1,1),(F(1,2),)*2),
           ('closed_square_join',[(0.,0.),(4.,0.),(4.,4.),(0.,4.)],True,(-1,-1),(PI_LO/16,PI_HI/16))]
    a=(F(-3),F(4));b=(F(0),F(0));nx,ny=F(2,5),F(3,10)
    polygon=[(a[0]+nx,a[1]+ny),(b[0]+nx,b[1]+ny),(b[0]-nx,b[1]-ny),(a[0]-nx,a[1]-ny)]
    exact=clip_area(polygon,-2,2)
    ring=[(0.,0.),(4.,3.),(1.,7.),(-3.,4.)]
    cases += [('closed_ring_edge',ring,True,(-2,2),(exact,exact)),('duplicate_closure',ring+[(0.,0.)],True,(-2,2),(exact,exact)),('duplicate_vertex',ring[:2]+[ring[1]]+ring[2:]+[(0.,0.)],True,(-2,2),(exact,exact))]
    result=[]
    for name,p,closed,(x,y),(lo,hi) in cases:
        g=cls([(0,0xffff,[(p,closed)])]);s=dict(cell_visits=1,max_heap_cells=0)
        v=g.pixel(x,y,(0,0,0),s,time.monotonic()+10,200000)
        assert v['state']=='precision_reached',name
        assert max(b-a for a,b in zip(v['lower_numerator'],v['upper_numerator']))*10240<=2*v['denominator'],name
        assert F(v['lower_numerator'][0],v['denominator'])<=lo<=hi<=F(v['upper_numerator'][0],v['denominator']),name
        result.append(dict(name=name,cells=s['cell_visits'],radius=v['radius'],exact_interval=[str(lo),str(hi)]))
    return result


def independent_contains(points,closed,x,y):
    # Built from raw source chain, never from producer primitives.
    p=[]
    for a in points:
        a=tuple(F(v) for v in a)
        if not p or a!=p[-1]:p.append(a)
    if len(p)<2:return False
    if closed and p[0]==p[-1]:p.pop()
    edges=list(zip(p,p[1:]))
    if closed:edges.append((p[-1],p[0]))
    for a,b in edges:
        dx,dy=b[0]-a[0],b[1]-a[1];l2=dx*dx+dy*dy
        if not l2:continue
        dot=(x-a[0])*dx+(y-a[1])*dy
        cross=(x-a[0])*dy-(y-a[1])*dx
        if 0<=dot<=l2 and cross*cross<=l2/4:return True
    joins=p if closed else p[1:-1]
    return any((x-a)**2+(y-b)**2<=F(1,4) for a,b in joins)


def randomized():
    rng=random.Random(20261118);checks=0
    for scene in range(96):
        ff=[]
        for i in range(3):
            p=[(rng.randrange(-8,17)/4,rng.randrange(-8,17)/4) for _ in range(rng.randrange(2,6))]
            closed=bool(rng.getrandbits(1))
            if scene%3==0:p.append(p[0])
            if scene%4==0:p.insert(1,p[0])
            ff.append((i,rng.randrange(65536),[(p,closed)]))
        g=Geometry(ff);background=rgb565(rng.randrange(65536));S=g.scale
        for cellnum in range(4):
            x=rng.randrange(-4,8);y=rng.randrange(-4,8)
            cell=(x*S//2,y*S//2,(x+1)*S//2,(y+1)*S//2)
            lo,hi,_=g.cell_colors(cell,tuple(reversed(g.features)),background)
            indexed=g.cell_colors(cell,g.pixel_features(x//2,y//2),background)
            assert indexed[:2]==(lo,hi),'BVH changes conservative color range'
            for u in (F(0),F(1,2),F(1)):
                for v in (F(0),F(1,2),F(1)):
                    px=F(x,2)+u/2;py=F(y,2)+v/2;color=background
                    for _,c,cc in ff:
                        if any(independent_contains(p,closed,px,py) for p,closed in cc):color=rgb565(c)
                    assert all(a<=c<=b for a,c,b in zip(lo,color,hi)),(scene,cellnum,px,py)
                    checks+=1
    # Integrated randomized axis rectangles, with exact last-writer area via source axis cuts.
    integral_cases=0
    for k in range(12):
        ff=[];cutsx={F(0),F(1)};cutsy={F(0),F(1)};regions=[]
        for i in range(3):
            x0=F(rng.randrange(0,3),4);x1=x0+F(1,2);cy=F(rng.randrange(0,5),4)
            p=[(float(x0),float(cy)),(float(x1),float(cy))];c=rng.randrange(65536)
            ff.append((i,c,[(p,False)]));regions.append((x0,x1,cy-F(1,2),cy+F(1,2),rgb565(c)))
            cutsx.update(a for a in [x0,x1] if 0<a<1);cutsy.update(a for a in [cy-F(1,2),cy+F(1,2)] if 0<a<1)
        sx,sy=sorted(cutsx),sorted(cutsy);truth=[F(0)]*3
        for a,b in zip(sx,sx[1:]):
            for c,d in zip(sy,sy[1:]):
                px,py=(a+b)/2,(c+d)/2;color=(0,0,0)
                for x0,x1,y0,y1,col in regions:
                    if x0<=px<=x1 and y0<=py<=y1:color=col
                for j in range(3):truth[j]+=(b-a)*(d-c)*F(color[j],255)
        g=Geometry(ff);r=g.pixel(0,0,(0,0,0),dict(cell_visits=1,max_heap_cells=0),time.monotonic()+10,200000)
        assert r['state']=='precision_reached'
        assert all(F(a,r['denominator'])<=t<=F(b,r['denominator']) for a,t,b in zip(r['lower_numerator'],truth,r['upper_numerator']))
        integral_cases+=1
    return dict(raw_chain_point_comparisons=checks,randomized_integrated_color_cases=integral_cases)


def mutations(out):
    code=Path(__file__).with_name('enclose.py').read_text();results=[]
    replacements=[('missing_open_joins','p if closed else p[1:-1]','p if closed else []'),('missing_closing_edges','p[1:]+([p[0]] if closed else [])','p[1:]'),('missing_closed_joins','p if closed else p[1:-1]','[] if closed else p[1:-1]')]
    for name,old,new in replacements:
        assert old in code
        path=out/(name+'.py');path.write_text(code.replace(old,new))
        spec=importlib.util.spec_from_file_location(name,path);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        try:topology(module.Geometry)
        except AssertionError as e:results.append(dict(mutant=name,killed=True,failing_control=str(e)))
        else:raise AssertionError('surviving mutant '+name)
    return results

if __name__=='__main__':
    out=Path(sys.argv[1]);result=dict(topology=topology(),randomized=randomized(),mutations=mutations(out))
    for bad in (float('nan'),float('inf'),1e6+1,1e-200):
        try:Geometry([(0,0xffff,[([(bad,0.),(1.,1.)],False)])])
        except ValueError:pass
        else:raise AssertionError('invalid domain accepted')
    fair=Geometry([(0,0xffff,[([(0.,0.),(8.,8.)],False)])]).tile(0,0,lambda x,y:(0,0,0),max_visits=64,seconds=10)
    assert all(p['cell_visits']==1 and p['pixel_cell_quota']==1 for p in fair['pixels'])
    assert len(fair['pixels'])==64
    result['fair_budget_control']={'pixels':64,'fixed_cell_quota':1,'all_initial_enclosures_retained':True}
    result['domain_rejection']='PASS finite +/-1e6, dyadic denominator<=2^128'
    (out/'reliability.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
