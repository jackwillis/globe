#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Compare Python parsing to C++ in-memory extraction counts, not any candidate image."""
import hashlib,json,sys
from pathlib import Path
from reference import read_geometry


def describe(features):
    detail=[]
    for identity,color,chains in features:
        cc=[]
        for points,closed in chains:
            # Deduplicated adjacent points; independently derive B's effective primitives.
            p=[]
            for q in points:
                if not p or q!=p[-1]: p.append(q)
            segments=max(0,len(p)-1)
            joins=max(0,segments-1)
            if closed and segments:
                if p[-1]!=p[0]: segments+=1;joins+=1
                joins+=1
            cc.append(dict(closed=closed,vertices=len(points),supplied_edges=len(points)-1,segments=segments,joins=joins))
        detail.append(dict(id=identity,color=color,chain_count=len(chains),chains=cc))
    return dict(features=len(features),chains=sum(len(chains) for _,_,chains in features),
                vertices=sum(len(p) for _,_,chains in features for p,_ in chains),detail=detail)

if __name__=='__main__':
    root,extracted,counts,out=map(Path,sys.argv[1:])
    rows=[json.loads(s) for s in counts.read_text().splitlines()]
    assert len(rows)==32 and len({r['fixture'] for r in rows})==32
    evidence=[]
    for r in rows:
        p=root/(r['fixture']+'.geometry')
        assert p.read_bytes()==(extracted/p.name).read_bytes()
        python=describe(read_geometry(p));assert python==r['counts'],p.name
        evidence.append(dict(fixture=r['fixture'],sha256=hashlib.sha256(p.read_bytes()).hexdigest(),
                             agreement=True,counts=python))
    out.write_text(json.dumps(evidence,indent=2)+'\n')
    print('PASS 32/32 geometry byte equality and complete Python/C++ per-feature/per-chain metadata/count equality')
