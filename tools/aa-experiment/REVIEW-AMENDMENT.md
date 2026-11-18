# PE-AA-01 Phase 3B review amendment — 2026-11-18

This amendment supersedes the conflicting reference, statistics, blinding and timing
interpretation paragraphs in PROTOCOL.md. No scored execution is authorized. Claude's complete
review was read before edits. Jack's correction assignment retains the interactive 25% gate;
Claude's suggested replacement of that gate is not adopted. Arm C remains deferred, with the
qualified Phase 3A chain-body finding and original archives preserved.

## Unchanged treatments and fixture admission

B is **exactly** the original aa.cpp/aa.hpp, including its distance approximation, feature-mask
max and integer blending. fixtures.cpp and images.py are unchanged. No optimized B exists.
A replay must remain bit-exact against production on the same host. No cross-platform tolerance
weakens that gate. Source baseline stays 00be059; local original prototype d4c9ad9 has the same
tree 959b678 as published 9cbe1bd. This correction branch descends from the local prototype.
The original geographic fixture matrix, source/data hashes and original raw outputs are retained.
No geographic oracle score has been inspected during these corrections.

## Independent bounded oracle

oracle_rows.py uses geometry-derived butt rectangle polygons and radius-0.5 interior discs.
Consecutive duplicate vertices are removed once in oracle preparation (zero-area edges only).
Closed duplicate endpoints are canonicalized. A tile index retains intersecting primitives in
source feature order. For each subpixel row, polygon-edge intersections and analytic circle
chords yield x intervals. Boolean interval union is per feature; later source colors overwrite
earlier colors on that row. RGB sample sums retain within-pixel feature correlation and the
piecewise-constant background. There is no B distance/profile code or call to B. Quantization
and the sequential integer blend are independently implemented from the written contract.

8x8 tiles, dimensions <=4096 with total pixels <=307200, <=8192 features, <=262144 points,
finite coordinates within +/-1e6, and <=1000000 tile-index entries are enforced. At most 8192
feature count arrays of 8x8 uint32 are retained (2 MiB), plus one <=8192-sample color row,
output arrays and bounded geometry/index Python objects. Storage scales with input/index limits
and N, never N squared. This is host reference tooling, not the <1 MiB runtime candidate.
NumPy 2.5.3 is used; this explicitly supersedes the old README's no-NumPy restriction for the
oracle only. No graphics dependency enters B or production. Resource/index exhaustion aborts
without a verdict; output files are written only after every tile converges.

Start 64/128 samples per axis; compare continuous RGB per tile, refine affected tiles through
256/512/1024. Both <=1/1024 and <=5% of the smallest absolute error margin supporting any
proposed verdict remain mandatory. Synthetic validation uses only the former threshold because
no geographic verdict is proposed. run_reference.py requires an explicit positive decision margin.
Any later verdict with a smaller margin must revalidate/refine with that margin before reporting.
Failure at 1024 is inconclusive. All refinement deltas and sample levels are retained. Quantized
reference jumps, including sequential-reference jumps, are recorded as sensitivity diagnostics;
finite sampled references are not claimed analytic exactness or proven error bounds.

Controls compare the row oracle to the separate slow membership oracle, exact axis-aligned
rectangle/cap results, color last-writer overlap, disc area/symmetry, empty/offscreen preservation,
and invalid input. Synthetic B-versus-reference output uses the compiled unchanged B, not a port.
The table includes eight octants, lengths .25/.5/1/3/16, turns 15/45/90/150 degrees, closed square
and ring, and 20-segment smooth arcs with .25/.5/1/3 chords. It reports support-only/full MAE,
worst pixel-channel error, quantized/sequential differences, ink ratio and material missing pixels.
These controls reveal approximation errors; they are not geographic quality outcomes.

## References and diagnostic metrics

Keep three distinct references: continuous full-color last-writer reference; once-quantized
reference; and **per-feature sequential RGB565 achievable control**. The third averages each
feature's union coverage, rounds to 8-bit coverage, and applies the frozen integer blend in
source order. It is achievable by this representation, not a mathematical lower bound on every
possible RGB error. It must not replace the continuous target or silently change the original
20% excess-error gate. Report gap closed against the original once-quantized reference, and
separately against the sequential control with explicit labels. Any floor denominator <=10/1024
is floor-limited; no percentage verdict. Compositing mismatch is separately quantified by the
sequential-control versus continuous error; do not label it all stroke-shape error.

Primary geographic RGB MAE retains the frozen three-pixel geometry-derived neighborhood and
full endpoints. Support-only/full-frame/max/outside-mask errors, integrated contrast, missing
pieces and layer-specific results remain required in a later scored run.

Peak contrast and visibility use **untimed isolated source-feature sidecars over the original
background**, so unrelated layers cannot hide a lost feature. Contrast is mean absolute encoded
RGB difference from that pixel's background, normalized to [0,1]. Report peak contrast in true
reference support and reference peak alongside it; also report peak retention relative to local
source/background contrast (zero-contrast pixels ineligible). review_instruments.py implements
these definitions. Thin-feature visibility: preregister the midpoint of every nonzero original
projected edge, without candidate-dependent selection; in its clipped 3x3 pixel neighborhood,
a probe is reference-eligible if reference contrast reaches 10% of local source/background
contrast. Record candidate hits at the same threshold, eligible/hit counts, hit fraction per
feature, and features with zero hits. Weight features equally for feature summaries; probes
are diagnostics, not independent observations. Retain per-layer results and native human review;
these numerical thresholds are diagnostics, not new owner-approved acceptance gates.
Motion/translation stability scoring is explicitly deferred: the present phase proves reversal
and synthetic translation invariants only. No claim of temporally stable AA follows from them.

## Two performance interpretations (neither has run)

1. **Interactive replacement:** retain the original <=1.25 relative line-latency gates at BOTH
   rasterization-only and projection-plus-lines boundaries. Keep >=20% quality, local regression,
   memory/semantics and native legibility gates. Whole-frame >10% overhead remains a sanity trigger.
   Rejection here is rejection as an interactive replacement, not evidence AA lacks visual value.
2. **Adaptive settled-frame enhancement:** render the baseline promptly; optional higher quality
   settled output is a separate product hypothesis. Report B and A absolute line milliseconds,
   delta milliseconds, complete settled-frame milliseconds, worst selected fixture, cold setup,
   retained-context cost, peak live scratch and executable growth. Jack must approve and freeze
   **maximum added line milliseconds AND maximum total settle milliseconds on a named platform**
   before ANY timed execution for this scenario. Both budgets are currently UNSET, not waived.
   A host budget cannot approve G4 responsiveness. No machine-speed heuristic or automatic
   toggling is implemented. Future integration must design cancellation, fresh-frame ownership,
   safe fallback, progressive compositing and explicit user override separately.

The same frozen B is used for both interpretations. Eligibility for further investigation is not
production adoption. B-fast would require a separate named implementation, diff, output tolerance,
freeze and owner authorization; no tuning follows observed geography in this assignment.

## Timing schedule and uncertainty

Ten fresh-process blocks per condition/boundary, alternating ABBA/BAAB, one discarded warmup per
arm per process. Two measurement records per arm per block; internal A-calibrated repetitions
are not independent observations. All original boundaries and fixture coverage remain intact.
Rasterization: 32 conditions x10 =320 processes,1280 records. Projection-plus-lines: same.
Whole-frame: anchor r220 and coastline r440, both styles, x10 =40 processes,160 records.
Total **680 processes,2720 records (1360 per arm),1360 discarded warmups**. No target schedule is
authorized. All setup required per frame is timed; retained storage stays charged; no sums of
separately timed medians. Timer overhead <1% of accumulated batch time remains mandatory.

For each geometry/style/boundary, average the two same-arm records within a block, form paired
B/A ratios, then take the median across its ten blocks. The pooled line statistic is the median
of the 16 geometry ratios **separately for Map and Relief**. Require each style to pass.
Whole-frame has only two geometry IDs: report descriptive paired results; do not infer 16 units.

Use a fixed-seed (20261118), 100000-replicate paired **geometry bootstrap**, resampling 16 IDs
with replacement and retaining all styles, arms and blocks for each ID. Recompute the statistic
in every replicate; report percentile 2.5/97.5 bounds. Quality uses the same resampling but the
frozen ratio of style-specific mean excess errors. These are approximate selected-view
sensitivity intervals, not population confidence from random geography. Geometry is not randomly
sampled; report this limitation prominently. Four/ten blocks reduce measurement noise but never
turn conditions/pixels into independent geographic observations. No per-condition 95% claim.
Pass timing only when upper interval <=1.25, reject when lower >1.25, else inconclusive, and list
all per-condition medians/MAD/max and descriptive p95/order effects. This does not establish a
universal platform guarantee; a borderline result warrants more authorized evidence, not a
selectively reduced fixture set. Adaptive budget checks likewise retain worst-case descriptive
absolute observations and must not hide a slow fixture behind the pooled statistic.

## Cross-platform arithmetic and performance risks

Exact A replay is required separately on every tested platform with same-build production.
Host repeat/hash and sanitizer equality remain exact. For later cross-libm B comparisons only:
proposed diagnostic tolerance is <=1 packed-channel level per pixel (R/B 1/31, G 1/63), with all
changed pixels, largest differences and counts reported, no material missing stroke, and all
native reversal/invalid-input tests still required. Larger differences block equivalence claims;
no tolerance permits changed geometry or relaxes the independent quality oracle. This is a
cross-platform diagnostic policy, not G4 acceptance; target tests require separate permission.

Structural counts reproduce B's actual clipped bounds, rectangles, joins and feature clear/scan
regions from serialized fixtures, without executing a timed render. For each frame report segment
and join counts, primitive pixel evaluations, sum of feature bounding-box areas (cleared once,
scanned once), and largest segment bbox. These are operation counts, not speed predictions.
B invokes hypot/floor per primitive pixel, plus floor/ceil bounds, sqrt-like work per segment,
and scans sparse feature bounding boxes. Long unsplit diagonal segments have quadratic bbox
work; high-zoom subdivision caps can expose it. G4 software/libm costs are hypotheses requiring
native measurement. No performance optimization is included.

## Authorization still required

Independent review of this corrected source freeze, oracle/synthetic evidence and truly separate
blind image package; explicit owner authorization for geographic quality scoring; explicit owner
authorization for timed execution, including frozen platform-specific adaptive millisecond budgets.
No G4, production adoption, PR merge, release or automation permission is implied.
