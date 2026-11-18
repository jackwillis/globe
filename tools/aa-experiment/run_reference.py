#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Reference generation entrypoint for a LATER authorized run; no candidate scoring.
Requires numpy. Run only after protocol freeze/owner authorization. No timing collected.
"""
import argparse, json
from pathlib import Path
import numpy as np
from reference import read_geometry
from oracle_rows import Oracle
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('geometry',type=Path);p.add_argument('background',type=Path)
p.add_argument('output',type=Path);p.add_argument('--width',type=int,default=640)
p.add_argument('--height',type=int,default=480)
p.add_argument('--decision-margin',type=float,required=True)
a=p.parse_args()
o=Oracle(read_geometry(a.geometry),a.width,a.height)
if a.background.stat().st_size!=a.width*a.height*2: raise ValueError('background byte size')
bg=np.frombuffer(a.background.read_bytes(),dtype='<u2').reshape(a.height,a.width)
rgb,q,f,log=o.converged(bg,a.decision_margin)
# No files produced on convergence failure. Exceptions imply inconclusive, never candidate rejection.
a.output.mkdir(parents=True,exist_ok=True)
np.save(a.output/'continuous.npy',rgb)
q.astype('<u2').tofile(a.output/'quantized.rgb565')
f.astype('<u2').tofile(a.output/'sequential.rgb565')
(a.output/'convergence.json').write_text(json.dumps(log,indent=2)+'\n')
