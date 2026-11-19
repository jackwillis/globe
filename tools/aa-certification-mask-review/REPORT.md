# PE-AA-01 certification review corrections and masked uncertainty — v1

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
1728subsets/180adversarial randomized cases. Exactly5%undefined is allowed;
above5%, a straddling threshold or uncertain original-point denominator is inconclusive. Nonintegral
5%counts, ties, paired16-ID/two-style ratio-of-means, shared-latent worlds and a2000draw/100ambiguous
stress case pass. It optimizes across ALL optional subset sizes using order-statistic monotonicity,
not one favorable subset. O(nlog n+m log n), O(n)storage, max100000supplied draws; actual tests are
synthetic, not geographic bootstrap. Joint feasibility is conservatively relaxed, explicitly
flagged: some optional combinations can be impossible under a common reference. No independent
reference-noise model is introduced. See SUBSET-METHOD.md for proof/limits.

## Literal frozen mask and exact measurements

Original `quality_only.neighborhood` was invoked directly, twice with byte-identical output.
Source SHA256 `30b87445f821bfd1d58cb6a200fdda60cc77ff0b9437b035a42a103ada728dea`;
geometry SHA256 `5bf4f7e97ca78b4011f581990f803d4027060147fdce268b3ad61c9c186d3bc2`.
Pinned Python3.14.8/NumPy2.5.3/Linuxx86-64 runtime and oracle_rows/reference source hashes are retained.
No idealized replacement mask, background image, geographic A/B output or new integration was used.

- Output:640×480,row-major uint8,0excluded/1included,no header,**307,200bytes**.
- Mask SHA256 **`68c4c2ff39983467819eb8f4d06a6e04713c946431b1f7e08b5bc6f0bb23fcd7`**.
- Included pixels **74,796 /307200 = 24.347656%**.
- Exact sum of in-mask channel width numerators **58615350941568**,
  common channel denominator **1095216660480**.
- Exact masked mean midpoint-error bound **u_M=152644143077/1279966020894720 ≈ 0.00011925640258**.
- Full-frame u=152644143077/5257039970304000≈2.90361389564e-05; masked/full ratio
  **25600/6233 ≈ 4.107172×**.
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
|min|0|
|p25|0|
|median|0|
|p75|0.000285668466605|
|p95|0.000330029126086|
|p99|0.000363568075342|
|max|0.000390490363626|

**43,178masked pixels have zero width**. Maximum pixel-channel radius is1/2560.
The top1%,5%,10%masked pixels contain respectively
3.117%,
14.688% and
28.092%of the width sum.
27,199pixels (36.364%of the mask) account for90%.

| Largest uncertainty contributions: tile(x,y) | Mask pixels | Tile u_M | Share total width |
|---|---:|---:|---:|
|(39,29)|64|0.000284364888|0.2040%|
|(34,56)|64|0.000276215722|0.1982%|
|(65,22)|64|0.000272255461|0.1953%|
|(59,36)|64|0.000269841021|0.1936%|
|(40,29)|64|0.000266743544|0.1914%|

| Most costly retained reference tiles | Historical reference CPU(s) | Masked tile u_M |
|---|---:|---:|
|(58,49)|5.023884|0.000261276974|
|(62,14)|4.092941|0.000179989895|
|(13,35)|3.911683|0.000167835177|

Rows224–239 have the largest16-row-band width share (~7.17%). Full30band records,4800tile records and per-mask-pixel integer widths are retained. Width-heavy and CPU-heavy regions differ; existing CPU is descriptive reference computation, not a new renderer benchmark. No region was excluded or chosen for a new run.

## Candidate-independent decision-resolution illustrations

For any fixed candidate, midpoint MAE deviation≤u_M. Shared A−B deviation≤2u_M=
**0.000238512805161**. For the original local guard, a safe generic
midpoint residual deviation is≤2.1u_M=**0.000250438445419**;
actual guard/outcome is unknown without candidates.

For once-quantized floor loss E_Q(R), exact quantizer-cell extrema give a reference-only midpoint
deviation e_Q≤0.000136797366028.
Since H=.8E_A−E_B+.2E_Q, its generic midpoint deviation is bounded by
**B_H=1.8u_M+.2e_Q=0.00024202099785**.
D=E_A−E_Q has B_D=u_M+e_Q=0.000256053768608.
These are conservative upper bounds for every candidate, not measured H,D,G or regression scores.

The review's2.4u illustration is safe for a FIXED control, but not universally for changing Q(R):
bit-replicated code expansion can make floor loss jump at ideal quantizer ties. An exact synthetic
tie counterexample and282endpoint controls validate why the actual illustration includes e_Q.
Here2.4u_M=0.000286215366193happens to be looser than B_H;
that empirical fact is not a universal Lipschitz proof. Unique quantizer codes are not required.

| Hypothetical denominator | Conservative illustration | Eligibility/result |
|---|---|---|
| D exactlyτ=10/1024=.009765625 | Equality to floor | Undefined; NO percentage verdict |
| Midpoint D=τ, uncertainty B_D | [0.00950957123139,0.0100216787686] | Straddles floor: inconclusive |
| D known exactly2τ=.01953125 | B_H/(2τ)≈0.0123914751 | Residual resolution scale, not an actual gap |
| Midpoint D=2τ, uncertainty B_D | [0.0192751962314,0.0197873037686] | Lower bound>τ, eligible in this hypothetical case |

Using that last lower denominator gives B_H/D.lower≈0.0125560848
(~1.256percentage points) as a conservative threshold-resolution
illustration. B_H/τ≈0.0247829502is scale-only:τ itself is NOT eligible.
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
