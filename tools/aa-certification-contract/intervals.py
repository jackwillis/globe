#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Small exact-rational synthetic demonstrator, NOT a geographic scorer."""
from dataclasses import dataclass
from fractions import Fraction as F
from itertools import product

@dataclass(frozen=True)
class I:
    lo:F
    hi:F
    def __post_init__(self):
        object.__setattr__(self,'lo',F(self.lo));object.__setattr__(self,'hi',F(self.hi))
        if self.lo>self.hi:raise ValueError('reversed enclosure')
    def __add__(self,b):
        b=b if isinstance(b,I) else I(b,b)
        return I(self.lo+b.lo,self.hi+b.hi)
    def __sub__(self,b):
        b=b if isinstance(b,I) else I(b,b)
        return I(self.lo-b.hi,self.hi-b.lo)
    def scale(self,k):
        a,b=self.lo*k,self.hi*k
        return I(min(a,b),max(a,b))
    def contains(self,x):return self.lo<=x<=self.hi

ZERO=I(0,0)
FLOOR=F(10,1024)
TARGET=F(1,5)

def linear_losses(r,terms):
    """Range of sum w*|x-r|; extrema at interval ends and in-range kinks."""
    points={r.lo,r.hi}|{F(x) for _,x in terms if r.lo<=x<=r.hi}
    values=[sum(w*abs(x-t) for w,x in terms) for t in points]
    return I(min(values),max(values))

def mae(candidate,reference):
    if not candidate or len(candidate)!=len(reference):raise ValueError('nonempty fixed scalar mask')
    return sum((linear_losses(r,[(1,F(c))]) for c,r in zip(candidate,reference)),ZERO).scale(F(1,len(candidate)))

def difference(a,b,reference):
    if not a or not len(a)==len(b)==len(reference):raise ValueError('shared fixed mask')
    return sum((linear_losses(r,[(1,F(x)),(-1,F(y))]) for x,y,r in zip(a,b,reference)),ZERO).scale(F(1,len(a)))

def round_codes(r,m):
    if m not in (31,63,255) or r.lo<0 or r.hi>1:raise ValueError('normalized quantizer domain')
    return range((m*r.lo+F(1,2)).__floor__(),(m*r.hi+F(1,2)).__floor__()+1)

def expand_code(q,m):
    if m==31:return (q<<3)|(q>>2)
    if m==63:return (q<<2)|(q>>4)
    raise ValueError('RGB channel depth')

def quantized_terms(a,b,r,m):
    """Shared-reference N,D,H and once-quantized control loss, half-up quantizer.
    Cell closures include limiting boundary values conservatively, never remove a true value.
    """
    den=[];res=[];floor=[]
    for q in round_codes(r,m):
        cell=I(max(r.lo,F(2*q-1,2*m),F(0)),min(r.hi,F(2*q+1,2*m),F(1)))
        x=F(expand_code(q,m),255)
        den.append(linear_losses(cell,[(1,F(a)),(-1,x)]))
        res.append(linear_losses(cell,[(1-TARGET,F(a)),(-1,F(b)),(TARGET,x)]))
        floor.append(linear_losses(cell,[(1,x)]))
    hull=lambda vv:I(min(v.lo for v in vv),max(v.hi for v in vv))
    return {'N':linear_losses(r,[(1,F(a)),(-1,F(b))]),'D':hull(den),'H':hull(res),'floor_loss':hull(floor)}

def quotient(n,d):
    if d.lo<=0:raise ValueError('positive denominator not proved')
    values=[a/b for a in (n.lo,n.hi) for b in (d.lo,d.hi)]
    return I(min(values),max(values))

def gap_status(h,d):
    if d.hi<=FLOOR:return 'floor_limited'
    if d.lo<=FLOOR:return 'inconclusive_denominator'
    if h.lo>=0:return 'pass_point_target'
    if h.hi<0:return 'fail_point_target'
    return 'inconclusive_target'

def local_status(a_error,b_minus_a):
    # Shared difference is bounded first, not manufactured from independent errors.
    residual=b_minus_a-I(max(a_error.lo/10,F(2,1024)),max(a_error.hi/10,F(2,1024)))
    return ('pass' if residual.hi<=0 else 'violation' if residual.lo>0 else 'inconclusive'),residual

def expand565(v):return (expand_code((v>>11)&31,31),expand_code((v>>5)&63,63),expand_code(v&31,31))

def blend(dst,src,q):
    d,s=expand565(dst),expand565(src)
    codes=[(((q*b+(255-q)*a+127)//255)*m+127)//255 for a,b,m in zip(d,s,[31,63,31])]
    return (codes[0]<<11)|(codes[1]<<5)|codes[2]

def sequential(background,layers,state_limit=4096):
    """Packed sets preserve shared alpha within channels. Layer independence is a relaxation."""
    states={background}
    for color,coverage in layers:
        qs=round_codes(coverage,255)
        if len(states)*len(qs)>state_limit:return None # unknown, never truncate or pass
        states={blend(d,color,q) for d,q in product(states,qs)}
    return states

def support(coverage,scale):
    if scale==0 or coverage.hi==0:return 'excluded'
    return 'eligible' if coverage.lo>0 else 'uncertain'

def visibility_counts(definite_hits,definite_misses,optional_hits,optional_misses):
    d=definite_hits+definite_misses
    if d+optional_hits+optional_misses==0:return {'fraction':None,'undefined_possible':True}
    lo=F(definite_hits,d+optional_misses) if d+optional_misses else F(1)
    hi=F(definite_hits+optional_hits,d+optional_hits) if d+optional_hits else F(0)
    return {'fraction':I(lo,hi),'undefined_possible':d==0}

def quantile(values,p):
    vv=sorted(values);h=(len(vv)-1)*p;k=h.__floor__();f=h-k
    return vv[k] if k==len(vv)-1 else vv[k]*(1-f)+vv[k+1]*f

def bootstrap_envelope(replicates,point_defined=True):
    """Finite pre-supplied synthetic paired-ID replicates, not the frozen RNG/scorer.
    Each item:(D interval,G interval or None). Undefined membership cannot be guessed.
    """
    n=len(replicates)
    if not n:raise ValueError('empty replicate schedule')
    bad=sum(d.hi<=FLOOR for d,g in replicates)
    maybe=sum(d.lo<=FLOOR<d.hi for d,g in replicates)
    meta={'undefined_min':bad,'undefined_max':bad+maybe,'ambiguous':maybe,'repeats':n,'threshold':F(1,20)}
    if not point_defined or F(bad,n)>F(1,20):return dict(meta,status='inconclusive_floor',interval=None)
    if maybe:return dict(meta,status='inconclusive_definedness',interval=None)
    good=[g for d,g in replicates if d.lo>FLOOR]
    if not good:return dict(meta,status='inconclusive_floor',interval=None)
    return dict(meta,status='conditional_sensitivity_envelope',interval=I(quantile([g.lo for g in good],F(1,40)),quantile([g.hi for g in good],F(39,40))))

def peak_bounds(candidate,refcontrast,eligibility,scales):
    if not len(candidate)==len(refcontrast)==len(eligibility)==len(scales):raise ValueError('same eligible pixel set required')
    definite=[i for i,s in enumerate(eligibility) if s=='eligible' and scales[i]>0]
    possible=[i for i,s in enumerate(eligibility) if s!='excluded' and scales[i]>0]
    def peaks(values):
        return I(max([values[i].lo for i in definite]+[F(0)]),max([values[i].hi for i in possible]+[F(0)]))
    c=peaks([I(x,x) for x in candidate]);r=peaks(refcontrast)
    cs=peaks([I(x/q,x/q) if q else ZERO for x,q in zip(candidate,scales)])
    rs=peaks([v.scale(1/q) if q else ZERO for v,q in zip(refcontrast,scales)])
    return dict(candidate_peak=c,reference_peak=r,peak_ratio_to_source=cs,reference_peak_ratio_to_source=rs,
                retention=quotient(c,r) if r.lo>0 else None,undefined_retention_possible=r.lo==0)

def support_mae(definite,optional):
    if not definite:return {'interval':I(0,1) if optional else ZERO,'empty_possible':True}
    return {'interval':I(sum(x.lo for x in definite)/(len(definite)+len(optional)),min(F(1),sum(x.hi for x in definite+optional)/len(definite))),'empty_possible':False}
