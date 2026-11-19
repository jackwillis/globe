#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,sys
from pathlib import Path
from fractions import Fraction as F
out=Path(sys.argv[1]);r=json.loads((out/'masked-audit.json').read_text());t=json.loads((out/'review-controls.json').read_text());val=lambda k:float(F(r[k]));src=Path(__file__).parent
text=f'''# PE-AA-01 certification review corrections and masked uncertainty — v1

2026-11-18. Follow-up to4c8bfc74; candidate-blind existing reference63016edb.
**Recommendation: enough evidence for steering to approve a minimal versioned certification
amendment, with the explicit semantic choices in AMENDMENT-DECISION.md. Nothing is adopted here.**
All100lines of Claude's supplied independent certification review were read before editing.

## Corrections and controls

All three targeted mutants fail before the corrected suite passes:

| Mutation | Independent failing control |
|---|---|
| Remove absolute2/1024local threshold | A error0,Δ=1/1024 must pass, with exact residual−1/1024 |
| Bootstrap denominator `<=floor` becomes `<floor` |20denominators exactly10/1024 must all count undefined |
| Exclude uncertain support from possible peak | Definite candidate contrast1/5 plus optional9/10 must bound peak[1/5,9/10] |

Exact tightness controls require shared error difference[−1/2,1/2] and once-quantized D exactly8/85
on R∈[1/10,11/100], rather than merely containing those results. All original synthetic controls
also pass unchanged. Mutant sources and failing-control labels are preserved; original proposal
and numeric implementation remain unchanged.

The new bounded-subset percentile operator matches exact independently enumerated endpoints over
{t['exhaustive_membership_subsets']}subsets/{t['subset_random_cases']}adversarial randomized cases. Exactly5%undefined is allowed;
above5%, a straddling threshold or uncertain original-point denominator is inconclusive. Nonintegral
5%counts, ties, paired16-ID/two-style ratio-of-means, shared-latent worlds and a2000draw/100ambiguous
stress case pass. It optimizes across ALL optional subset sizes using order-statistic monotonicity,
not one favorable subset. O(nlog n+m log n), O(n)storage, max100000supplied draws; actual tests are
synthetic, not geographic bootstrap. Joint feasibility is conservatively relaxed, explicitly
flagged: some optional combinations can be impossible under a common reference. No independent
reference-noise model is introduced. See SUBSET-METHOD.md for proof/limits.

## Literal frozen mask and exact measurements

Original `quality_only.neighborhood` was invoked directly, twice with byte-identical output.
Source SHA256 `{r['mask_source_sha256']['quality_only.py']}`;
geometry SHA256 `{r['geometry_sha256']}`.
Pinned Python3.14.8/NumPy2.5.3/Linuxx86-64 runtime and oracle_rows/reference source hashes are retained.
No idealized replacement mask, background image, geographic A/B output or new integration was used.

- Output:640×480,row-major uint8,0excluded/1included,no header,**{r['mask_bytes']:,}bytes**.
- Mask SHA256 **`{r['mask_sha256']}`**.
- Included pixels **{r['mask_pixels']:,} /307200 = {100*r['mask_pixels']/307200:.6f}%**.
- Exact sum of in-mask channel width numerators **{r['masked_width_numerator']}**,
  common channel denominator **{r['denominator']}**.
- Exact masked mean midpoint-error bound **u_M={r['u_mask']} ≈ {r['u_mask_display']:.12g}**.
- Full-frame u={r['u_full']}≈{r['u_full_display']:.12g}; masked/full ratio
  **{r['masked_over_full_ratio']} ≈ {float(F(r['masked_over_full_ratio'])):.6f}×**.
- Outside-mask width sum **0**: all already-retained uncertainty is inside this literal mask.
  This is a reference observation, not an A/B error or benefit.

The initial audit failed after arithmetic because platform.platform tried a lazy subprocess-module
import through the strict input guard. Initial source/error/mask hash are preserved; pre-resolving
platform metadata before the guard fixes that harness issue. Final mask matches the initial hash.
Both attempts read only existing certificates/geometry; no reference generation or candidate access.

## Distribution and spatial concentration

Per-pixel mean channel midpoint-radius distribution, including exact-zero masked pixels:

| Quantile | Radius |
|---|---:|
'''
for name in ['min','p25','median','p75','p95','p99','max']:text+=f"|{name}|{float(F(r['distribution']['mean_channel_radius'][name])):.12g}|\n"
text+=f'''
**{r['distribution']['zero_width_mask_pixels']:,}masked pixels have zero width**. Maximum pixel-channel radius is1/2560.
The top1%,5%,10%masked pixels contain respectively
{100*float(F(r['concentration']['top1pct']['fraction_of_masked_width'])):.3f}%,
{100*float(F(r['concentration']['top5pct']['fraction_of_masked_width'])):.3f}% and
{100*float(F(r['concentration']['top10pct']['fraction_of_masked_width'])):.3f}%of the width sum.
{r['pixels_for90pct_masked_width']:,}pixels ({100*r['pixels_for90pct_masked_width']/r['mask_pixels']:.3f}%of the mask) account for90%.

| Largest uncertainty contributions: tile(x,y) | Mask pixels | Tile u_M | Share total width |
|---|---:|---:|---:|
'''
for v in r['top_uncertainty_tiles'][:5]:text+=f"|({v['tx']},{v['ty']})|{v['mask_pixels']}|{float(F(v['masked_mean_radius'])):.9g}|{100*float(F(v['share_masked_width'])):.4f}%|\n"
text+='\n| Most costly retained reference tiles | Historical reference CPU(s) | Masked tile u_M |\n|---|---:|---:|\n'
for v in r['top_existing_reference_cost_tiles'][:3]:text+=f"|({v['tx']},{v['ty']})|{v['reference_cpu_seconds']:.6f}|{float(F(v['masked_mean_radius'])):.9g}|\n"
text+='\nRows224–239 have the largest16-row-band width share (~7.17%). Full30band records,4800tile records and per-mask-pixel integer widths are retained. Width-heavy and CPU-heavy regions differ; existing CPU is descriptive reference computation, not a new renderer benchmark. No region was excluded or chosen for a new run.\n'
text+=f'''
## Candidate-independent decision-resolution illustrations

For any fixed candidate, midpoint MAE deviation≤u_M. Shared A−B deviation≤2u_M=
**{val('paired_midpoint_deviation_bound'):.12g}**. For the original local guard, a safe generic
midpoint residual deviation is≤2.1u_M=**{val('local_guard_deviation_bound'):.12g}**;
actual guard/outcome is unknown without candidates.

For once-quantized floor loss E_Q(R), exact quantizer-cell extrema give a reference-only midpoint
deviation e_Q≤{val('once_quantized_floor_loss_midpoint_deviation_bound'):.12g}.
Since H=.8E_A−E_B+.2E_Q, its generic midpoint deviation is bounded by
**B_H=1.8u_M+.2e_Q={val('point_H_midpoint_deviation_bound_including_quantization'):.12g}**.
D=E_A−E_Q has B_D=u_M+e_Q={val('D_midpoint_deviation_bound'):.12g}.
These are conservative upper bounds for every candidate, not measured H,D,G or regression scores.

The review's2.4u illustration is safe for a FIXED control, but not universally for changing Q(R):
bit-replicated code expansion can make floor loss jump at ideal quantizer ties. An exact synthetic
tie counterexample and{t['quantizer_jump_H_checks']}endpoint controls validate why the actual illustration includes e_Q.
Here2.4u_M={val('fixed_control_loose_H_illustration_2point4u'):.12g}happens to be looser than B_H;
that empirical fact is not a universal Lipschitz proof. Unique quantizer codes are not required.

| Hypothetical denominator | Conservative illustration | Eligibility/result |
|---|---|---|
| D exactlyτ=10/1024=.009765625 | Equality to floor | Undefined; NO percentage verdict |
| Midpoint D=τ, uncertainty B_D | [{float(F(r['floor_midpoint_D_interval'][0])):.12g},{float(F(r['floor_midpoint_D_interval'][1])):.12g}] | Straddles floor: inconclusive |
| D known exactly2τ=.01953125 | B_H/(2τ)≈{val('H_bound_over_exact_twice_floor'):.9g} | Residual resolution scale, not an actual gap |
| Midpoint D=2τ, uncertainty B_D | [{float(F(r['twice_floor_midpoint_D_interval'][0])):.12g},{float(F(r['twice_floor_midpoint_D_interval'][1])):.12g}] | Lower bound>τ, eligible in this hypothetical case |

Using that last lower denominator gives B_H/D.lower≈{val('H_bound_over_twice_floor_conservative_lower_D'):.9g}
(~{100*val('H_bound_over_twice_floor_conservative_lower_D'):.3f}percentage points) as a conservative threshold-resolution
illustration. B_H/τ≈{val('H_bound_over_floor_scale_only_not_eligible'):.9g}is scale-only:τ itself is NOT eligible.
The20%point criterion is unchanged. No actual A/B gap, point verdict, local outcome or native
legibility conclusion is inferred. This one fixture cannot resolve a16geometry/statistical verdict.

## Decision record and stop

AMENDMENT-DECISION.md is the minimal proposed record: valid enclosure versus operational precision
versus decision resolution; exact endpoint/quantizer semantics and floating mask provenance;
original point20%, strict floor and local guards (baseline=A mask MAE); separate bootstrap/sequential
reports; diagnostic support/peak/visibility eligibility. It does not introduce a lower-bootstrap-bound
acceptance gate. Exact endpoint/quantizer adoption, resolution-target choice, support diagnostic
meaning and a future interval-input format need explicit owner sign-off; none is decided implicitly.

Steering has enough evidence to approve that narrow versioned amendment with those choices recorded.
Approval would not authorize new reference computation, scoring, fine frames, candidate inspection,
B tuning, G4, production, merge or release. Original experiment/proposal/evidence remain unchanged.
No C++/hardware tests are relevant to this synthetic/read-only Python follow-up; none was run.
'''
(src/'REPORT.md').write_text(text)
