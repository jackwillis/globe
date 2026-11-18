#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Frozen paired geometry sensitivity bootstrap. Synthetic self-test only in Phase 3B.
Input arrays keep geometry as axis zero and Map/Relief as axis one.
"""
import numpy as np


def bootstrap_interval(values, statistic, repeats=100000):
    values=np.asarray(values,dtype=float)
    if values.shape[0]!=16 or not np.isfinite(values).all():
        raise ValueError('16 finite paired geometry units required')
    if not isinstance(repeats,int) or repeats<=0 or repeats>1000000:
        raise ValueError('bootstrap repeat bound')
    rng=np.random.default_rng(20261118)
    result=[]
    # Batches bound temporary memory and preserve RNG sample order.
    for offset in range(0,repeats,1000):
        ids=rng.integers(0,16,size=(min(1000,repeats-offset),16))
        result.extend(statistic(values[index]) for index in ids)
    # Per-condition exclusion; never drop the other style's defined value.
    a=np.asarray(result,dtype=float)
    if a.ndim==1: a=a[:,None]
    undefined=(~np.isfinite(a)).sum(axis=0)
    fraction=undefined/repeats
    intervals=[]
    for i in range(a.shape[1]):
        good=a[np.isfinite(a[:,i]),i]
        intervals.append(np.quantile(good,[.025,.975]).tolist()
                         if len(good) and fraction[i]<=.05 else None)
    point=np.asarray(statistic(values),dtype=float).reshape(-1)
    status=['defined' if intervals[i] is not None and np.isfinite(point[i]) else 'inconclusive_floor_limited'
            for i in range(len(intervals))]
    intervals=[v if status[i]=='defined' else None for i,v in enumerate(intervals)]
    return dict(intervals=intervals,status=status,undefined_counts=undefined.tolist(),
                undefined_fractions=fraction.tolist(),defined_counts=(repeats-undefined).tolist(),
                repeats=repeats,undefined_threshold=.05,seed=20261118)


def timing_statistic(geometry_style_ratios):
    # Input ratios already reduced by within-block means then ten-block medians.
    return np.median(geometry_style_ratios,axis=0)


def quality_statistic(errors):
    # shape (16,2,3): A error, B error, original floor error; ratio of means.
    means=np.mean(errors,axis=0)
    denom=means[:,0]-means[:,2]
    return np.divide(means[:,0]-means[:,1],denom,
                     out=np.full(2,np.nan),where=denom>10/1024)


if __name__=='__main__':
    v=np.ones((16,2))
    assert bootstrap_interval(v,timing_statistic,repeats=1000)['intervals']==[[1.,1.],[1.,1.]]
    e=np.zeros((16,2,3));e[:,:,0]=.1;e[:,:,1]=.05
    assert np.allclose(quality_statistic(e),.5)
    e[:,:,0]=.001
    assert np.isnan(quality_statistic(e)).all()
    # Mixed valid/undefined: deliberately near the frozen threshold, prior to geography.
    e[:,:,0]=.011;e[:,:,1]=.005;e[:8,0,0]=.007
    mixed=bootstrap_interval(e,quality_statistic,repeats=1000)
    assert 0<mixed['undefined_counts'][0]<1000
    assert mixed['undefined_fractions'][0]>.05 and mixed['intervals'][0] is None
    assert mixed['undefined_counts'][1]==0 and mixed['status'][1]=='defined'
    # Rare undefined replicates are excluded, counted and retain a finite interval.
    e[:,:,0]=.02;e[0,0,0]=0
    rare=bootstrap_interval(e,quality_statistic,repeats=1000)
    # Exercise deterministic exclusion directly with a statistic undefined on a rare resample.
    def rare_stat(v):
        return np.array([np.nan if np.sum(v[:,0]==0)>=4 else np.mean(v[:,0]),1.])
    z=np.ones((16,2));z[0,0]=0
    rare=bootstrap_interval(z,rare_stat,repeats=10000)
    assert 0<rare['undefined_counts'][0]<=500 and rare['intervals'][0] is not None
    assert rare==bootstrap_interval(z,rare_stat,repeats=10000)
    print('PASS paired-unit bootstrap, mixed/rare undefined replicates, counts, threshold and determinism')
