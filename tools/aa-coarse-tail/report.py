#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,sys,hashlib
from pathlib import Path
out=Path(sys.argv[1]);a=json.loads((out/'final-validation.json').read_text());freeze=json.loads((out/'source-freeze.json').read_text());source=Path(__file__).parent;complete=a['completed_tiles']==4800
text=f'''# PE-AA-01 aggregate accounting repair and coarse tail v1

2026-11-18. Baseline8951cec392b13f3cff5c54574f876ca9e55734c0; numerical baseline7ab358f2 unchanged.
Recommendation: {"GO for a steering decision about the next certification-development slice; no further run is authorized" if complete else "REVISE the retained resource/failure limitation; no automatic attempt"}.
Candidate-blind feasibility only, not scoring, renderer acceptance or a protocol amendment.

## Recovery and final status

Original archive SHA256 `d198144721dc036020ac0332c25499eececb312e33fce76fd88d7eb58555649b`
and all4226 archived hashed entries verified. The contiguous4138certificate prefix was independently
admitted and is unchanged byte-for-byte; no old certificate was replaced or recomputed.
Exactly view5-r220-relief, resume4138 row-major to4799, midpoint radius1/2560.

| Final state | Count |
|---|---:|
|Completed tiles|{a['completed_tiles']}|
|Additional tail tiles|{a['tail_tiles']}|
|Accounted completed pixels|{a['pixels']}|
|Precision-attained tiles|{a['precision_attained_tiles']}|
|Precision-limited tiles|{a['precision_limited_tiles']}|
|Interrupted tile|{a['interrupted_tile']}|
|Unstarted tiles|{a['unstarted_tiles']}|

Pixel states `{json.dumps(a['pixel_states'])}`. Final verification uses the frozen original full-coarse
checker, independently of the continuation writer/admission code: identity, hash,64pixel positions,
integer lower/upper bounds, exact denominator/target inequality, cell/depth limits and rational
quantization ranges all pass. Immutable original manifest and every original payload hash rechecked.
Partial/invalid outputs never enter the completed manifest. Exact bounds remain conditional on the
frozen dyadic endpoint/width-one/opaque feature-order model and correct implementation, not formal
software verification or upstream coordinate certification.

## Accounting repair and controls

The CPU ledger is locked, atomically replaced, file/directory-fsync'd. Fixed new budget300CPU seconds,
600elapsed,384MiB/process,1GiB combined evidence. Preparation50,reporting30,supervision10 are reserved
allowances. Before child launch, pre-reserve full worker allowance; if terminal telemetry disappears,
keep the full debit. On wait/reap, record user+system child CPU including startup, setup, geography,
serialization and output, then settle a rounded-up measured debit. Live /proc counters are informative,
not terminal totals. Duplicate reservation/reinitialization is rejected; an abandoned reservation
cannot be erased by restarting. One geographic worker, one authorized tail execution.

Failure-injection controls pass for supervisor exception, worker termination, missing atomic temporary
file, interrupted output collection, partial manifest, exhausted/abandoned budgets, restart reset,
partial tile, false completion, invalid bound and invalid quantization. Supervisor finally reaps child
before collection/settlement. Any existing full allowance remains charged if accounting cannot finish.
Malformed certificates are rejected before writing a payload or manifest.

An initial pre-geographic launch failed because the parent hardCPU10 limit was inherited before the
child tried to set210. No geographic worker started. Original failed source freeze, ledger and result
are preserved under launch-failure. It was charged a conservative1s (not an invented measurement),
and worker allowance reduced to209. Parent soft9/hard220 before exec, hard10 only after child launch;
child207soft/209hard,clean205. A high-child-limit launch control passed before geography. These sum
with allowances to300 and do not reset a budget. Numerical engine, adapter,100000cells/pixel,depth16,
75/64s/pixel and conservative failure behavior are unchanged. Geometry/BVH retained for the tail.

## Original and additional resources, separately

Original worker accounting range remains {a['original_worker_range'][0]:.6f}–{a['original_worker_range'][1]:.6f}CPU s;
original total upper accounting envelope with its allowances remains {a['original_total_accounting_upper']:.6f}s.
First historical worker terminal telemetry remains missing. This authorization neither invents it nor
retroactively expands the original ceiling. Historical interruption/replay evidence is unchanged.

| New incremental observation | Value |
|---|---:|
|Tail worker total user+system CPU|{a['worker_incremental_cpu']:.6f}s|
|Supervisor measured CPU|{a['supervision_incremental_cpu']:.6f}s|
|Preflight measured CPU|{a['preparation_measured_preflight_cpu']:.6f}s|
|Independent final validation CPU|{a['validation_measured_cpu']:.6f}s|
|Known measured component sum at snapshot|{a['new_known_measured_cpu_sum']:.6f}s|
|Durable aggregate conservative charge including allowances|{a['additional_cpu_charge_upper']:.6f}s /300|
|Elapsed from recorded anchor +initial30s allowance|{a['additional_elapsed_with_initial_allowance']:.3f}s /600|
|Tail maximum recorded RSS|{a['tail_peak_rss_kib']:,}KiB|
|Maximum recorded RSS across fixture certificates|{a['peak_recorded_fixture_rss_kib']:,}KiB|
|Combined raw original/additional evidence at validation|{a['combined_old_new_evidence_bytes']:,}bytes|

Measured components are not the same as allowances or total exact CPU. Initial shell/tool preparation,
controls not externally metered, final reporting/packaging fit in the still-reserved preparation/reporting
allowances; they are not retrospectively invented measurements. The ledger conservative charge is the
resource admission figure, not CPU claimed consumed. Supervisor measured field is process-local;
the shared10s allowance covers both prelaunch/recovery invocations, whose records are retained.
Original peak RSS during its old interruption is unrecorded; reported maximum is recorded evidence,
not an exact retrospective global peak. Reference workers and supervisors enforce384MiB address space.
No rendering benchmark, production test, G4, candidate images or scoring instrumentation ran.

## Measured full-fixture cost versus models

Full completed-tile CPU: **{a['full_tile_cpu']:.6f}s** (old prefix{a['original_prefix_tile_cpu']:.6f}s,
new tail{a['tail_tile_cpu']:.6f}s). Original model1037.995314s;
{("difference "+format(a['versus_original_percent'],'.3f')+"%") if complete else "full completion not established"}.
Revised model1240.007999s;
{("difference "+format(a['versus_revised_percent'],'.3f')+"%") if complete else "full completion not established"}.
Completed-tile clocks include in-tile pixel checkpointing. Worker totals additionally include retained
setup, certificate compression/writes, manifest updates and process startup/exit. Supervisory CPU,
validation/reporting and historical replay are distinct and must not be presented as tile computation.

The old density interpolation underpredicted the original prefix by19.46%; transferring that factor
to its structurally different remaining tail was only a hypothesis. The newly measured tail tests it;
no model was used to exclude hard tiles. Prior descriptive evidence shows maximum density121 is not
maximum cost: density94 tile(58,49)cost5.024s versus density121 tile(13,30)3.534s. Orientation, feature
overlap, opaque source ordering and background contrast are not causally separated by one fixture.
Spatial lookup, predicate counts and per-pixel refinement/resource records are retained; no algorithm
was optimized after outcomes.

Conditional coarse-only32condition projections:
- Old model{a['coarse32_old_hours']:.6f}CPU h.
- Replace only this fixture with measured tile CPU: {a['coarse32_replace_measured_fixture_hours']}CPU h.
- Transfer its measured/full-model ratio to all Relief conditions, leave Map unchanged:
  {a['coarse32_transfer_relief_hours']}CPU h.

These are sensitivity models, not CIs, completion guarantees or proof all32conditions fit any budget.
No fine projection is validated by coarse completion. Isolated-feature passes, alpha rounding,
sequential RGB565 compositing, fixed-mask uncertainty certification and future scoring costs are
excluded and need separate decisions/instrumentation.

## Accuracy and next scientific decision

Maximum midpoint radius{a['max_radius']:.12g}; mean channel radius over completed pixels
{a['mean_radius']:.12g}; unresolved color-boundary area{a['unresolved_area_pixels']:.12g}pixel areas
remains charged to bounds. RGB565 ambiguity{a['rgb565_ambiguous_pixels']}pixels/{a['rgb565_ambiguous_channels']}channels.
Cells{a['cells']:,}, predicates{a['predicates']:,}, initial predicates{a['initial_predicates']:,}, depth{a['max_depth']}.
Coarse target is four times historical fine radius; these are reference uncertainty observations,
not candidate fidelity or subjective preference. Empty-pixel dilution does not certify a stroke-mask
quality metric. RGB565 outcome ambiguity requires discrete reasoning, not continuous-error shortcuts.

Recommended steering decision: whether to authorize a narrowly scoped certification-contract slice
for geometry-fixed metric uncertainty and discrete outcome bounds before proposing any fine/broader
reference generation. This completed fixture supports local coarse feasibility only. The original
scored experiment remains inconclusive; sequential scoring, fine frames and32condition generation
are not authorized by this report. Stop for PE-AA-01 steering review.

## Provenance and reproduction

Source/input hashes are in source-freeze.json and final source-manifest. Original archive, raw prefix,
models, logs and historical bounds are immutable dependencies. New outputs live separately; combined
manifest contains hashes and provenance rather than rewriting originals. Python3.14.8/Linuxx86-64,
stdlib-only numerical path. Exact commands:

```sh
ulimit -v 393216
python3 tools/aa-coarse-tail/preflight.py /tmp/aa-coarse-tail-evidence
python3 tools/aa-coarse-tail/test_accounting.py /tmp/aa-coarse-tail-evidence
# Recorded preexec repair/control/source freezes precede actual worker launch.
python3 tools/aa-coarse-tail/supervise.py /tmp/aa-coarse-tail-evidence
python3 tools/aa-coarse-tail/finalize.py /tmp/aa-coarse-tail-evidence
python3 tools/aa-coarse-tail/report.py /tmp/aa-coarse-tail-evidence
```

Reproduction needs a fresh output directory, immutable time anchor and verified original archive/prefix.
Do not rerun launcher against supplied evidence; duplicate ledger reservation and certificate admission
reject it. Commands do not authorize another experiment. No original protocol, renderer, G4, production,
merge or release changed.
'''
(source/'REPORT.md').write_text(text)
