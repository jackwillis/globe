#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Independent analytical/exhaustive synthetic controls; no geographic inputs."""
import itertools,json,random,sys,time,resource
from pathlib import Path
from fractions import Fraction as F
from intervals import I,ZERO,FLOOR,TARGET,mae,difference,linear_losses,quantized_terms,round_codes,blend,sequential,expand565,quotient,gap_status,local_status,support,visibility_counts,peak_bounds,support_mae,bootstrap_envelope,quantile

# Independent truth path decodes bit strings and rounds exact rational averages.
def nearest(x):return (2*x.numerator+x.denominator)//(2*x.denominator)
def channels(v):
    bits=f'{v:016b}'
    return tuple(int((s*2)[:8],2) for s in (bits[:5],bits[5:11],bits[11:]))
def independent_blend(dst,src,q):
    a,b=channels(dst),channels(src);out=[]
    for x,y,m in zip(a,b,[31,63,31]):
        z=nearest(F((255-q)*x+q*y,255));out.append(nearest(F(z*m,255)))
    return out[0]*2048+out[1]*32+out[2]
def qvalue(t,m):
    code=nearest(t*m);bits=f'{code:05b}' if m==31 else f'{code:06b}'
    return F(int((bits*2)[:8],2),255)
def known_terms(a,b,t,m):
    ea,eb=abs(a-t),abs(b-t);ef=abs(qvalue(t,m)-t)
    return {'N':ea-eb,'D':ea-ef,'H':(ea-eb)-F(1,5)*(ea-ef),'floor_loss':ef}

def run(out):
    start=time.process_time();records={};point_checks=0
    grid=[F(i,32) for i in range(33)]
    # Every analytic kink is in this independent exhaustive grid.
    for a,b in itertools.product([F(0),F(1,4),F(3,4),F(1)],repeat=2):
        for l in range(8):
            for u in range(l+1,9):
                r=I(F(l,8),F(u,8));got=difference([a],[b],[r]);truth=[abs(a-t)-abs(b-t) for t in grid if r.contains(t)]
                assert got==I(min(truth),max(truth));point_checks+=len(truth)
    assert difference([F(1,2)],[F(1,2)],[I(0,1)])==ZERO
    independent=mae([F(1,2)],[I(0,1)])-mae([F(1,2)],[I(0,1)])
    assert independent==I(-F(1,2),F(1,2))
    records['fixed_mask_shared_reference']={'exhaustive_truth_values':point_checks,'identical_candidates_difference':'[0,0]','independent_error_subtraction':'[-1/2,1/2]'}
    ties=0
    for m in [31,63,255]:
        for k in range(m):
            t=F(2*k+1,2*m);eps=F(1,10**9)
            assert list(round_codes(I(t,t),m))==[k+1]
            assert list(round_codes(I(t-eps,t),m))==[k,k+1]
            assert list(round_codes(I(t-eps,t-eps),m))==[k]
            ties+=1
    records['all_half_up_ties']={'ties':ties,'endpoint_codes_0_and_max':all(list(round_codes(I(0,0),m))==[0] and list(round_codes(I(1,1),m))==[m] for m in [31,63,255])}
    rng=random.Random(20261118);checks=0
    for _ in range(160):
        lo,hi=sorted([F(rng.randrange(257),256),F(rng.randrange(257),256)]);r=I(lo,hi);a,b=F(rng.randrange(256),255),F(rng.randrange(256),255);m=rng.choice([31,63]);bounds=quantized_terms(a,b,r,m)
        samples={lo,hi,(lo+hi)/2}|{F(2*k+1,2*m) for k in range(m) if lo<=F(2*k+1,2*m)<=hi}
        for t in samples:
            truth=known_terms(a,b,t,m)
            assert all(bounds[k].contains(v) for k,v in truth.items());checks+=1
    records['quantized_shared_loss_intervals']={'independent_exact_truth_points':checks}
    blendchecks=0
    colors=[0,0xffff,0xf800,0x001f,0x07e0,0x8410]
    for dst,src,q in itertools.product(colors,colors,range(256)):
        assert blend(dst,src,q)==independent_blend(dst,src,q);blendchecks+=1
    states=sequential(0,[(0xffff,I(F(1,2)-F(1,10**6),F(1,2)))])
    assert states=={0x7bef,0x8410}
    cartesian={int(r)*2048+int(g)*32+int(b) for r,g,b in itertools.product([15,16],[31,32],[15,16])}
    assert len(cartesian)==8 and len(states)==2
    assert sequential(0,[(0xffff,I(0,1))],state_limit=4) is None
    records['ordered_integer_blend']={'independent_exact_blends':blendchecks,'ambiguous_alpha_codes':[127,128],'packed_shared_alpha_outcomes':sorted(states),'independent_channel_product_outcomes':8,'state_budget_exhaustion':'unknown; never truncated'}
    # Independently known four equal subpixel atoms, not implementation primitives.
    def opaque(first,second):
        result=[]
        for i in range(4):result.append((0,0,1) if i in second else (1,0,0) if i in first else (0,0,0))
        return tuple(sum(F(c[j],4) for c in result) for j in range(3))
    coincident=opaque({0,1},{0,1});disjoint=opaque({0,1},{2,3})
    assert coincident==(0,0,F(1,2)) and disjoint==(F(1,2),0,F(1,2))
    seq=sequential(0,[(0xf800,I(F(1,2),F(1,2))),(0x001f,I(F(1,2),F(1,2)))])
    assert seq=={0x4010} and sequential(0,[(0x001f,I(F(1,2),F(1,2))),(0xf800,I(F(1,2),F(1,2)))])=={0x8008}
    records['opaque_vs_sequential_overlap']={'coincident':list(map(str,coincident)),'disjoint':list(map(str,disjoint)),'same_feature_coverages_different_continuous_targets':True,'sequential_red_then_blue':'0x4010','reversed':'0x8008'}
    assert gap_status(I(1,1),I(0,FLOOR))=='floor_limited'
    assert gap_status(I(1,1),I(FLOOR-F(1,10**6),FLOOR+F(1,10**6)))=='inconclusive_denominator'
    assert gap_status(I(0,0),I(FLOOR+F(1,10**6),1))=='pass_point_target'
    records['denominator_floor']={'strictly_greater_than':'10/1024','equality':'floor_limited','straddling':'inconclusive','20percent_equality':'pass only if denominator proved eligible'}
    assert support(I(0,F(1,10**9)),1)=='uncertain' and support(ZERO,1)=='excluded' and support(I(0,1),0)=='excluded'
    assert visibility_counts(1,0,0,1)['fraction']==I(F(1,2),1)
    assert visibility_counts(0,0,1,0)=={'fraction':I(1,1),'undefined_possible':True}
    assert visibility_counts(0,0,0,1)=={'fraction':ZERO,'undefined_possible':True}
    assert support_mae([ZERO],[I(1,1)])['interval']==I(0,1)
    c=F(394,765);p=peak_bounds([c],[I(1,1)],['eligible'],[F(1)])
    assert p['candidate_peak']==p['retention']==I(c,c) and p['reference_peak']==I(1,1)
    assert peak_bounds([F(0)],[ZERO],['excluded'],[F(0)])['retention'] is None
    assert peak_bounds([F(1)],[I(0,F(1,1000))],['uncertain'],[F(1)])['undefined_retention_possible']
    # Visibility >=.1 includes exact ties; a straddling interval has uncertain eligibility.
    assert I(F(1,10),F(1,10)).lo>=F(1,10)
    assert I(F(99,1000),F(101,1000)).lo<F(1,10)<=I(F(99,1000),F(101,1000)).hi
    records['support_peak_visibility']={'zero_contrast_retention':None,'0x8410_reference_white_peak_retention':str(c),'tiny_positive_support':'uncertain, not discarded','definite_hit_plus_optional_miss_fraction':'[1/2,1]','reference_10percent_tie':'eligible','straddling_10percent':'uncertain'}
    # A very narrow shared reference enclosure flips the20% target despite zero bootstrap spread.
    mid=F(3,20);eps=F(1,10**6);a=F(0);dmid=known_terms(a,0,mid,31)['D'];b=TARGET*dmid+eps/10;r=I(mid-eps,mid+eps)
    truthmid=known_terms(a,b,mid,31);truthhigh=known_terms(a,b,r.hi,31);bounds=quantized_terms(a,b,r,31)
    assert truthmid['H']>0 and truthhigh['H']<0 and bounds['D'].lo>FLOOR
    assert gap_status(bounds['H'],bounds['D'])=='inconclusive_target'
    g=quotient(bounds['N'],bounds['D']);point=truthmid['N']/truthmid['D']
    env=bootstrap_envelope([(bounds['D'],g)]*100)
    assert env['interval'].lo<TARGET<point and env['interval'].hi>point
    records['midpoint_false_pass']={'reference_radius':str(eps),'midpoint_gap':str(point),'true_high_reference_gap':str(truthhigh['N']/truthhigh['D']),'midpoint_identical_16_units_percentile_interval':[str(point),str(point)],'certified_numeric_envelope':[str(env['interval'].lo),str(env['interval'].hi)],'robust_status':gap_status(bounds['H'],bounds['D'])}
    # Local guard: exact equality allowed, but plausible reference shifts violate it.
    c=F(132,255);mid=c*F(10,21);r=I(mid-F(1,1000),mid+F(1,1000));ea=mae([0],[r]);delta=difference([c],[0],[r]);status,residual=local_status(ea,delta)
    assert status=='inconclusive' and abs(c-mid)-mid==mid/10
    assert abs(c-r.lo)-r.lo>r.lo/10
    records['local_midpoint_false_pass']={'midpoint_guard':'equality passes','plausible_lower_reference':'violation','interval_status':status,'thresholds':['10% baseline','2/1024']}
    # Exactly5% undefined remains allowed; ambiguous classification is never guessed.
    finite=(I(1,1),I(F(1,4),F(1,3)));undefined=(ZERO,None);maybe=(I(0,1),None)
    assert bootstrap_envelope([finite]*19+[undefined])['status']=='conditional_sensitivity_envelope'
    assert bootstrap_envelope([finite]*18+[undefined]*2)['status']=='inconclusive_floor'
    assert bootstrap_envelope([finite]*19+[maybe])['status']=='inconclusive_definedness'
    records['undefined_replicates']={'exact_5percent':'conditional defined-subset interval','above_5percent':'inconclusive','possible_undefined':'inconclusive_definedness; no silent exclusion'}
    # Sixteen heterogeneous paired geometry IDs; two latent endpoint errors shared across styles.
    schedule=[[rng.randrange(16) for _ in range(16)] for _ in range(64)]
    bases=[[F(3,20)+F(i,1000),F(7,20)+F(i,2000)] for i in range(16)]
    candidates=[[F([8,16,24,33][i%3+style],255) for style in range(2)] for i in range(16)]
    reps=[]
    for ids in schedule:
        styles=[]
        for style in range(2):
            tt=[quantized_terms(0,candidates[i][style],I(bases[i][style]-eps,bases[i][style]+eps),31) for i in ids]
            n=sum((x['N'] for x in tt),ZERO).scale(F(1,16));d=sum((x['D'] for x in tt),ZERO).scale(F(1,16));styles.append((d,quotient(n,d)))
        reps.append(styles)
    world_checks=0
    for style in range(2):
        envelope=bootstrap_envelope([x[style] for x in reps])['interval']
        for deltas in itertools.product([-eps,eps],repeat=2):
            values=[]
            for ids,rep in zip(schedule,reps):
                truth=[known_terms(0,candidates[i][style],bases[i][style]+deltas[i%2],31) for i in ids]
                value=sum(t['N'] for t in truth)/sum(t['D'] for t in truth)
                assert rep[style][1].contains(value);values.append(value);world_checks+=1
            assert envelope.lo<=quantile(values,F(1,40))<=quantile(values,F(39,40))<=envelope.hi
    # Heterogeneous interval endpoints test quantile monotonicity independently by all2^6 worlds.
    intervals=[I(F(i,10),F(i+2,10)) for i in [0,1,2,3,4,5]];bound=bootstrap_envelope([(I(1,1),v) for v in intervals])['interval']
    for world in itertools.product(*[(v.lo,v.hi) for v in intervals]):
        assert bound.lo<=quantile(world,F(1,40))<=quantile(world,F(39,40))<=bound.hi
    records['statistics']={'paired_geometry_units':16,'styles':2,'explicit_synthetic_schedules':64,'not_frozen_numpy_PCG_schedule':True,'shared_reference_not_resampled_as_noise':True,'heterogeneous_paired_reference_world_checks':world_checks,'quantile_endpoint_worlds_exhausted':64,'meaning':'numerical envelope of a fixed percentile functional, not95% population coverage certification'}
    records['resources']={'cpu_seconds':time.process_time()-start,'peak_rss_kib':resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,'no_geographic_inputs':True}
    (out/'synthetic-controls.json').write_text(json.dumps(records,indent=2));print(json.dumps(records,indent=2))
if __name__=='__main__':run(Path(sys.argv[1]))
