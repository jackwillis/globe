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
    rng=np.random.default_rng(20261118)
    result=[]
    # Batches bound temporary memory and preserve RNG sample order.
    for offset in range(0,repeats,1000):
        ids=rng.integers(0,16,size=(min(1000,repeats-offset),16))
        result.extend(statistic(values[index]) for index in ids)
    return np.quantile(result,[.025,.975],axis=0)


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
    assert np.array_equal(bootstrap_interval(v,timing_statistic,repeats=1000),np.ones((2,2)))
    e=np.zeros((16,2,3));e[:,:,0]=.1;e[:,:,1]=.05
    assert np.allclose(quality_statistic(e),.5)
    e[:,:,0]=.001
    assert np.isnan(quality_statistic(e)).all()
    print('PASS paired-unit bootstrap and ratio-of-means/floor-limited synthetic controls')
