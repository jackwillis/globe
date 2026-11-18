#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Untimed diagnostic metrics and structural work counts. No geographic scoring entrypoint."""
import math
import numpy as np
from oracle_rows import expand


def feature_metrics(candidate, reference, background, source, support, probe_pixels):
    """Isolated feature over original background; RGB means, not linear-light luminance.
    probe_pixels is frozen before candidate inspection (one per nonzero source segment midpoint).
    This measures feature detectability, not geographic correctness or human legibility.
    """
    bg=expand(background)/255.
    out=expand(candidate)/255.
    src=expand(np.asarray(source))/255.
    scale=np.mean(np.abs(src-bg),axis=-1)
    eligible=support & (scale>0)
    contrast=np.mean(np.abs(out-bg),axis=-1)
    refcontrast=np.mean(np.abs(reference-bg),axis=-1)
    ratios=np.divide(contrast,scale,out=np.zeros_like(scale),where=scale>0)
    refratios=np.divide(refcontrast,scale,out=np.zeros_like(scale),where=scale>0)
    hits=reference_hits=0
    h,w=scale.shape
    for x,y in probe_pixels:
        x,y=math.floor(x),math.floor(y)
        x0,x1=max(0,x-1),min(w,x+2); y0,y1=max(0,y-1),min(h,y+2)
        if x0>=x1 or y0>=y1: continue
        if np.max(refratios[y0:y1,x0:x1])>=.1:
            reference_hits+=1
            hits+=bool(np.max(ratios[y0:y1,x0:x1])>=.1)
    peak=float(contrast[eligible].max()) if eligible.any() else 0
    reference_peak=float(refcontrast[eligible].max()) if eligible.any() else 0
    return dict(peak_contrast=peak,
                reference_peak_contrast=reference_peak,
                peak_ratio_to_source=float(ratios[eligible].max()) if eligible.any() else 0,
                reference_peak_ratio_to_source=float(refratios[eligible].max()) if eligible.any() else 0,
                peak_retention=peak/reference_peak if reference_peak>0 else None,
                visibility_eligible=reference_hits,visibility_hits=hits,
                visibility_fraction=hits/reference_hits if reference_hits else None)


def work_counts(features,w=640,h=480):
    def area(points):
        if not points: return 0
        x0=max(0,min(w,math.floor(min(x for x,y in points)-1)))
        y0=max(0,min(h,math.floor(min(y for x,y in points)-1)))
        x1=max(0,min(w,math.ceil(max(x for x,y in points)+1)+1))
        y1=max(0,min(h,math.ceil(max(y for x,y in points)+1)+1))
        return (x1-x0)*(y1-y0)
    result=dict(segments=0,joins=0,primitive_pixel_evaluations=0,feature_bbox_pixels=0,max_segment_bbox=0)
    for _,_,chains in features:
        result['feature_bbox_pixels']+=area([q for p,_ in chains for q in p])
        for points,closed in chains:
            edges=[(a,b) for a,b in zip(points,points[1:]) if a!=b]
            if not edges: continue
            joins=[a for a,b in edges[1:]]
            if closed:
                if points[-1]!=edges[0][0]:
                    joins.append(points[-1]);edges.append((points[-1],edges[0][0]))
                joins.append(edges[0][0])
            for edge in edges:
                n=area(edge); result['segments']+=1
                result['primitive_pixel_evaluations']+=n
                result['max_segment_bbox']=max(result['max_segment_bbox'],n)
            result['joins']+=len(joins)
            result['primitive_pixel_evaluations']+=sum(area([p]) for p in joins)
    return result


def self_test():
    bg=np.zeros((5,5),dtype=np.uint16); out=bg.copy();out[2,2]=0xffff
    ref=expand(out)/255.; support=out>0
    m=feature_metrics(out,ref,bg,0xffff,support,[(2.5,2.5)])
    assert m['peak_contrast']==1 and m['visibility_fraction']==1
    assert feature_metrics(bg,ref,bg,0xffff,support,[(2.5,2.5)])['visibility_fraction']==0
    half=bg.copy();half[2,2]=0x8410
    m=feature_metrics(half,ref,bg,0xffff,support,[(2.5,2.5)])
    expected=(132+130+132)/(3*255)
    assert abs(m['peak_contrast']-expected)<1e-15
    assert m['reference_peak_contrast']==1
    assert m['peak_ratio_to_source']==m['peak_retention']==expected
    assert m['reference_peak_ratio_to_source']==1
    assert feature_metrics(bg,np.zeros_like(ref),bg,0xffff,support,[])['peak_retention'] is None
    print('PASS peak/visibility analytical controls, half-intensity retention and zero reference')

if __name__=='__main__': self_test()
