#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Bounded-subset percentile envelopes; synthetic schedules only, no scorer/RNG."""
import sys
from pathlib import Path
from fractions import Fraction as F
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-certification-contract'))
from intervals import I,FLOOR,quotient

def _kth(a,b,offset,count,rank):
    # Sorted merge order statistic, B restricted to contiguous prefix/suffix; no merge allocation.
    n=len(a);need=rank+1;lo=max(0,need-count);hi=min(need,n)
    while lo<=hi:
        i=(lo+hi)//2;j=need-i
        if i and j<count and a[i-1]>b[offset+j]:hi=i-1
        elif j and i<n and b[offset+j-1]>a[i]:lo=i+1
        else:
            if not i:return b[offset+j-1]
            if not j:return a[i-1]
            return max(a[i-1],b[offset+j-1])
    raise AssertionError('merge partition')

def _quantile(a,b,offset,count,p):
    h=(len(a)+count-1)*p;k=h.__floor__();f=h-k
    v=_kth(a,b,offset,count,k)
    return v if not f else v*(1-f)+_kth(a,b,offset,count,k+1)*f

def conditional_ratio(n,d):
    if d.hi<=FLOOR:return None
    # Included branch is strictly D>floor. Its closure is safe for inf/sup, not eligibility.
    return quotient(n,I(max(d.lo,FLOOR),d.hi))

def envelope(replicates,point_d=I(1,1),max_replicates=100000):
    n=len(replicates)
    if not 1<=n<=max_replicates:raise ValueError('bounded supplied schedule required')
    bad=[];good=[];maybe=[]
    for d,g in replicates:
        if d.hi<=FLOOR:bad.append((d,g))
        elif d.lo>FLOOR:good.append(g)
        else:maybe.append(g)
    meta=dict(repeats=n,undefined_min=len(bad),undefined_max=len(bad)+len(maybe),ambiguous=len(maybe),undefined_threshold='1/20',joint_feasibility_relaxed=True)
    if point_d.hi<=FLOOR:return dict(meta,status='inconclusive_point_floor',interval=None)
    if point_d.lo<=FLOOR:return dict(meta,status='inconclusive_point_eligibility',interval=None)
    if len(bad)*20>n:return dict(meta,status='inconclusive_floor',interval=None)
    if (len(bad)+len(maybe))*20>n:return dict(meta,status='inconclusive_threshold_eligibility',interval=None)
    if any(g is None for g in good+maybe):return dict(meta,status='inconclusive_missing_conditional_bounds',interval=None)
    a=sorted(g.lo for g in good);b=sorted(g.lo for g in maybe)
    c=sorted(g.hi for g in good);d=sorted(g.hi for g in maybe)
    lows=[];highs=[]
    for k in range(len(maybe)+1):
        lows.append(_quantile(a,b,0,k,F(1,40)))
        highs.append(_quantile(c,d,len(d)-k,k,F(39,40)))
    return dict(meta,status='subset_sensitivity_envelope',interval=I(min(lows),max(highs)),counts_evaluated=len(maybe)+1,
                lower_extremal_k=lows.index(min(lows)),upper_extremal_k=highs.index(max(highs)))
