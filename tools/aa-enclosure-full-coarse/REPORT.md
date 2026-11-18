# PE-AA-01 one coarse full-fixture feasibility v1

2026-11-18. Baseline `7ab358f2ca6006f65d6bdea41acecc8de8d8791d`.
**Recommendation: REVISE — resolve resource-accounting gaps and measure the unstarted tail.**
This is candidate-blind feasibility research, not a quality comparison or protocol amendment.

## Measured outcome

Exactly `view5-r220-relief`, 640×480, all 4,800 8×8 tiles selected before execution,
row-major index `80*ty+tx`. Target midpoint radius `1/2560 = 0.000390625`.
This is four times the historical fine radius `1/10240`; it is insufficient for that fine quality decision.

| Final state | Count |
|---|---:|
| Completed 64-pixel tile certificates |4138|
| All pixels at coarse precision |4138|
| Completed but precision-limited tiles |0|
| Currently interrupted tiles |0|
| Unstarted tiles |662|

Pixel states: `{"precision_reached": 264832}`.
Every retained certificate passes independent admission checks for identity, 64 pixel positions,
integer bound ordering/denominators, exact target inequality, quotas/depth, and rational RGB565
outcome ranges. SHA256 is verified for every completed payload. No difficult tile was excluded.

## Implementation and reliability

`retained.py` retains a scale-v1 Geometry/BVH across tiles; it calls the unchanged `Geometry.tile`.
Geometric semantics, primitive predicates, accepted coordinate model, source-feature order and opaque
last-writer colors are unchanged. Original analytical controls, six topology controls, 3,456 independent
rational point/color comparisons, 12 randomized integrated controls, three topology mutation kills,
domain and fair-budget checks passed before whole-fixture work. All original anchors were evaluated
twice at their original precision: all 512 pixel results reproduce their original integer bounds.
A synthetic mixed-tile reuse sequence also agrees with fresh-index evaluations.

Every pixel retains its own 100,000-cell quota, depth16 ceiling and 75/64-second share.
Failure yields a wider conservative interval or an explicitly unknown unfinished pixel, never
false precision. Numerical certification is conditional on the exact binary64-endpoint/width-one
stroke model and correct implementation/CPython integer arithmetic, not formal software verification
or certification of upstream projection/model uncertainty. Coarse success does not remedy the
historical fine quota failures or successive-grid certification defect.

## Interruption, repair and recovery

There was **one real operational interruption**. The initial supervisor raced an atomic rename while
statting `inflight.json.tmp` and raised FileNotFoundError. The worker was no longer running when
collected. Its terminal CPU/RSS/termination signal record is missing; none is invented.
There were 1,326 completed tile certificates and 48 completed pixel records of tile1326.
Original checkpoint/logs and incident transcription remain under `interruption1/`.

Before continuation, the disappearing-file race was fixed and new orchestration hashes frozen.
Every completed tile hash was checked and tiles0–1325 skipped. Only the interrupted tile was
recomputed, retaining its original partial checkpoint separately. All 48 previously completed
pixel bounds, precision states, cell counts and depths agree exactly after index reconstruction.
No completed tile was recomputed or counted twice. Five malformed-certificate admission mutations
and changed-byte hash rejection pass. A disappearing-temp control passes.

Completed tiles are gzip payloads written to a temporary path, fsync'd and atomically renamed;
then a hash record is appended/fsync'd to completed.jsonl. Inflight checkpoints always say
`complete:false`. A stale inflight file for a verified completed tile does not create an interruption.
Partial/tmp payloads never count as completed. Recovery was demonstrated in this actual run;
a generic resumable scorer or multi-fixture job service was not implemented.

## Resources

| Observation | Value |
|---|---:|
| Sum completed-tile CPU (comparable to old retained-index cost model) |1,090.660 s|
| Sum completed-tile wall |1,103.143 s|
| First geometry/read/BVH setup CPU |0.178 s|
| Recovery geometry/read/BVH setup CPU |0.213 s|
| Continuation worker CPU from parent rusage |990.369 s|
| Continuation supervisor CPU |38.537 s|
| Preparation control CPU |35.207 s|
| Recorded postprocessing CPU |3.258 s|
| Maximum recorded completed-tile RSS |54,632 KiB|
| Evidence bytes at accounting snapshot |15,672,701|

The first worker's terminal rusage is unavailable. Retained work establishes its CPU lower bound
103.651 s; an explicitly conservative
elapsed-based upper accounting bound is 336.331 s
for this single-thread reference work. Total worker CPU is therefore bounded in accounting by
1,094.020–1,326.700 s; it is not an exact measurement.
The known CPU component sum is at least 1,171.022 s.
With the reserved 300 s preparation/reporting allowance, the accounting envelope is
1,626.700 s, below the owner ceiling1,800 s.
The allowance covers unmetered initial read-only/tool setup, first-supervisor overhead and final
packaging; it must not be presented as CPU actually consumed. First interrupted peak RSS is not
recorded; maximum recorded RSS is not a claim of exact global peak.

Elapsed since the recorded budget start at accounting is 1,490.180 s;
including the 330 s initial/reporting allowance gives 1,820.180 s,
below2,700 s. Reference workers/supervisors enforce384MiB address space; one worker at a time.
Initial worker CPUsoft1445/hard1450; recovery worker soft995/hard1000/clean990, with unchanged
absolute elapsed guard. The reduction preserves the total envelope, not a fresh1,800 s budget.
Controls' virtual-address-space peaks were not separately logged; recorded control RSS is retained.
Output guards900/950MiB leave space below1GiB; final archive/payload sizes are in evidence hashes.
These are host reference resource observations, never renderer benchmarks or G4 acceptance.

The final stop was the predeclared recovery clean guard990CPU seconds, not an observed owner1800CPU ceiling breach. First-worker hard accounting uncertainty and the conservative reserve were NOT spent again to finish the tail. Historical interrupted tile count1 (recovered); current interrupted count0. Final unstarted pixels42,368 remain unknown, not certified background.

## Enclosure and discrete outcomes

Total geometric predicate calls: 149,199,903; initial classification calls:
114,058, initial classification CPU 4.599 s.
BVH box checks: 4,831,428; cell visits: 133,123,816; maximum depth: 13.
Maximum pixel midpoint radius: 0.000390625; all-completed-pixel mean channel radius:
3.04996364606e-05. Unresolved color-boundary area sums to
60.3051734865 pixel areas and remains fully charged to the bounds.
Exact per-pixel numerators, unresolved leaves/area, cells, predicates, time and states are retained.

RGB565-ambiguous pixels: 1,915; ambiguous channels: 2,002.
Channel outcome ranges are conservative supersets and need not be jointly achievable.
These discrete ambiguities are not small continuous errors or a failed renderer finding.
All-frame mean uncertainty includes empty pixels and is not a stroke-mask quality measure.
Any future aggregate MAE/difference claim needs an independently frozen geometry-fixed mask and
proper uncertainty propagation. No candidate metric was calculated.

## Comparison with the pre-existing model

The held-out fixture's old coarse prediction was 1,037.995 CPU s,
with scenario879.302–1,401.579 s,
not a confidence interval. The measured completed-tile sum is 1.194618×
the old prediction for the SAME completed prefix (912.978 s).
It is already above the old full-fixture point prediction, but this is not a measured full frame. Generation setup, failed partial work, serialization/manifest
and supervisor overhead are separate; an exact end-to-end worker total is unavailable after interruption.

There are1,626 nonempty tiles and19,026 primitives; maximum bbox density121, including three tiles
beyond the prior style support. `cost-diagnostics.json` retains orientation/feature/color box
counts and same-density CPU spread, without changing tile selection or numerical work.
Descriptive nonempty-tile CPU correlations are `{"cells": 0.9962409220495932, "complexity": 0.8643812563284237, "feature_count": 0.6417151421807719, "initial_predicates": 0.9150212732881585, "near_axis_segments": -0.0672170067429264, "near_diagonal_segments": 0.24669923447818076, "predicates": 0.9905112818937782}`.
These descriptors are not causal identification of orientation, overlap, background contrast or
opaque source ordering. Source ordering was preserved, not experimentally varied.
The observed initial predicate count demonstrates spatial pruning; no exhaustive-scan speedup
benchmark was run. Piecewise interpolation at only five selected anchors cannot encode phase,
edge orientation, overlap/occlusion or color contrast. Such effects are explanations to investigate,
not measured causal shares of a discrepancy. Per-density groups and expensive tiles are retained. The most expensive completed tile is(58,49), density94, CPU5.024s, nine source features and three colors; the maximum-density tile(13,30), density121, costs3.534s with seven features. Thus maximum density is not maximum cost. Near-axis/near-diagonal descriptors and feature overlaps correlate differently with cost, but source ordering and background contrast were not varied or independently isolated.

Conditional **coarse-only** 32-condition projections, excluding future sidecar/sequential/scoring work:
- Original model: 5.058424 CPU h.
- Conditional full-fixture tile CPU: 1240.008 s, estimated from
  the measured prefix plus old remaining-tail prediction scaled by the prefix ratio.
- Replace only held-out fixture with this CONDITIONAL full estimate; leave others unchanged:
  5.114538 CPU h.
- Transfer that conditional held-out/full-model ratio to all Relief predictions; leave Map unchanged:
  5.657212 CPU h.

Remaining tail: 662 tiles (134 nonempty, 528 empty).
The model's per-curve tail scenarios give 1229.775–1250.566 s
for full tile CPU; this narrow range is NOT a confidence interval or robust uncertainty bound.
Row-major censoring leaves a structurally different tail; prefix-factor transfer can be wrong.

The Relief-wide transfer is an explicit sensitivity scenario, not evidence that every Relief condition
has this factor. One censored held-out fixture cannot establish a population CI or guarantee.
No fine-frame cost/convergence claim is upgraded; the scale-v1 fine failures remain unresolved.
No arbitrary multiplier estimates isolated-feature or sequential passes: their coverages, alpha
rounding/correlation and discrete compositing need distinct instrumentation and budgets.

## Recommendation and prerequisites

REVISE: make the host supervisor exception-safe with a continuously retained aggregate CPU ledger, preserving the resource scope and numerical engine; then seek steering authorization for a narrowly budgeted continuation of the remaining662 tiles. The central cost model underpredicted the measured prefix by19.46%, and the current reduced operational guard stopped before the owner ceiling. This does not establish that the fixture cannot fit in1800 CPU seconds. Fix accounting and measure the tail before any broader generation. No continuation is started here.
The original scored experiment remains inconclusive; no numerical fidelity, visual preference,
production acceptance or adaptive rendering conclusion follows. Original scored sources33,
pilot sources9 and scale-v1 manifest entries9 verify unchanged. No production/G4 tests were run
because this is host-only Python research. No A/B image inspection, scoring, tuning, merge or release.

## Provenance and reproduction

Source hashes and both pre-execution freezes accompany the evidence; the initial orchestration
version is preserved in git and archive. Baseline treec313a92ce62ccde27814631f26bf42bca8211a04.
Python3.14.8, Linuxx86-64; decoder and enclosure use only stdlib. NumPy distribution metadata
is2.5.3, but NumPy is neither imported nor used by this reference path. A separate post-run
NumPy version probe under384MiB failed OpenBLAS allocation; its transcribed error is retained.
That environment-probe failure is not a geographic job failure and was not retried.

| Input | SHA256 |
|---|---|
| `view5-r220-relief.geometry` | `5bf4f7e97ca78b4011f581990f803d4027060147fdce268b3ad61c9c186d3bc2` |
| `view5-r220-relief-background.rgb565` | `7c015590e67af062f511baaec37114a2a478faff8a67988ed99bfe9dad760dcc` |

Scale-v1 engine SHA256 `06f2cda0efd4649426b4928908600e0a6fb80de56d70ab9fbfd0f1b98cca2f49`.
Retained adapter SHA256 `f28be04ca8244365318dc95814e0c1068cf82ffaa04abec7195d65641467606b`.

Exact commands are in README.md/EXECUTION.md. The incident requires the two frozen orchestration versions, not pretending the repaired version produced initial observations. analyze.py, geometry_diagnostics.py, cost_diagnostics.py, account_resources.py and write_report.py are postprocessing additions; numerical producer hashes are independently retained. Reproduction requires a fresh directory and the archived geometry/background/reference-bound inputs. Commands are evidence, not authorization for another run. Issue#25 is the restart record. Stop for steering review.
