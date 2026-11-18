#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Independent polygon/disc sample-row oracle. Never imports candidate implementation.

Tiles contain primitive indices; one high-resolution row is painted in source order.
Memory does not scale with samples squared. numpy is host research tooling only.
"""
import math
import numpy as np
from reference import channels


def primitives(features):
    out = []
    for fi, (_, color, chains) in enumerate(features):
        for points, closed in chains:
            p = []
            for q in points:
                if not p or q != p[-1]:
                    p.append(q)
            if closed and len(p) > 1 and p[-1] == p[0]:
                p.pop()
            if len(p) < 2:
                continue
            edges = list(zip(p, p[1:] + ([p[0]] if closed else [])))
            for (ax, ay), (bx, by) in edges:
                dx, dy = bx-ax, by-ay
                length = math.sqrt(dx*dx+dy*dy)
                if length == 0:
                    continue
                nx, ny = -dy/(2*length), dx/(2*length)
                vertices = [(ax+nx,ay+ny),(bx+nx,by+ny),
                            (bx-nx,by-ny),(ax-nx,ay-ny)]
                out.append((fi, color, 'polygon', vertices))
            for q in (p if closed else p[1:-1]):
                out.append((fi, color, 'disc', q))
    return out


def bounds(primitive):
    _, _, kind, data = primitive
    if kind == 'disc':
        x, y = data
        return x-.5, y-.5, x+.5, y+.5
    return min(p[0] for p in data), min(p[1] for p in data), max(p[0] for p in data), max(p[1] for p in data)


def interval(primitive, y):
    _, _, kind, data = primitive
    if kind == 'disc':
        x, cy = data
        r2 = .25-(y-cy)**2
        if r2 < 0:
            return None
        half = math.sqrt(r2)
        return x-half, x+half
    xs = []
    for a, b in zip(data, data[1:]+data[:1]):
        if a[1] == b[1]:
            if y == a[1]:
                xs.extend((a[0], b[0]))
        elif min(a[1],b[1]) <= y <= max(a[1],b[1]):
            xs.append(a[0]+(y-a[1])*(b[0]-a[0])/(b[1]-a[1]))
    return (min(xs),max(xs)) if xs else None


def quantize(rgb):
    q = np.floor(np.clip(rgb,0,1)*np.array([31,63,31])+.5).astype(np.uint16)
    return (q[...,0]<<11)|(q[...,1]<<5)|q[...,2]


def expand(a):
    a = np.asarray(a, dtype=np.uint16)
    r,g,b = (a>>11)&31,(a>>5)&63,a&31
    return np.stack(((r<<3)|(r>>2),(g<<2)|(g>>4),(b<<3)|(b>>2)),axis=-1)


def sequential(dst, color, coverage):
    # Independent implementation of frozen integer blend; no aa helpers.
    q = np.floor(coverage*255+.5).astype(np.int64)
    d = expand(dst).astype(np.int64)
    s = np.array(channels(color),dtype=np.int64)
    z = (q[...,None]*s+(255-q[...,None])*d+127)//255
    v = (z*np.array([31,63,31])+127)//255
    return ((v[...,0]<<11)|(v[...,1]<<5)|v[...,2]).astype(np.uint16)


class Oracle:
    def __init__(self, features, width, height, tile=8):
        if not 1 <= width <= 4096 or not 1 <= height <= 4096 or width*height > 640*480 or tile != 8:
            raise ValueError('dimensions/tile bound')
        if len(features)>8192:
            raise ValueError('feature bound')
        count = sum(len(p) for _,_,cc in features for p,_ in cc)
        if count>262144 or any(not math.isfinite(v) or abs(v)>1e6
                              for _,_,cc in features for p,_ in cc for q in p for v in q):
            raise ValueError('point bound/domain')
        self.features, self.width, self.height, self.tile = features,width,height,tile
        self.primitives = primitives(features)
        self.bins = {}
        entries = 0
        for pi,p in enumerate(self.primitives):
            x0,y0,x1,y1 = bounds(p)
            for ty in range(max(0,math.floor(y0/tile)),min((height-1)//tile,math.floor(y1/tile))+1):
                for tx in range(max(0,math.floor(x0/tile)),min((width-1)//tile,math.floor(x1/tile))+1):
                    entries += 1
                    if entries>1_000_000:
                        raise MemoryError('tile index budget exceeded')
                    self.bins.setdefault((tx,ty),[]).append(pi)
        self.entries = entries

    def tile_reference(self, tx, ty, background, n):
        if n not in (64,128,256,512,1024):
            raise ValueError('sample schedule')
        x,y=tx*8,ty*8
        h,w=min(8,self.height-y),min(8,self.width-x)
        bg=np.asarray(background[y:y+h,x:x+w],dtype=np.uint16)
        ids=self.bins.get((tx,ty),[])
        sums=np.zeros((h,w,3),dtype=np.uint64)
        achievable=bg.copy()
        grouped={}
        for i in ids:
            grouped.setdefault(self.primitives[i][0],[]).append(self.primitives[i])
        # Coverage counts retained only for features intersecting this tile.
        counts={f:np.zeros((h,w),dtype=np.uint32) for f in grouped}
        for row in range(h):
            base=np.repeat(expand(bg[row]),n,axis=0)
            for sy in range(n):
                py=y+row+(sy+.5)/n
                colors=base.copy()
                for f,pp in grouped.items():
                    mask=np.zeros(w*n,dtype=bool)
                    for primitive in pp:
                        span=interval(primitive,py)
                        if span is None:
                            continue
                        lo=max(0,math.ceil((span[0]-x)*n-.5))
                        hi=min(w*n,math.floor((span[1]-x)*n-.5)+1)
                        if hi>lo:
                            mask[lo:hi]=True
                    colors[mask]=channels(self.features[f][1])
                    counts[f][row]+=mask.reshape(w,n).sum(axis=1,dtype=np.uint32)
                sums[row]+=colors.reshape(w,n,3).sum(axis=1,dtype=np.uint64)
        for f,c in counts.items():
            achievable=sequential(achievable,self.features[f][1],c/(n*n))
        rgb=sums/(n*n*255.)
        return rgb,quantize(rgb),achievable

    def render(self, background, n=64):
        bg=np.asarray(background,dtype=np.uint16)
        if bg.shape!=(self.height,self.width):
            raise ValueError('background dimensions')
        rgb=expand(bg)/255.
        quant=bg.copy(); floor=bg.copy()
        for tx,ty in sorted(self.bins):
            h,w=min(8,self.height-ty*8),min(8,self.width-tx*8)
            a,b,c=self.tile_reference(tx,ty,bg,n)
            rgb[ty*8:ty*8+h,tx*8:tx*8+w]=a
            quant[ty*8:ty*8+h,tx*8:tx*8+w]=b
            floor[ty*8:ty*8+h,tx*8:tx*8+w]=c
        return rgb,quant,floor

    def converged(self, background, margin=None):
        # Only affected tiles refine. No candidate output inspected here.
        bg=np.asarray(background,dtype=np.uint16)
        if bg.shape!=(self.height,self.width):
            raise ValueError('background dimensions')
        rgb=expand(bg)/255.; quant=bg.copy(); floor=bg.copy(); log=[]
        if margin is not None and (not math.isfinite(margin) or margin<=0):
            raise ValueError('finite positive decision margin required')
        limit=min(1/1024, .05*margin) if margin is not None else 1/1024
        if limit<=0:
            raise ValueError('positive decision margin required')
        for tx,ty in sorted(self.bins):
            prev=self.tile_reference(tx,ty,bg,64)
            for n in (128,256,512,1024):
                cur=self.tile_reference(tx,ty,bg,n)
                delta=float(np.max(np.abs(cur[0]-prev[0])))
                # Also track per-feature-floor convergence; quantization jumps recorded.
                floor_delta=float(np.max(np.abs(expand(cur[2]).astype(float)-expand(prev[2])))/255)
                log.append(dict(tile=[tx,ty],samples=n,delta=delta,floor_delta=floor_delta,limit=limit))
                if delta<=limit:
                    break
                prev=cur
            else:
                raise RuntimeError('inconclusive reference convergence at tile '+str((tx,ty)))
            h,w=cur[0].shape[:2]
            rgb[ty*8:ty*8+h,tx*8:tx*8+w]=cur[0]
            quant[ty*8:ty*8+h,tx*8:tx*8+w]=cur[1]
            floor[ty*8:ty*8+h,tx*8:tx*8+w]=cur[2]
        return rgb,quant,floor,log
