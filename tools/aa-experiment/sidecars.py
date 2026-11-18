#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Isolated feature sidecars, NO aggregate geographic quality calculation.
Real execution requires later owner authorization and positive reference margin.
"""
import argparse,json,subprocess
from pathlib import Path
import numpy as np
from reference import read_geometry
from oracle_rows import Oracle


def generate(geometry,background,driver,output,width,height,margin):
    if not np.isfinite(margin) or margin<=0: raise ValueError('positive decision margin required')
    features=read_geometry(geometry)
    bg=np.frombuffer(Path(background).read_bytes(),dtype='<u2').reshape(height,width)
    output=Path(output);output.mkdir(parents=True,exist_ok=False)
    records=[];storage=0
    for index,(identity,color,chains) in enumerate(features):
        candidates={}
        for arm in ('A','B'):
            raw=subprocess.check_output([str(driver),str(geometry),str(background),str(width),str(height),str(index),arm])
            candidates[arm]=np.frombuffer(raw,dtype='<u2').reshape(height,width)
        feature=[features[index]]
        ref,q,f,log=Oracle(feature,width,height).converged(bg,margin)
        # Probes depend only on original supplied nonzero edges, never candidate output.
        probes=[((a[0]+b[0])/2,(a[1]+b[1])/2) for points,_ in chains
                for a,b in zip(points,points[1:]) if a!=b]
        # Include implicit closing edge if present, excluding duplicate closure.
        for points,closed in chains:
            if closed and points[-1]!=points[0]:
                a,b=points[-1],points[0];probes.append(((a[0]+b[0])/2,(a[1]+b[1])/2))
        path=output/f'feature{index:04d}.npz'
        np.savez_compressed(path,candidate_a=candidates['A'],candidate_b=candidates['B'],
                            reference=ref,quantized=q,sequential=f,background=bg,source=np.uint16(color),
                            support=np.any(ref!=OracleBackground(bg),axis=2),probes=np.array(probes).reshape(-1,2))
        storage+=path.stat().st_size
        if storage>4*1024**3: raise OSError('sidecar storage cap exceeded; incomplete manifest withheld')
        records.append(dict(index=index,identity=identity,color=color,file=path.name,convergence=log))
    (output/'manifest.json').write_text(json.dumps(records,indent=2)+'\n')
    return records


def OracleBackground(bg):
    from oracle_rows import expand
    return expand(bg)/255.

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('geometry','background','driver','output'):p.add_argument(name,type=Path)
    p.add_argument('--width',type=int,default=640);p.add_argument('--height',type=int,default=480)
    p.add_argument('--decision-margin',type=float,required=True)
    a=p.parse_args();generate(a.geometry,a.background,a.driver,a.output,a.width,a.height,a.decision_margin)
