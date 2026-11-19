#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Literal frozen mask plus existing certificate arithmetic ONLY; no geographic integrator."""
import sys,json,gzip,hashlib,os,time,csv,platform
from pathlib import Path
from fractions import Fraction as F
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-experiment'))
import quality_only,oracle_rows,reference,numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'aa-certification-contract'))
from intervals import I,round_codes,expand_code
from reference_bounds import floor_deviation
platform_label=platform.platform()
out=Path(sys.argv[1]).resolve();gp=Path('/tmp/pe-aa-phase3b-fixtures/view5-r220-relief.geometry');manifest=Path('/tmp/aa-coarse-tail-evidence/combined-manifest.json');roots=[Path('/tmp/aa-full-coarse-evidence/tiles'),Path('/tmp/aa-coarse-tail-evidence/tiles')]
source_hashes={Path(m.__file__).name:hashlib.sha256(Path(m.__file__).read_bytes()).hexdigest() for m in [quality_only,oracle_rows,reference]};rows=json.loads(manifest.read_text());mh=hashlib.sha256(manifest.read_bytes()).hexdigest();opened=set();start=time.process_time()
def audit(event,args):
 if event=='open' and isinstance(args[0],(str,bytes)):
  p=Path(os.fsdecode(args[0]));mode=args[1];flags=args[2];write=(isinstance(mode,str) and any(c in mode for c in 'wax+')) or(isinstance(flags,int) and bool(flags&(os.O_WRONLY|os.O_RDWR)))
  if write and p.is_relative_to(out):return
  if not write and (p==gp or(p.suffix=='.gz' and any(p.is_relative_to(r) for r in roots))):opened.add(str(p));return
  raise PermissionError('candidate-blind audit read rejected '+str(p))
sys.addaudithook(audit)
geometry=reference.read_geometry(gp);mask=quality_only.neighborhood(geometry,640,480);raw=mask.astype(np.uint8).tobytes(order='C');assert len(raw)==307200
repeat=quality_only.neighborhood(geometry,640,480).astype(np.uint8).tobytes(order='C');assert raw==repeat
(out/'view5-r220-relief-mask.u8').write_bytes(raw)
d=255*4**16;allwidth=0;maskedwidth=0;outsidewidth=0;area_sum=0;pixels=[];tiles=[];loss_floor_dev=F(0);bands=[{'mask_pixels':0,'width_sum':0} for _ in range(30)]
for i,rec in enumerate(rows):
 assert i==rec['index'];p=roots[0 if i<4138 else 1]/f'{i:04d}.json.gz';b=p.read_bytes();assert hashlib.sha256(b).hexdigest()==rec['sha256'];r=json.loads(gzip.decompress(b));assert r['complete'] and r['tile_index']==i and len(r['pixels'])==64
 tile={'index':i,'tx':i%80,'ty':i//80,'mask_pixels':0,'width_sum':0,'max_width':0,'reference_cpu_seconds':r['cpu_seconds'],'cells':r['cell_visits'],'predicate_calls':r['predicate_calls']}
 for k,p in enumerate(r['pixels']):
  x,y=p['pixel'];assert [x,y]==[(i%80)*8+k%8,(i//80)*8+k//8] and p['denominator']==d
  ww=[v-u for u,v in zip(p['lower_numerator'],p['upper_numerator'])];assert max(ww)*2560<=2*d;allwidth+=sum(ww)
  if not mask[y,x]:outsidewidth+=sum(ww);continue
  s=sum(ww);maskedwidth+=s;tile['mask_pixels']+=1;tile['width_sum']+=s;tile['max_width']=max(tile['max_width'],max(ww));bands[y//16]['mask_pixels']+=1;bands[y//16]['width_sum']+=s
  pixels.append({'x':x,'y':y,'sum_channel_width_numerators':s,'max_channel_width_numerator':max(ww),'tile_index':i});area_sum+=F(p['unresolved_area_numerator'],p['area_denominator'])
  loss_floor_dev+=sum((floor_deviation(I(F(a,d),F(b,d)),m) for a,b,m in zip(p['lower_numerator'],p['upper_numerator'],[31,63,31])),F(0))
 tiles.append(tile)
n=int(mask.sum());assert n==len(pixels);u=F(maskedwidth,6*n*d);ufull=F(allwidth,6*307200*d);eq=loss_floor_dev/(3*n);bh=F(9,5)*u+eq/5;bd=u+eq;tau=F(10,1024)
def q(v,p):
 a=sorted(v);h=(len(a)-1)*p;k=h.__floor__();alpha=h-k
 return F(a[k]) if not alpha else F(a[k])*(1-alpha)+F(a[k+1])*alpha
levels={'min':F(0),'p25':F(1,4),'median':F(1,2),'p75':F(3,4),'p95':F(19,20),'p99':F(99,100),'max':F(1)}
means=[p['sum_channel_width_numerators'] for p in pixels];worst=[p['max_channel_width_numerator'] for p in pixels];distribution={'mean_channel_radius':{k:str(q(means,v)/(6*d)) for k,v in levels.items()},'max_channel_radius':{k:str(q(worst,v)/(2*d)) for k,v in levels.items()},'zero_width_mask_pixels':sum(x==0 for x in means)}
sortedwidth=sorted(means,reverse=True);total=maskedwidth;concentration={}
for label,frac in [('top1pct',F(1,100)),('top5pct',F(1,20)),('top10pct',F(1,10))]:
 count=(n*frac).__ceil__();concentration[label]={'pixels':count,'fraction_of_masked_width':str(F(sum(sortedwidth[:count]),total)) if total else '0'}
count90=0;acc=0
for s in sortedwidth:
 if acc*10>=total*9:break
 acc+=s;count90+=1
for t in tiles:t['masked_mean_radius']=str(F(t['width_sum'],6*t['mask_pixels']*d)) if t['mask_pixels'] else None;t['share_masked_width']=str(F(t['width_sum'],total)) if total else '0'
for j,b in enumerate(bands):b.update(y0=16*j,y1=16*j+15,share_masked_width=str(F(b['width_sum'],total)) if total else '0')
result={'fixture':'view5-r220-relief','mask_encoding':'640x480, row-major uint8,1=included/0=excluded,no header','mask_sha256':hashlib.sha256(raw).hexdigest(),'mask_bytes':len(raw),'mask_pixels':n,'mask_source_sha256':source_hashes,'geometry_sha256':hashlib.sha256(gp.read_bytes()).hexdigest(),'manifest_sha256':mh,'mask_deterministic_repeat':True,'runtime':{'python':sys.version,'numpy':np.__version__,'platform':platform_label,'float_model':'binary64 frozen NumPy/libm operations; output bytes authoritative'},'denominator':d,'masked_width_numerator':maskedwidth,'full_width_numerator':allwidth,'outside_mask_width_numerator':outsidewidth,'u_mask':str(u),'u_mask_display':float(u),'u_full':str(ufull),'u_full_display':float(ufull),'masked_over_full_ratio':str(u/ufull),'paired_midpoint_deviation_bound':str(2*u),'local_guard_deviation_bound':str(F(21,10)*u),'once_quantized_floor_loss_midpoint_deviation_bound':str(eq),'point_H_midpoint_deviation_bound_including_quantization':str(bh),'D_midpoint_deviation_bound':str(bd),'fixed_control_loose_H_illustration_2point4u':str(F(12,5)*u),'quantization_caveat':'2.4u assumes a fixed control, not globally valid across bit-replicated quantizer jumps. Actual once-Q illustration uses1.8u+0.2eQ, exact quantizer cells.','strict_floor':str(tau),'floor_midpoint_D_interval':[str(tau-bd),str(tau+bd)],'floor_equality_eligibility':'undefined; D must be strictly greater','twice_floor_midpoint_D_interval':[str(2*tau-bd),str(2*tau+bd)],'twice_floor_eligibility_proved':2*tau-bd>tau,'H_bound_over_floor_scale_only_not_eligible':str(bh/tau),'H_bound_over_exact_twice_floor':str(bh/(2*tau)),'H_bound_over_twice_floor_conservative_lower_D':str(bh/(2*tau-bd)),'distribution':distribution,'concentration':concentration,'pixels_for90pct_masked_width':count90,'masked_unresolved_area_pixel_units':str(area_sum),'widest_pixels':sorted(pixels,key=lambda p:p['sum_channel_width_numerators'],reverse=True)[:20],'top_uncertainty_tiles':sorted(tiles,key=lambda t:t['width_sum'],reverse=True)[:15],'top_existing_reference_cost_tiles':[t for t in sorted(tiles,key=lambda t:t['reference_cpu_seconds'],reverse=True) if t['mask_pixels']][:15],'row_bands':bands,'existing_reference_certificates_read':4800,'no_new_references':True,'candidate_images_read':False,'audit_cpu_seconds':time.process_time()-start}
(out/'masked-audit.json').write_text(json.dumps(result,indent=2));(out/'tiles.json').write_text(json.dumps(tiles,indent=2))
with (out/'masked-pixels.csv').open('w') as f:
 w=csv.DictWriter(f,fieldnames=list(pixels[0]));w.writeheader();w.writerows(pixels)
print(json.dumps({k:v for k,v in result.items() if k not in ['widest_pixels','top_uncertainty_tiles','top_existing_reference_cost_tiles','row_bands']},indent=2))
