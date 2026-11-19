#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import sys,json,importlib.util,random,itertools,time
from pathlib import Path
from fractions import Fraction as F
from subsets import envelope,conditional_ratio
from reference_bounds import floor_loss,floor_deviation
import intervals as baseline
from intervals import I,FLOOR,ZERO

def branch_controls(m):
    assert m.local_status(m.I(0,0),m.I(F(1,1024),F(1,1024)))==('pass',m.I(-F(1,1024),-F(1,1024))),'absolute_guard'
    r=m.bootstrap_envelope([(m.I(FLOOR,FLOOR),None)]*20)
    assert r['undefined_min']==r['undefined_max']==20 and r['interval'] is None,'bootstrap_floor_equality'
    r=m.peak_bounds([F(1,5),F(9,10)],[m.I(F(1,10),F(1,5)),m.I(0,F(1,20))],['eligible','uncertain'],[F(1),F(1)])
    assert r['candidate_peak']==m.I(F(1,5),F(9,10)),'uncertain_peak_upper'
    assert m.difference([0],[1],[m.I(F(1,4),F(3,4))])==m.I(-F(1,2),F(1,2)),'exact_tightness'
    assert m.quantized_terms(0,F(1,50),m.I(F(1,10),F(11,100)),31)['D']==m.I(F(8,85),F(8,85)),'quantized_D_exact_tightness'

def q(values,p):
    # Independent exhaustive checker: materialize all endpoint worlds; direct sorted interpolation.
    v=sorted(values);x=(len(v)-1)*p;rank=x.numerator//x.denominator;alpha=x-rank
    return v[rank] if alpha==0 else (1-alpha)*v[rank]+alpha*v[rank+1]

def exhaustive(reps):
    good=[g for d,g in reps if d.lo>FLOOR];maybe=[g for d,g in reps if d.lo<=FLOOR<d.hi];lo=[];hi=[];worlds=0
    for bits in itertools.product([False,True],repeat=len(maybe)):
        selected=good+[g for g,keep in zip(maybe,bits) if keep]
        # All membership subsets; independent endpoint extremes are exact for monotone quantiles.
        lo.append(q([g.lo for g in selected],F(1,40)));hi.append(q([g.hi for g in selected],F(39,40)));worlds+=1
    return I(min(lo),max(hi)),worlds

def main(out):
    start=time.process_time();code=Path(baseline.__file__).read_text();mutations=[]
    changes=[('drop_absolute_guard','F(2,1024)','F(0)'),('strict_bootstrap_floor','bad=sum(d.hi<=FLOOR','bad=sum(d.hi<FLOOR'),('exclude_optional_peak',"possible=[i for i,s in enumerate(eligibility) if s!='excluded' and scales[i]>0]",'possible=definite')]
    for name,old,new in changes:
        assert old in code
        path=out/(name+'.py');path.write_text(code.replace(old,new));spec=importlib.util.spec_from_file_location(name,path);mod=importlib.util.module_from_spec(spec);sys.modules[name]=mod;spec.loader.exec_module(mod)
        try:branch_controls(mod)
        except AssertionError as e:mutations.append({'mutant':name,'killed':True,'control':str(e)})
        else:raise AssertionError('surviving mutant '+name)
    # Confirm original numerical implementation passes only AFTER each targeted mutant failed.
    branch_controls(baseline)
    rng=random.Random(20261118);cases=0;worlds=0
    for n,m in [(20,1),(40,2),(60,3),(80,4),(100,5)]:
        for trial in range(24):
            good=[(I(1,1),I(F(rng.randrange(-100,100),50),F(rng.randrange(100,300),50))) for _ in range(n-m)]
            may=[(I(FLOOR-F(1,10000),FLOOR+F(1,10000)),I(F(rng.randrange(-200,0),50),F(rng.randrange(1,400),50))) for _ in range(m)]
            reps=good+may;actual=envelope(reps);truth,count=exhaustive(reps)
            assert actual['interval']==truth,(n,m,trial,actual,truth);cases+=1;worlds+=count
    for n,m,nb in [(40,1,1),(100,3,2),(100,1,3)]:
        for trial in range(20):
            good=[(I(1,1),I(F(rng.randrange(-40,10),20),F(rng.randrange(20,100),20))) for _ in range(n-m-nb)]
            may=[(I(0,2*FLOOR),I(F(-10),F(10))) for _ in range(m)]
            reps=good+may+[(I(FLOOR,FLOOR),None)]*nb
            actual=envelope(reps);truth,count=exhaustive(reps);assert actual['interval']==truth;cases+=1;worlds+=count
    finite=(I(1,1),I(F(1,4),F(1,3)));bad=(I(FLOOR,FLOOR),None);uncertain=(I(0,2*FLOOR),I(-100,100))
    assert envelope([finite]*19+[bad])['status']=='subset_sensitivity_envelope'
    assert envelope([finite]*18+[bad]*2)['status']=='inconclusive_floor'
    assert envelope([finite]*19+[uncertain])['status']=='subset_sensitivity_envelope'
    assert envelope([finite]*20+[uncertain])['status']=='subset_sensitivity_envelope'
    assert envelope([finite]*18+[uncertain])['status']=='inconclusive_threshold_eligibility'
    assert envelope([finite]*18+[bad,uncertain])['status']=='inconclusive_threshold_eligibility'
    assert envelope([finite]*20,point_d=I(FLOOR,FLOOR))['status']=='inconclusive_point_floor'
    assert envelope([finite]*20,point_d=I(0,2*FLOOR))['status']=='inconclusive_point_eligibility'
    # All four chosen membership states here are driven by ONE shared latent reference.
    # The optional-subset relaxation must enclose both feasible worlds; no noise is resampled.
    reps=[finite]*38+[uncertain,uncertain];r=envelope(reps)
    for selected in [[F(1,4)]*38,[F(1,4)]*38+[F(-50)]*2,[F(1,3)]*38+[F(50)]*2]:
        assert r['interval'].lo<=q(selected,F(1,40))<=q(selected,F(39,40))<=r['interval'].hi
    # Ratios are ratio-of-means across16paired geometry IDs; duplicate IDs retain their reference.
    schedule=[0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,15]
    errors=[[(F(i+1,1000),F(i+20,1000)),(F(i+2,1000),F(i+22,1000))] for i in range(16)]
    for style in range(2):
        num=sum(errors[i][style][0] for i in schedule)/16;den=sum(errors[i][style][1] for i in schedule)/16
        got=conditional_ratio(I(num,num),I(den,den));assert got==I(num/den,num/den)
        assert num/den!=sum(errors[i][style][0]/errors[i][style][1] for i in schedule)/16
    #20paired-ID draws: one potentially undefined draw, driven by one shared unit0 denominator.
    delta=F(1,10000);draws=[[0]+[1+(j+k)%15 for k in range(15)] for j in range(19)]+[[0]*16]
    means=[]
    for style in range(2):
        unit_d=[I(FLOOR-delta,FLOOR+delta) if i==0 and style==0 else I(FLOOR+delta*(style+1),FLOOR+delta*(style+1)) for i in range(16)]
        unit_n=[F(i+10,10000) for i in range(16)]
        point_d=sum(unit_d,ZERO).scale(F(1,16));reps=[]
        for ids in draws:
            dd=sum((unit_d[i] for i in ids),ZERO).scale(F(1,16));nn=sum(unit_n[i] for i in ids)/16
            reps.append((dd,conditional_ratio(I(nn,nn),dd)))
        bounded=envelope(reps,point_d=point_d);assert bounded['status']=='subset_sensitivity_envelope'
        for latent in [FLOOR-delta,FLOOR,FLOOR+delta]:
            actual=[]
            for ids in draws:
                dd=sum((latent if i==0 and style==0 else FLOOR+delta*(style+1)) for i in ids)/16
                if dd>FLOOR:actual.append(sum(unit_n[i] for i in ids)/16/dd)
            assert bounded['interval'].lo<=q(actual,F(1,40))<=q(actual,F(39,40))<=bounded['interval'].hi
    paired_membership_worlds=6
    # Once-quantized floor loss has discontinuities: bit expansion is not q/M.
    jump_checks=0
    for m in [31,63]:
        for k in range(m):
            tie=F(2*k+1,2*m);eps=F(1,10**9);rr=I(tie-eps,tie+eps);u=eps;eq=floor_deviation(rr,m)
            hmid=baseline.quantized_terms(0,0,I(tie,tie),m)['H'].lo
            for t in [rr.lo,tie,rr.hi]:
                point=baseline.quantized_terms(0,0,I(t,t),m)['H'].lo
                assert abs(point-hmid)<=F(9,5)*u+eq/5;jump_checks+=1
    tie=F(7,62);eps=F(1,10**9);rr=I(tie-eps,tie+eps)
    hp=baseline.quantized_terms(0,0,I(rr.lo,rr.lo),31)['H'].lo
    hm=baseline.quantized_terms(0,0,I(tie,tie),31)['H'].lo
    assert abs(hp-hm)>F(12,5)*eps #2.4u is not a universal bound for once-Q(R).
    assert floor_loss(I(F(1,10),F(11,100)),31)==I(F(1,10)-F(8,85),F(11,100)-F(8,85))
    # Larger synthetic bound demonstrates polynomial enumeration of counts, not2^optional masks.
    stress=envelope([finite]*1900+[uncertain]*100);assert stress['counts_evaluated']==101
    result={'mutation_kills':mutations,'correct_implementation_controls_pass_after_mutants':True,'exact_tightness_control':'shared difference [-1/2,1/2] and quantized D singleton8/85 equal exact analytic extrema','subset_random_cases':cases,'exhaustive_membership_subsets':worlds,'exact_subset_extrema_equal':True,'five_percent_equality_permitted':True,'threshold_eligibility_straddle_inconclusive':True,'point_floor_equality_ineligible':True,'shared_latent_reference_worlds_enclosed':True,'paired16_id_ratio_of_means_control':True,'paired16_id_conditional_membership_worlds':paired_membership_worlds,'stress_replicates':2000,'stress_ambiguous':100,'stress_k_values':101,'quantizer_jump_H_checks':jump_checks,'fixed_control_2point4u_not_used_for_changing_Q':True,'cpu_seconds':time.process_time()-start}
    (out/'review-controls.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
if __name__=='__main__':main(Path(sys.argv[1]))
