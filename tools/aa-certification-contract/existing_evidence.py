#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Read existing reference certificates only; no integration or candidates."""
import json,gzip,hashlib,sys,os,time
from pathlib import Path
from fractions import Fraction as F
from intervals import I,round_codes
out=Path(sys.argv[1]).resolve();roots=[Path('/tmp/aa-full-coarse-evidence/tiles'),Path('/tmp/aa-coarse-tail-evidence/tiles')];manifest=Path('/tmp/aa-coarse-tail-evidence/combined-manifest.json');rows=json.loads(manifest.read_text());assert len(rows)==4800
opened=set()
def audit(event,args):
    if event=='open' and isinstance(args[0],(str,bytes)):
        p=Path(os.fsdecode(args[0]));mode=args[1];flags=args[2];write=(isinstance(mode,str) and any(c in mode for c in 'wax+')) or (isinstance(flags,int) and bool(flags&(os.O_WRONLY|os.O_RDWR)))
        if write and p.is_relative_to(out):return
        if write or p.suffix!='.gz' or not any(p.is_relative_to(r) for r in roots):raise PermissionError('reference-only input rejected '+str(p))
        opened.add(str(p))
sys.addaudithook(audit);start=time.process_time();width=0;largest=0;amb=0;den=255*4**16;pixels=0
for index,row in enumerate(rows):
    assert row['index']==index
    path=roots[0 if index<4138 else 1]/f'{index:04d}.json.gz';raw=path.read_bytes();assert hashlib.sha256(raw).hexdigest()==row['sha256'];r=json.loads(gzip.decompress(raw));assert r['tile_index']==index and r['complete'] and len(r['pixels'])==64
    for p in r['pixels']:
        assert p['denominator']==den
        ww=[b-a for a,b in zip(p['lower_numerator'],p['upper_numerator'])];assert max(ww)*2560<=2*den
        width+=sum(ww);largest=max(largest,max(ww));pixels+=1
        sets=[list(round_codes(I(F(a,den),F(b,den)),m)) for a,b,m in zip(p['lower_numerator'],p['upper_numerator'],[31,63,31])]
        assert [[min(v),max(v)] for v in sets]==p['quantized_codes'];amb+=any(len(v)>1 for v in sets)
u=F(width,6*pixels*den);worst=F(largest,2*den)
result={'reference_tiles_read':len(opened),'pixels':pixels,'exact_sum_channel_width_numerators':width,'channel_denominator':den,'full_frame_midpoint_mae_uncertainty':str(u),'full_frame_generic_paired_difference_uncertainty':str(2*u),'max_midpoint_radius':str(worst),'arbitrary_fixed_nonempty_mask_generic_mae_bound':str(worst),'arbitrary_fixed_nonempty_mask_generic_paired_bound':str(2*worst),'quantization_ambiguous_pixels':amb,'manifest_sha256':hashlib.sha256(json.dumps(rows,separators=(',',':')).encode()).hexdigest(),'existing_only_no_new_reference':True,'candidate_inputs_opened':False,'cpu_seconds':time.process_time()-start}
(out/'existing-evidence.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
