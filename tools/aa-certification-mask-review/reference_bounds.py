#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Reference-only once-quantized floor loss bounds; no candidates/geometric integration."""
from fractions import Fraction as F
from subsets import I
from intervals import round_codes,expand_code

def floor_loss(r,m):
    lows=[];highs=[]
    for q in round_codes(r,m):
        left=max(r.lo,F(2*q-1,2*m),F(0));right=min(r.hi,F(2*q+1,2*m),F(1));s=F(expand_code(q,m),255)
        lows.append(F(0) if left<=s<=right else min(abs(s-left),abs(s-right)))
        highs.append(max(abs(s-left),abs(s-right)))
    return I(min(lows),max(highs))

def floor_deviation(r,m):
    if r.lo==r.hi:return F(0)
    mid=(r.lo+r.hi)/2;q=(m*mid+F(1,2)).__floor__();fm=abs(F(expand_code(q,m),255)-mid);bounds=floor_loss(r,m)
    return max(fm-bounds.lo,bounds.hi-fm)
