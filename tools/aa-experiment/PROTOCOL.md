**Final correction record:** [FINAL-PRE-SCORING.md](FINAL-PRE-SCORING.md) supersedes conflicting metric, bootstrap, control and readiness claims.

# PE-AA-01 Phase 3B — original A/B implementation and measurement plan

**Review correction:** [REVIEW-AMENDMENT.md](REVIEW-AMENDMENT.md) supersedes conflicting
reference, metrics, statistics, timing interpretation and blinding text below.
The original freeze remains preserved in the original evidence and parent commit.

2026-11-18. Owner-authorized host prototype/correctness/fixture verification/images only.
**Do not run scored image comparisons or performance measurements before independent review.**
Baseline: 00be059502db996661fdd7a9f22d366bcef87ed8. Related: #25, #64, #97, accepted ADR 0002.

## Dated amendment: arm C deferred

AGG outline AA is deferred from the complete connected-polyline comparison. Phase 3A confirmed
high-level vertex removal and isolated short-stroke/endpoint problems. Its final diagnostic
qualified the blanket NO-GO: smooth chain interiors with 0.5–3 px chords remained connected and
generally close to the independent reference, with localized reversal/overlap differences.
The 0.25 px chains had insufficient endpoint-excluded interior to establish body behavior.
There is no general finding against AGG scanline/stroke functionality and no permission to
extend that investigation here. C is neither adopted nor proven impossible.

Complete prior evidence is preserved unchanged, not replaced by these conclusions:

* [Phase 3A evidence](https://chatgpt.com/library?file_id=libfile_d57a53befb488191bc9631b38a24d5d7&account_id=personal), archive SHA-256 `0697938c0062068dc55e9128df643d2fae01641b85f8ec1fd84aee1d3faaaf96`.
* [Final chain diagnostic](https://chatgpt.com/library?file_id=libfile_cdef2644f3508191b35c4352975ad989&account_id=personal), archive SHA-256 `b57ae664da8ed563c6e1f947471e2cd0a402af7bcb407d400eefb032b9e0ece1`.

The only present treatments are A (production draw_line unchanged) and B (first-party distance-profile AA).
This is research under #25, not release-scope or accepted-ADR modification.

## Candidate and geometry contract

One-pixel perpendicular nominal width; union of butt-ended segment rectangles and radius-0.5
discs at interior vertices; closed features join at the first vertex. Exact consecutive
duplicates contribute no segment and do not turn open caps into round joins. An entirely
degenerate feature paints nothing in B; A retains its existing zero-length pixel behavior.

B uses the revision-2 Wu-style variant: at pixel center, primitive coverage is
clamp(0.5 - signed_distance_to_primitive, 0, 1). Accumulate the maximum across all primitives
of the same feature, then blend once. This is an approximate distance-filtered stroke,
**not exact union area coverage**; notably min primitive signed distance is not generally
the exact signed distance inside an intersecting union. The independent area oracle can reject it.
No smoothing/profile/width tuning after geographic images or scores. No short-edge simplification.

Canonicalize each rectangle's two endpoints lexicographically for arithmetic reversal identity.
Retain all original projected points; this is not geographic simplification. Closed source
features with horizon breaks become open visible chains; do not join across hidden portions.
Separate visible chains belonging to one source feature share a single coverage accumulation.
Distinct source features composite in production order: parallels, meridians, coasts, borders.

Reject non-finite inputs and absolute coordinates >1e6 before integer conversion, without changing
the output. Exact boundary values remain supported. Clip pixel writes after footprint bounds;
do not clip the centerline to the viewport. Respect framebuffer row clipping. Width and alpha
are fixed; no new settings, production integration or progressive composition.

Coverage q=floor(255*a+0.5). Shared RGB565 blend: expand 5-bit channels by (v<<3)|(v>>2), 6-bit
by (v<<2)|(v>>4); z=(q*src+(255-q)*dst+127)/255 using integer division; quantize to M=31/63
by (z*M+127)/255. q=0 preserves dst; q=255 writes src. Blend once per source feature.

Memory: one reusable byte per viewport pixel, allocated with nothrow before drawing; dimensions
1..4096, caller-enforced byte budget, injectable failing allocator test. No per-feature dynamic
allocation in B. At 640x480 this is 307200 bytes plus fixed workspace metadata. Borrowed geometry,
input/background/output buffers are separately reported common-fixture memory. There is no claim
of a hard wall-time bound from bounded pixel/segment counts.

## Fixture freeze

Geography: 16 geometries x 2 styles, 640x480, heading 0, radii 220/440; centers
(30,10), (62,8), (-3,120), (48,15), (0,179), (89,0), (-89,0), (-20,-150).
Map and Relief use the actual terrain-job colors (including Relief black borders), smooth 1-degree
level-zero terrain, centered focus, max_chord_px=3.0; all three line layers enabled.
Map/Relief share geometry and are paired conditions, not independent observations.

Inputs and SHA-256:

```
ec2f5bde911fc131610786520a36482a98ad7312f29b953abc7ba6ccda8df773 data/atlas.gatl
8d0fd2e6777f24dc13ac47e0f73aa459e503c70116d66a61bb2b339b618766a6 data/elevation-1deg.gelv
52f39e0b1623ccde92462c591da8335649064ff3d6e892b671cf02f3519070cc data/landclasses-1deg.ggrd
```

Use a research-only transcription of pinned production arc projection/subdivision, preserving
all numeric operation order. Exact A replay against production lines-at-end refinement is a
mandatory fixture admission gate. Terrain-only backgrounds use existing RefinementJob with
terrain_only=true, lines_first=false, preserve_bitmap=false; compare budgets 1,7,256.
Serialize feature/chain boundaries, colors and binary64 coordinates explicitly little-endian;
hash every background, geometry file and output. No native structs on disk.

Synthetic matrix: all octants; lengths 0,0.25,0.5,1,3,16,256; phases {0,.25,.5,.75}²;
reversals, joins 15/45/90/150 degrees, closed paths, crossings, retraces, exact duplicates,
viewport and row clipping, dark/light/ramps, odd dimensions, nonfinite/extreme coordinates.
Short pieces must remain supplied to B; correctness tests do not claim exact area coverage.
Dense-coastline phase sweep is reserved for later diagnostics, not extra independent samples.

## Independent quality reference — frozen, not scored in Phase 3B

Use independent analytic rectangle/disc sample membership, original continuous coordinates,
pixel centers (x+.5,y+.5). At each high-resolution sample, form the union for each source feature,
then apply opaque feature colors in fixture order over that pixel's piecewise-constant terrain.
Average samples and retain unquantized encoded-RGB reference; also nearest-quantized RGB565.
Do not composite averaged feature coverages: this loses subpixel overlap correlation.
The implementation must have no calls to B's distance, coverage or blend helpers.

Start 64x64 and 128x128 centers per pixel; refine affected tiles through 256/512/1024 until max
per-channel difference <=1/1024 and <=5% of any proposed quality-decision margin; otherwise
inconclusive. Validate exact rectangle coverage, disc area/symmetry, feature-order crossings and
reversal. The existing Phase 3A independent Python union oracle is retained for synthetic controls;
a full colored multi-feature oracle must pass its controls before later scored testing.

Primary metric normalized RGB MAE in a fixed three-pixel geometry-derived stroke neighborhood;
also report support-only error, max error, full-frame error, missing pieces, contrast and motion
stability. Endpoints remain in primary quality scores. Quantization floor = oracle quantized once;
sequential quantization is a sensitivity control, not a lower bound. Gap closed is ratio of mean
excess errors, separately per style across 16 equally weighted geometries. Denominator <=10/1024
is floor-limited; no percentage conclusion. No pseudoreplication through styles or repeated pixels.

Native-size review uses concealed/randomized A/B labels, identity key withheld until independent
reviewer observations are recorded. Include maps, Relief, dense islands, borders and endpoint/join
crops; enlargements are diagnostic, never a replacement for native-size review. Generating images
now is authorized; no aggregate numeric geographic scoring or tuning is authorized.

## Future timing boundaries and order — do not run now

Raster-only includes candidate setup/coverage/blending. Projection-plus-lines adds identical
projection/clipping/subdivision. Whole settled frame includes actual fresh terrain+geometry+lines,
not a sum of independent medians. Exclude import, logging, hashing, image export, reference oracle
and display submission. Distinguish retained workspace from cold allocation/setup.

One binary; four fresh-process blocks per fixture/boundary, alternating ABBA and BAAB, one
discarded warmup per arm per process: 8 records/arm/condition. Raster and projection boundaries
use all 32 conditions: 128 processes and 512 records each. Whole-frame uses anchor r220 and
coastline r440, both styles: 16 processes and 64 records. Total 272 processes, 1088 records,
544 per arm, plus 544 discarded warmup invocations. Tiny kernels use in-process batches with
common reset outside timing; choose common batch size from A-only calibration before B timing.
Internal draws are not independent observations; timer overhead must be <1% or unresolved.

Retain raw ordered results; median/MAD/max and descriptive p95, paired block uncertainty, process
wall/user/system time. With four blocks, tail/interval uncertainty is limited; report inconclusive
where appropriate. Keep outliers, record failure, compiler/source/data/binary/layout/fixture hashes,
flags, host/OS, build type and allocator accounting. No cold-cache claim or cache flushing.

## Decision rules — future comparison, subject to review

Retain revision-2 criteria: >=20% excess-error gap closed in both styles; no condition worsens
beyond max(10% baseline error,2/1024); no confirmed loss of legibility or missing material stroke.
Both raster-only and projection-plus-lines median overhead <=25% versus A; pass only if upper
95% uncertainty bound is within budget, reject only if lower bound exceeds, else inconclusive.
Whole-frame median overhead >10% triggers investigation, not a hidden allowance for slow lines.
Report p95 descriptively. Peak incremental scratch <1 MiB at 640x480, enforced and gracefully fallible.
Installed-code growth, stack/object sizes and common-fixture memory remain separate observations.

Reject B, accept further investigation, or defer for evidence: native visual benefit alone cannot
excuse failed semantics/resources. None of these host results accept G4/Classic behavior.
Hardware, production integration, progressive AA and a scored run all require separate authorization.

Explicit changes from revision 2: C deferred; three-arm permutations replaced by paired ABBA/BAAB
with exact reduced counts; setup and body cost still separated; experiment standalone build only.
The full colored oracle and timing runner are review prerequisites for a later scored run, not
silently replaced by the present correctness tests. No automation is added.
