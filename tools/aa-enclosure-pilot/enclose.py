#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Pilot only: exact-integer geometric cell predicates and opaque last-writer enclosures.
Binary64 input endpoints are interpreted as exact dyadic rationals, width exactly one.
No renderer or sampled-oracle helpers. No candidate image inputs.
"""
import heapq
import time


def rgb565(v):
    r,g,b=(v>>11)&31,(v>>5)&63,v&31
    return ((r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2))


def primitive_inputs(features):
    result=[]
    for identity,color,chains in features:
        primitives=[]
        for points,closed in chains:
            p=[]
            for q in points:
                if not p or q!=p[-1]:p.append(q)
            if closed and len(p)>1 and p[-1]==p[0]:p.pop()
            if len(p)<2:continue
            for a,b in zip(p,p[1:]+([p[0]] if closed else [])):
                if a!=b:primitives.append(('rectangle',a,b))
            for q in (p if closed else p[1:-1]):primitives.append(('disc',q))
        result.append((identity,rgb565(color),primitives))
    return result


class Geometry:
    def __init__(self,features,max_depth=16):
        if not 0<=max_depth<=20:raise ValueError('pilot depth bound')
        raw=primitive_inputs(features)
        denominators=[v.as_integer_ratio()[1].bit_length()-1
                      for _,_,pp in raw for p in pp for q in p[1:] for v in q]
        self.shift=max([1,max_depth,*denominators]);self.scale=1<<self.shift
        self.max_depth=max_depth
        def point(p):
            return tuple(v.as_integer_ratio()[0]*(self.scale//v.as_integer_ratio()[1]) for v in p)
        half=self.scale//2
        self.features=[]
        for identity,color,pp in raw:
            primitives=[]
            for p in pp:
                if p[0]=='disc':
                    x,y=point(p[1]);primitives.append(('disc',(x-half,y-half,x+half,y+half),(x,y,half*half)))
                else:
                    ax,ay=point(p[1]);bx,by=point(p[2]);dx,dy=bx-ax,by-ay
                    l2=dx*dx+dy*dy
                    primitives.append(('rectangle',(min(ax,bx)-half,min(ay,by)-half,max(ax,bx)+half,max(ay,by)+half),(ax,ay,dx,dy,l2,l2*half*half)))
            self.features.append((identity,color,primitives))
        self.predicate_calls=0

    def classify(self,p,cell):
        """Return 1 inside everywhere, -1 outside everywhere, 0 unresolved; sound not complete."""
        self.predicate_calls+=1
        x0,y0,x1,y1=cell;kind,(lx,ly,hx,hy),data=p
        if x1<lx or x0>hx or y1<ly or y0>hy:return -1
        if kind=='disc':
            cx,cy,r2=data
            nearx=max(x0-cx,0,cx-x1);neary=max(y0-cy,0,cy-y1)
            if nearx*nearx+neary*neary>r2:return -1
            farx=max(abs(x0-cx),abs(x1-cx));fary=max(abs(y0-cy),abs(y1-cy))
            if farx*farx+fary*fary<=r2:return 1
            return 0
        ax,ay,dx,dy,l2,limit=data
        # Extrema of linear dot and cross forms on a rectangle occur at corners.
        def span(u,v):
            a=u*(x0-ax);b=u*(x1-ax);c=v*(y0-ay);d=v*(y1-ay)
            return min(a,b)+min(c,d),max(a,b)+max(c,d)
        lo,hi=span(dx,dy)
        if hi<0 or lo>l2:return -1
        cl,ch=span(dy,-dx)
        smallest=0 if cl<=0<=ch else min(abs(cl),abs(ch))
        if smallest*smallest>limit:return -1
        largest=max(abs(cl),abs(ch))
        return 1 if lo>=0 and hi<=l2 and largest*largest<=limit else 0

    def cell_colors(self,cell,features,background):
        # features are in reverse source order, carrying only possible primitives.
        # A certain late feature masks all earlier features. Uncertain layers yield
        # a superset of possible pointwise colors, never averaged-alpha painting.
        possible=[];survivors=[]
        for identity,color,primitives in features:
            remaining=[];inside=False
            for p in primitives:
                state=self.classify(p,cell)
                if state==1:inside=True;break
                if state==0:remaining.append(p)
            if inside:
                possible.append(color)
                survivors.append((identity,color,primitives))
                break
            if remaining:
                possible.append(color);survivors.append((identity,color,remaining))
        else:possible.append(background)
        lower=tuple(min(c[i] for c in possible) for i in range(3))
        upper=tuple(max(c[i] for c in possible) for i in range(3))
        return lower,upper,tuple(survivors)

    def pixel(self,x,y,background,stats,deadline,max_visits):
        s=self.scale;D=self.max_depth;unit=1<<(2*D)
        cell=(x*s,y*s,(x+1)*s,(y+1)*s)
        lo,hi,active=self.cell_colors(cell,tuple(reversed(self.features)),background)
        lower=[c*unit for c in lo];upper=[c*unit for c in hi]
        heap=[];serial=0;unresolved_area=0;leaf_count=1;deepest=0
        def put(box,depth,low,high,features):
            nonlocal serial,unresolved_area
            area=1<<(2*(D-depth))
            priority=sum(b-a for a,b in zip(low,high))*area
            if priority:
                unresolved_area+=area
                serial+=1;heapq.heappush(heap,(-priority,serial,box,depth,low,high,features))
        put(cell,0,lo,hi,active)
        state='precision_reached'
        # Midpoint error <= half enclosure width; target1/10240 normalized.
        while max(b-a for a,b in zip(lower,upper))*5120>255*unit:
            if not heap:raise AssertionError('unaccounted width')
            if stats['cell_visits']+4>max_visits:state='cell_budget';break
            if time.monotonic()>deadline:state='time_budget';break
            item=heap[0]
            if item[3]>=D:state='depth_budget';break
            _,_,box,depth,low,high,features=heapq.heappop(heap)
            area=1<<(2*(D-depth));unresolved_area-=area
            x0,y0,x1,y1=box;mx=(x0+x1)//2;my=(y0+y1)//2
            children=[]
            for child in ((x0,y0,mx,my),(mx,y0,x1,my),(x0,my,mx,y1),(mx,my,x1,y1)):
                a,b,f=self.cell_colors(child,features,background)
                assert all(low[i]<=a[i]<=b[i]<=high[i] for i in range(3))
                children.append((child,a,b,f))
            for i in range(3):lower[i]-=low[i]*area;upper[i]-=high[i]*area
            for child,a,b,f in children:
                for i in range(3):lower[i]+=a[i]*(area//4);upper[i]+=b[i]*(area//4)
                put(child,depth+1,a,b,f)
            deepest=max(deepest,depth+1);leaf_count+=3;stats['cell_visits']+=4
            stats['max_heap_cells']=max(stats['max_heap_cells'],len(heap))
        return dict(pixel=[x,y],lower_numerator=lower,upper_numerator=upper,denominator=255*unit,
                    radius=max(b-a for a,b in zip(lower,upper))/(2*255*unit),
                    mean_radius=sum(b-a for a,b in zip(lower,upper))/(6*255*unit),
                    unresolved_area_numerator=unresolved_area,area_denominator=unit,
                    unresolved_leaves=len(heap),total_leaves=leaf_count,deepest=deepest,state=state)

    def tile(self,tx,ty,background,max_visits=2000000,seconds=90):
        start=time.monotonic();deadline=start+seconds
        stats=dict(cell_visits=64,max_heap_cells=0);pixels=[]
        for yy in range(ty*8,ty*8+8):
            for xx in range(tx*8,tx*8+8):
                pixels.append(self.pixel(xx,yy,background(xx,yy),stats,deadline,max_visits))
        return dict(tile=[tx,ty],pixels=pixels,stats=stats,predicate_calls=self.predicate_calls,
                    max_radius=max(p['radius'] for p in pixels),tile_mae_uncertainty=sum(p['mean_radius'] for p in pixels)/64,
                    paired_mean_difference_uncertainty=2*sum(p['mean_radius'] for p in pixels)/64,
                    max_depth_limit=self.max_depth,coordinate_scale_exponent=self.shift,
                    mae_radius_numerator=sum(sum(b-a for a,b in zip(p['lower_numerator'],p['upper_numerator'])) for p in pixels),
                    mae_radius_denominator=6*64*pixels[0]['denominator'],
                    local_radius_numerator=max(max(b-a for a,b in zip(p['lower_numerator'],p['upper_numerator'])) for p in pixels),
                    local_radius_denominator=2*pixels[0]['denominator'],
                    target_attained=all(max(b-a for a,b in zip(p['lower_numerator'],p['upper_numerator']))*5120<=p['denominator'] for p in pixels),
                    seconds=time.monotonic()-start,cell_budget=max_visits,time_budget_seconds=seconds)
