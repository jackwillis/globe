#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Post-run independent rational checks, not a producer/integrator change."""
from fractions import Fraction as F
from pathlib import Path
import hashlib,json,sys


def atan_bounds(q,n=64):
    s=sum((F(-1 if k%2 else 1,(2*k+1)*q**(2*k+1)) for k in range(n)),F(0))
    next_term=F(1,(2*n+1)*q**(2*n+1))
    # Even n: alternating partial sum is lower, first omitted term positive.
    assert n%2==0
    return s,s+next_term


def main(root):
    root=Path(root)
    controls=json.loads((root/'controls.json').read_text())
    assert controls['status']=='PASS'
    assert (F(120,119)-F(1,239))/(1+F(120,119)*F(1,239))==1
    a,b=atan_bounds(5),atan_bounds(239)
    pi_lo,pi_hi=16*a[0]-4*b[1],16*a[1]-4*b[0]
    disc=next(c for c in controls['controls'] if c['name']=='quarter_disc_area_reflection_consistency')
    assert F(disc['pi_lower'])<=pi_lo<pi_hi<=F(disc['pi_upper'])
    for p in [disc['bound'],disc['reflected_bound']]:
        for i in range(3):assert F(p['lower_numerator'][i],p['denominator'])<=pi_lo/16<pi_hi/16<=F(p['upper_numerator'][i],p['denominator'])
    rows=[]
    for i in range(4):
        r=json.loads((root/f'tile{i}.json').read_text());assert 'pixels' in r and len(r['pixels'])==64
        uncertainty=F(0);area=F(0)
        for p in r['pixels']:
            den=p['denominator'];lo=p['lower_numerator'];hi=p['upper_numerator']
            assert all(0<=x<=y<=den for x,y in zip(lo,hi))
            # Validate fixed historical precision exactly, with independent Fraction arithmetic.
            radius=max(F(b-a,2*den) for a,b in zip(lo,hi));assert radius<=F(1,10240)
            uncertainty+=sum((F(b-a,6*den) for a,b in zip(lo,hi)),F(0))/64
            area+=F(p['unresolved_area_numerator'],p['area_denominator'])
            for a,b,m,codes in zip(lo,hi,[31,63,31],p['quantized_channel_code_bounds']):
                # Independent Fraction floor rather than producer integer formula.
                q0=(F(a*m,den)+F(1,2)).numerator//(F(a*m,den)+F(1,2)).denominator
                q1=(F(b*m,den)+F(1,2)).numerator//(F(b*m,den)+F(1,2)).denominator
                assert codes==[q0,q1]
        assert uncertainty==F(r['mae_radius_numerator'],r['mae_radius_denominator'])
        rows.append(dict(index=i,exact_tile_mae_uncertainty=str(uncertainty),exact_paired_difference_uncertainty=str(2*uncertainty),
                         unresolved_color_boundary_area_pixels=float(area),max_depth_used=max(p['deepest'] for p in r['pixels']),
                         unresolved_leaves=sum(p['unresolved_leaves'] for p in r['pixels']),
                         ambiguous_pixels=[dict(pixel=p['pixel'],code_bounds=p['quantized_channel_code_bounds']) for p in r['pixels'] if any(a!=b for a,b in p['quantized_channel_code_bounds'])]))
    # Resource-stop control: a single root cell must retain a valid wide interval.
    from enclose import Geometry
    import time
    top=459.5553989016212-458;cy=top+.5
    g=Geometry([(0,0xffff,[([(1.,cy),(7.,cy)],False)])])
    bound=g.pixel(3,2,(0,0,0),dict(cell_visits=1,max_heap_cells=0),time.monotonic()+10,1)
    exact=F.from_float(cy)+F(1,2)-2
    assert bound['state']=='cell_budget'
    assert F(bound['lower_numerator'][0],bound['denominator'])<=exact<=F(bound['upper_numerator'][0],bound['denominator'])
    source=Path(__file__).parent
    freeze=json.loads((root/'pre-geographic-source-freeze.json').read_text())
    assert all(hashlib.sha256((source/n).read_bytes()).hexdigest()==h for n,h in freeze.items())
    result=dict(status='PASS',machin_identity='pi=16*atan(1/5)-4*atan(1/239); alternating rational series64terms each',
                pi_enclosure_width=str(pi_hi-pi_lo),producer_sources_unchanged_since_geographic_freeze=True,resource_stop_preserves_enclosure=True,tiles=rows)
    (root/'independent-validation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))

if __name__=='__main__':main(sys.argv[1])
