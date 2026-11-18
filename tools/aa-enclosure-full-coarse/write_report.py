#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Render feasibility report from retained numerical/resource evidence, never candidates."""
import json,sys,hashlib
from pathlib import Path
p=Path(sys.argv[1]);a=json.loads((p/'analysis.json').read_text());r=json.loads((p/'resource-accounting.json').read_text());d=json.loads((p/'cost-diagnostics.json').read_text());f=json.loads((p/'source-freeze.json').read_text());setup=json.loads((p/'setup.json').read_text());cont=json.loads((p/'setup-continuation.json').read_text());pred=a['old_prediction'];t=a['totals'];done=a['completed_tiles']==4800;go=done and a['precision_limited_tiles']==0
z=json.loads((p/'conditional-projection.json').read_text())
source=Path(__file__).parent
h=lambda n:hashlib.sha256((source/n).read_bytes()).hexdigest()
fmt=lambda x:f'{x:,.3f}'
spreads=sorted(d['same_density_spread'],key=lambda q:q['max_cpu']/max(q['min_cpu'],1e-9),reverse=True)[:3]
text=f'''# PE-AA-01 one coarse full-fixture feasibility v1

2026-11-18. Baseline `7ab358f2ca6006f65d6bdea41acecc8de8d8791d`.
**Recommendation: {"GO — further development toward a conservative full-reference pipeline is justified" if go else "REVISE — resolve resource-accounting gaps and measure the unstarted tail"}.**
This is candidate-blind feasibility research, not a quality comparison or protocol amendment.

## Measured outcome

Exactly `view5-r220-relief`, 640×480, all 4,800 8×8 tiles selected before execution,
row-major index `80*ty+tx`. Target midpoint radius `1/2560 = 0.000390625`.
This is four times the historical fine radius `1/10240`; it is insufficient for that fine quality decision.

| Final state | Count |
|---|---:|
| Completed 64-pixel tile certificates |{a['completed_tiles']}|
| All pixels at coarse precision |{a['precision_attained_tiles']}|
| Completed but precision-limited tiles |{a['precision_limited_tiles']}|
| Currently interrupted tiles |{a['interrupted_tiles']}|
| Unstarted tiles |{a['unstarted_tiles']}|

Pixel states: `{json.dumps(a['pixel_states'],sort_keys=True)}`.
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
| Sum completed-tile CPU (comparable to old retained-index cost model) |{fmt(t['cpu'])} s|
| Sum completed-tile wall |{fmt(t['wall'])} s|
| First geometry/read/BVH setup CPU |{fmt(setup['cpu_seconds'])} s|
| Recovery geometry/read/BVH setup CPU |{fmt(cont['cpu_seconds'])} s|
| Continuation worker CPU from parent rusage |{fmt(r['continuation_worker_cpu_rusage'])} s|
| Continuation supervisor CPU |{fmt(r['continuation_supervisor_cpu'])} s|
| Preparation control CPU |{fmt(r['preparation_control_child_cpu'])} s|
| Recorded postprocessing CPU |{fmt(r['recorded_postprocessing_cpu'])} s|
| Maximum recorded completed-tile RSS |{r['observed_max_completed_tile_rss_kib']:,} KiB|
| Evidence bytes at accounting snapshot |{r['current_evidence_bytes']:,}|

The first worker's terminal rusage is unavailable. Retained work establishes its CPU lower bound
{fmt(r['phase1_worker_cpu_lower_bound_from_retained_work'])} s; an explicitly conservative
elapsed-based upper accounting bound is {fmt(r['phase1_worker_cpu_conservative_upper_bound'])} s
for this single-thread reference work. Total worker CPU is therefore bounded in accounting by
{fmt(r['worker_cpu_upper_bound'][0])}–{fmt(r['worker_cpu_upper_bound'][1])} s; it is not an exact measurement.
The known CPU component sum is at least {fmt(r['known_cpu_component_sum_lower_bound'])} s.
With the reserved 300 s preparation/reporting allowance, the accounting envelope is
{fmt(r['total_cpu_upper_bound_with_300s_preparation_reporting_allowance'])} s, below the owner ceiling1,800 s.
The allowance covers unmetered initial read-only/tool setup, first-supervisor overhead and final
packaging; it must not be presented as CPU actually consumed. First interrupted peak RSS is not
recorded; maximum recorded RSS is not a claim of exact global peak.

Elapsed since the recorded budget start at accounting is {fmt(r['elapsed_since_budget_start'])} s;
including the 330 s initial/reporting allowance gives {fmt(r['conservative_elapsed_with_330s_initial_reporting_allowance'])} s,
below2,700 s. Reference workers/supervisors enforce384MiB address space; one worker at a time.
Initial worker CPUsoft1445/hard1450; recovery worker soft995/hard1000/clean990, with unchanged
absolute elapsed guard. The reduction preserves the total envelope, not a fresh1,800 s budget.
Controls' virtual-address-space peaks were not separately logged; recorded control RSS is retained.
Output guards900/950MiB leave space below1GiB; final archive/payload sizes are in evidence hashes.
These are host reference resource observations, never renderer benchmarks or G4 acceptance.

The final stop was the predeclared recovery clean guard990CPU seconds, not an observed owner1800CPU ceiling breach. First-worker hard accounting uncertainty and the conservative reserve were NOT spent again to finish the tail. Historical interrupted tile count1 (recovered); current interrupted count0. Final unstarted pixels42,368 remain unknown, not certified background.

## Enclosure and discrete outcomes

Total geometric predicate calls: {t['predicates']:,}; initial classification calls:
{t['initial_predicates']:,}, initial classification CPU {fmt(t['initial_cpu'])} s.
BVH box checks: {t['bboxes']:,}; cell visits: {t['cells']:,}; maximum depth: {a['max_depth']}.
Maximum pixel midpoint radius: {a['max_radius']:.12g}; all-completed-pixel mean channel radius:
{a['mean_midpoint_radius']:.12g}. Unresolved color-boundary area sums to
{a['unknown_area_pixels']:.12g} pixel areas and remains fully charged to the bounds.
Exact per-pixel numerators, unresolved leaves/area, cells, predicates, time and states are retained.

RGB565-ambiguous pixels: {t['ambiguous_pixels']:,}; ambiguous channels: {t['ambiguous_channels']:,}.
Channel outcome ranges are conservative supersets and need not be jointly achievable.
These discrete ambiguities are not small continuous errors or a failed renderer finding.
All-frame mean uncertainty includes empty pixels and is not a stroke-mask quality measure.
Any future aggregate MAE/difference claim needs an independently frozen geometry-fixed mask and
proper uncertainty propagation. No candidate metric was calculated.

## Comparison with the pre-existing model

The held-out fixture's old coarse prediction was {fmt(pred['coarse_cpu_seconds_central'])} CPU s,
with scenario{fmt(pred['coarse_cpu_seconds_scenario_min'])}–{fmt(pred['coarse_cpu_seconds_scenario_max'])} s,
not a confidence interval. The measured completed-tile sum is {z['measured_prefix_to_old_prefix']:.6f}×
the old prediction for the SAME completed prefix ({z['old_prefix_prediction_style_median']:.3f} s).
It is already above the old full-fixture point prediction, but this is not a measured full frame. Generation setup, failed partial work, serialization/manifest
and supervisor overhead are separate; an exact end-to-end worker total is unavailable after interruption.

There are1,626 nonempty tiles and19,026 primitives; maximum bbox density121, including three tiles
beyond the prior style support. `cost-diagnostics.json` retains orientation/feature/color box
counts and same-density CPU spread, without changing tile selection or numerical work.
Descriptive nonempty-tile CPU correlations are `{json.dumps(d['descriptive_pearson_cpu_correlations'],sort_keys=True)}`.
These descriptors are not causal identification of orientation, overlap, background contrast or
opaque source ordering. Source ordering was preserved, not experimentally varied.
The observed initial predicate count demonstrates spatial pruning; no exhaustive-scan speedup
benchmark was run. Piecewise interpolation at only five selected anchors cannot encode phase,
edge orientation, overlap/occlusion or color contrast. Such effects are explanations to investigate,
not measured causal shares of a discrepancy. Per-density groups and expensive tiles are retained. The most expensive completed tile is(58,49), density94, CPU5.024s, nine source features and three colors; the maximum-density tile(13,30), density121, costs3.534s with seven features. Thus maximum density is not maximum cost. Near-axis/near-diagonal descriptors and feature overlaps correlate differently with cost, but source ordering and background contrast were not varied or independently isolated.

Conditional **coarse-only** 32-condition projections, excluding future sidecar/sequential/scoring work:
- Original model: {a['coarse_32_original_hours']:.6f} CPU h.
- Conditional full-fixture tile CPU: {z['conditional_full_tile_cpu']:.3f} s, estimated from
  the measured prefix plus old remaining-tail prediction scaled by the prefix ratio.
- Replace only held-out fixture with this CONDITIONAL full estimate; leave others unchanged:
  {z['conditional_32_coarse_hours_replace_only_heldout']:.6f} CPU h.
- Transfer that conditional held-out/full-model ratio to all Relief predictions; leave Map unchanged:
  {z['conditional_32_coarse_hours_transfer_relief']:.6f} CPU h.

Remaining tail: {z['remaining_tiles']} tiles ({z['remaining_nonempty']} nonempty, {z['remaining_empty']} empty).
The model's per-curve tail scenarios give {z['conditional_full_cpu_scenario_range'][0]:.3f}–{z['conditional_full_cpu_scenario_range'][1]:.3f} s
for full tile CPU; this narrow range is NOT a confidence interval or robust uncertainty bound.
Row-major censoring leaves a structurally different tail; prefix-factor transfer can be wrong.

The Relief-wide transfer is an explicit sensitivity scenario, not evidence that every Relief condition
has this factor. One censored held-out fixture cannot establish a population CI or guarantee.
No fine-frame cost/convergence claim is upgraded; the scale-v1 fine failures remain unresolved.
No arbitrary multiplier estimates isolated-feature or sequential passes: their coverages, alpha
rounding/correlation and discrete compositing need distinct instrumentation and budgets.

## Recommendation and prerequisites

{"GO applies only to further pipeline development. Before any new reference generation, steering should review the operational repair, accounting gap and source/certificate validation; specify a geometry-fixed uncertainty contract and discrete RGB565/sequential outcome policy; and approve a separately versioned execution budget. No further geographic run is started or implicitly approved." if go else "REVISE: make the host supervisor exception-safe with a continuously retained aggregate CPU ledger, preserving the resource scope and numerical engine; then seek steering authorization for a narrowly budgeted continuation of the remaining662 tiles. The central cost model underpredicted the measured prefix by19.46%, and the current reduced operational guard stopped before the owner ceiling. This does not establish that the fixture cannot fit in1800 CPU seconds. Fix accounting and measure the tail before any broader generation. No continuation is started here."}
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
'''
for n,v in f['inputs_sha256'].items():text+=f'| `{n}` | `{v}` |\n'
text+='\nScale-v1 engine SHA256 `06f2cda0efd4649426b4928908600e0a6fb80de56d70ab9fbfd0f1b98cca2f49`.\n'
text+='Retained adapter SHA256 `'+h('retained.py')+'`.\n'
text+='\nExact commands are in README.md/EXECUTION.md. The incident requires the two frozen orchestration versions, not pretending the repaired version produced initial observations. analyze.py, geometry_diagnostics.py, cost_diagnostics.py, account_resources.py and write_report.py are postprocessing additions; numerical producer hashes are independently retained. Reproduction requires a fresh directory and the archived geometry/background/reference-bound inputs. Commands are evidence, not authorization for another run. Issue#25 is the restart record. Stop for steering review.\n'
(source/'REPORT.md').write_text(text)
