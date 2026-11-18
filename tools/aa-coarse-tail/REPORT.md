# PE-AA-01 aggregate accounting repair and coarse tail v1

2026-11-18. Baseline8951cec392b13f3cff5c54574f876ca9e55734c0; numerical baseline7ab358f2 unchanged.
Recommendation: GO for a steering decision about the next certification-development slice; no further run is authorized.
Candidate-blind feasibility only, not scoring, renderer acceptance or a protocol amendment.

## Recovery and final status

Original archive SHA256 `d198144721dc036020ac0332c25499eececb312e33fce76fd88d7eb58555649b`
and all4226 archived hashed entries verified. The contiguous4138certificate prefix was independently
admitted and is unchanged byte-for-byte; no old certificate was replaced or recomputed.
Exactly view5-r220-relief, resume4138 row-major to4799, midpoint radius1/2560.

| Final state | Count |
|---|---:|
|Completed tiles|4800|
|Additional tail tiles|662|
|Accounted completed pixels|307200|
|Precision-attained tiles|4800|
|Precision-limited tiles|0|
|Interrupted tile|None|
|Unstarted tiles|0|

Pixel states `{"precision_reached": 307200}`. Final verification uses the frozen original full-coarse
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

Original worker accounting range remains 1094.019716–1326.699909CPU s;
original total upper accounting envelope with its allowances remains 1626.699909s.
First historical worker terminal telemetry remains missing. This authorization neither invents it nor
retroactively expands the original ceiling. Historical interruption/replay evidence is unchanged.

| New incremental observation | Value |
|---|---:|
|Tail worker total user+system CPU|118.660918s|
|Supervisor measured CPU|0.681820s|
|Preflight measured CPU|2.561970s|
|Independent final validation CPU|3.581906s|
|Known measured component sum at snapshot|125.486613s|
|Durable aggregate conservative charge including allowances|209.661000s /300|
|Elapsed from recorded anchor +initial30s allowance|451.411s /600|
|Tail maximum recorded RSS|41,664KiB|
|Maximum recorded RSS across fixture certificates|54,632KiB|
|Combined raw original/additional evidence at validation|17,710,250bytes|

Measured components are not the same as allowances or total exact CPU. Initial shell/tool preparation,
controls not externally metered, final reporting/packaging fit in the still-reserved preparation/reporting
allowances; they are not retrospectively invented measurements. The ledger conservative charge is the
resource admission figure, not CPU claimed consumed. Supervisor measured field is process-local;
the shared10s allowance covers both prelaunch/recovery invocations, whose records are retained.
Original peak RSS during its old interruption is unrecorded; reported maximum is recorded evidence,
not an exact retrospective global peak. Reference workers and supervisors enforce384MiB address space.
No rendering benchmark, production test, G4, candidate images or scoring instrumentation ran.

## Measured full-fixture cost versus models

Full completed-tile CPU: **1208.081948s** (old prefix1090.660003s,
new tail117.421945s). Original model1037.995314s;
difference 16.386%.
Revised model1240.007999s;
difference -2.575%.
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
- Old model5.058424CPU h.
- Replace only this fixture with measured tile CPU: 5.105669947712073CPU h.
- Transfer its measured/full-model ratio to all Relief conditions, leave Map unchanged:
  5.562579180106195CPU h.

These are sensitivity models, not CIs, completion guarantees or proof all32conditions fit any budget.
No fine projection is validated by coarse completion. Isolated-feature passes, alpha rounding,
sequential RGB565 compositing, fixed-mask uncertainty certification and future scoring costs are
excluded and need separate decisions/instrumentation.

## Accuracy and next scientific decision

Maximum midpoint radius0.000390625; mean channel radius over completed pixels
2.90361389564e-05; unresolved color-boundary area65.4167556465pixel areas
remains charged to bounds. RGB565 ambiguity2177pixels/2296channels.
Cells147,136,304, predicates165,543,930, initial predicates128,107, depth13.
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
