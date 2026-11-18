# PE-AA-01 final pre-scoring correction record — 2026-11-18

Baseline: published 2955beda16c002e92a38967583bde198a259e8f4, tree
df32400828f392e95418ec42b9e888799077efa0. Claude's complete 83-line review was read before
editing. This record supersedes conflicting metric/control/readiness claims in earlier reports.
Only quality preparation is in scope. No geographic candidate scoring or performance measurement
has occurred. B, fixtures.cpp, images.py, original geographic geometry and production are unchanged.

## C1 — distinct peak measures

On the same eligible pixels (true isolated reference support AND positive source/background
contrast), report candidate and reference peak contrast; their respective maximum per-pixel ratios
to local source contrast; and peak_retention = candidate_peak/reference_peak. When reference peak
is zero, retention is null/undefined (including no eligible pixels); other empty-support maxima
are zero. The two peaks may occur at different pixels in the same eligible set. The 0x8410 versus
0xffff analytical control over black gives 394/765 = 0.5150326797385621 retention. Visibility is
a lost-feature probe, not a measure of contrast loss. The original visibility threshold remains.

## C2 — undefined bootstrap policy, frozen before geography

Seed 20261118 and 100000 replicates remain fixed for scoring. For each style/condition separately,
exclude nonfinite replicate statistics from its percentiles, count and report defined/undefined
replicates and undefined fraction. An undefined fraction **greater than 5%**, no defined replicates,
or a floor-limited original point statistic makes that condition's interval inconclusive/null.
Exactly 5% is permitted; a reported finite interval is conditional on defined replicates. This
policy does not erase the floor limitation or guarantee unbiased inference. Mixed valid/undefined,
rare undefined, deterministic repetition and all-floor-limited controls cover the policy. Map and
Relief remain paired within the 16 geometry units. Percentile bootstrap at n=16 can undercover;
these intervals describe selected-view sensitivity and are not random-population confidence.

## C3 — independent reader/extractor parity

extractor_audit.cpp includes the **unchanged** original extractor in a separate translation unit
and runs only its extraction path, not its image generation main. It emits metadata/counts directly
from the C++ in-memory fixture and serializes new geometry into a separate directory. The independent
Python reader is compared against that data for all 32 frozen fixtures: feature IDs/colors/counts,
per-feature chain counts, every closed flag and original vertex count, supplied adjacent-edge count,
effective nonzero rectangle-segment count and join-disc count. Implicit closure is counted separately
from supplied edges; consecutive exact duplicates contribute no rectangle. All new serialized bytes
must equal the original frozen files. Candidate outputs are neither loaded nor scored.

## C4 — analytical and full-frame controls

Added exact horizontal AND vertical rectangle area, half-covered butt endpoint packed 0x8410,
reversal, adjacent empty pixels, viewport/cap clipping, tangent outside and degenerate controls.
The direct disc-row primitive has radius .5, analytical area pi/4, area error <=1/1024 at 256x256,
exact reflection symmetry, exact center/tangent chords and nextafter-outside rejection. Two complete
8x8 synthetic full-color frames compare every pixel to the separately implemented slow membership
oracle, including a wobbly closed ring, duplicates, intersections and random packed backgrounds.
Continuous boundary ties permit at most one 64x64 sample contribution per channel (1/4096 normalized);
packed RGB565 must still agree exactly. Analytical axis-aligned controls retain exact requirements.
Recorded comparisons had zero continuous differences and zero packed differences. Nonconvergence
at the fixed 1024 ceiling is tested to return inconclusive; NaN/inf/nonpositive margins are rejected.

The earlier synthetic table contains **50 render conditions**, not 50 independent geometries.
For each of five lengths, octants (0,7), (1,6), (2,5), (3,4) mirror about y=20.5 and are retained.
Thus there are 20 reflected pairs plus ten other conditions, i.e. 30 groups after only that
reflection equivalence. No synthetic row is a geographic observational unit. Short isolated stubs'
excess ink remains a documented limitation of B; B was not adjusted.

## Candidate-blind preflight

The structurally selected original anchor view0-r220-map is used; no candidate-dependent selection.
A fresh directory exposes only its .geometry and background.rgb565. An audit hook rejects every
other file access after imports; only those two inputs can be opened. Sampling checks tolerance
1/1024 with margin=None, **not** a fabricated approved positive decision margin. Every affected
tile must converge through the existing 64/128/256/512/1024 schedule. Operational limits are 512 MiB
address space, 295s CPU soft/300s hard, and 360s elapsed; limits were set before execution. Resource
usage is reference feasibility accounting, not a rasterizer performance benchmark. Limits and
invalid/convergence failures produce inconclusive/error status and no candidate verdict. Initial
and final preflight records are retained. Final run uses the frozen implementation. The initial
run predates only explicit invalid-margin/resource-signal guards; valid geometry semantics did
not change. Sequential floor jumps remain logged sensitivity diagnostics, not a separate continuous
convergence proof. Scoring must rerun references at the owner-approved positive decision margin.

## Isolated-feature sidecars and blinding

feature_sidecar.cpp strictly reads bounded serialized geometry and background, validates the entire
input, then renders one source feature using unchanged A or B. sidecars.py pairs those isolated
outputs with an independent isolated reference, quantized/sequential controls, original edge-midpoint
probes and geometry/reference support. It processes one feature at a time and writes compressed
sidecars with a 4 GiB output cap; no completion manifest is written after failure. Reference margin
must be positive/finite. Synthetic tests verify both arm outputs, independent cap/reference,
other-feature exclusion, probes, metrics and truncated-input rejection. No geographic sidecars
have been generated. isolated_metrics.py is a later authorized diagnostic entrypoint, not run here
on geography. It does not infer human legibility.

The separate review directory contains exactly 64 neutral 640x480 PNGs, review.html and judgments.csv.
PNG chunks are only IHDR/IDAT/IEND; no metadata key. HTML references only neutral filenames. The sheet
has 32 blank judgment rows and eight consistent columns; the prior extra blank column is corrected.
Identity remains outside both the technical and visual packages. Collect visual judgments before
revealing identities. No new reviewer preference has been invented.

## Source/runtime freeze and provenance

final-source-freeze.json names the exact immutable **instrument payload commit and tree**, Python
and NumPy versions, source hashes, unchanged B/extractor hashes and baseline identity. It is added
in a following metadata commit to avoid impossible self-referential commit hashes; the payload
commit contains every scored instrument. The final metadata commit differs only by that freeze.
The old pre-image reference.py/test_aa.cpp versions remain unrecovered; no diff or unchanged-semantics
claim is made. New instrument diffs, C++ geometry parity and the new freeze close the current audit
path without inventing historical evidence. The independent slow reference is a cross-check only.

## Exact verification commands

From this isolated repository, all artifacts use fresh /tmp paths:

```sh
cmake -S . -B /tmp/aa-final-core -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/aa-final-core --target globe_core -j 4
cmake -S tools/aa-experiment -B /tmp/aa-final-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLOBE_CORE_BUILD=/tmp/aa-final-core
cmake --build /tmp/aa-final-build -j 4
ctest --test-dir /tmp/aa-final-build --output-on-failure
python3 tools/aa-experiment/review_instruments.py
python3 tools/aa-experiment/statistics_plan.py
python3 tools/aa-experiment/final_controls.py
mkdir /tmp/aa-final-extracted
/tmp/aa-final-build/aa_extractor_audit "$PWD" /tmp/aa-final-extracted > /tmp/aa-final-evidence/extractor-counts.jsonl
python3 tools/aa-experiment/check_geometry_parity.py /tmp/pe-aa-phase3b-fixtures /tmp/aa-final-extracted /tmp/aa-final-evidence/extractor-counts.jsonl /tmp/aa-final-evidence/geometry-parity.json
python3 tools/aa-experiment/test_sidecars.py /tmp/aa-final-build/aa_feature_sidecar
python3 tools/aa-experiment/audit_blind.py /tmp/aa-final-blind
python3 tools/aa-experiment/quality_only.py --self-test
OPENBLAS_NUM_THREADS=1 python3 tools/aa-experiment/oracle_preflight.py /tmp/aa-final-preflight-input /tmp/aa-final-evidence > /tmp/aa-final-evidence/preflight.json
cmake -S tools/aa-experiment -B /tmp/aa-final-san -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGLOBE_CORE_BUILD=/tmp/aa-final-core -DAA_SANITIZE=ON
cmake --build /tmp/aa-final-san -j 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/aa-final-san --output-on-failure
ASAN_OPTIONS=detect_leaks=0 python3 tools/aa-experiment/test_sidecars.py /tmp/aa-final-san/aa_feature_sidecar
```

Imported baseline core remains uninstrumented; LeakSanitizer disabled for the established environment
restriction. No CI/full production CTest or Retro68/G4 claim follows from these checks. Retro68 probe
is still unavailable and is explicitly not a quality prerequisite. No timed-runner expansion.

## Geographic quality-only commands — DO NOT RUN before owner authorization

Owner must explicitly authorize quality-only execution and set PEAA_OWNER_DECISION_MARGIN to an
approved positive normalized absolute reference decision margin. It is currently UNSET. This is
not a millisecond budget; adaptive/performance decisions remain separate and cannot block or
implicitly authorize quality-only scoring.

```sh
: "${PEAA_OWNER_DECISION_MARGIN:?Owner must approve a positive geographic reference decision margin}"
mkdir /tmp/aa-quality-references /tmp/aa-quality-sidecars
for view in 0 1 2 3 4 5 6 7; do
  for radius in 220 440; do
    for style in map relief; do
      fixture="view${view}-r${radius}-${style}"
      OPENBLAS_NUM_THREADS=1 python3 tools/aa-experiment/run_reference.py "/tmp/pe-aa-phase3b-fixtures/${fixture}.geometry" "/tmp/pe-aa-phase3b-fixtures/${fixture}-background.rgb565" "/tmp/aa-quality-references/${fixture}" --decision-margin "$PEAA_OWNER_DECISION_MARGIN"
      OPENBLAS_NUM_THREADS=1 python3 tools/aa-experiment/sidecars.py "/tmp/pe-aa-phase3b-fixtures/${fixture}.geometry" "/tmp/pe-aa-phase3b-fixtures/${fixture}-background.rgb565" /tmp/aa-final-build/aa_feature_sidecar "/tmp/aa-quality-sidecars/${fixture}" --decision-margin "$PEAA_OWNER_DECISION_MARGIN"
      python3 tools/aa-experiment/isolated_metrics.py "/tmp/aa-quality-sidecars/${fixture}" "/tmp/aa-quality-sidecars/${fixture}/metrics.json"
    done
  done
done
python3 tools/aa-experiment/quality_only.py /tmp/pe-aa-phase3b-fixtures /tmp/aa-quality-references /tmp/aa-quality-summary.json --decision-margin "$PEAA_OWNER_DECISION_MARGIN"
```

quality_only.py implements the original three-pixel neighborhood literally as output pixel centers
at Euclidean distance <=3 from the continuous rectangle/disc union. Its tests are synthetic only.
It reports primary MAE, maximum/full-frame errors, outside-mask changes, original/sequential controls,
style-specific ratio-of-mean-excess-errors and bootstrap counts/status. The separate feature sidecars
supply peak/visibility diagnostics. This is not the full three-arm/performance benchmark. Original
native-review, local-regression/missing-feature and reference-margin requirements still apply before
any quality verdict; the runner does not issue an automatic adoption verdict. If a proposed verdict
needs a smaller absolute margin than approved, refine the reference accordingly or report inconclusive.
No geographic scoring command above was executed in this assignment.

## Remaining owner decisions

Authorize **quality-only** execution and specify its positive reference decision margin. Preserve
blind judgments before identity disclosure. No further general design review is requested. Performance
runner work, adaptive millisecond budgets, target access, production adoption, merge and release all
remain separate future authorizations.
