# PE-AA-01 conservative error-enclosure pilot — 2026-11-18

**Recommendation1: develop the enclosure method further.** All four fixed candidate-blind tiles
received mathematically conservative exact-arithmetic bounds tight enough for the historical
continuous-reference precision, within the predeclared host budgets. This is a local feasibility
result, not a complete replacement oracle, scoring result or protocol amendment. Frozen experiment
`5dd566d942b01b8b8e92e0cb1cc3507910681532` remains inconclusive and unchanged.

## Method and numerical claim

Original binary64 endpoint coordinates are interpreted as exact dyadic rationals; width1 and
join radius1/2 are exact. Axis-aligned cells are classified against endpoint-defined rectangles by
bounds on dot/cross products (squaring removes square roots), and against discs by nearest/farthest
squared box distance. Every geometric predicate, dyadic area and accumulated color numerator uses
unbounded Python integers. The common coordinate scales in the real probes are2^45 or2^46.
There is no floating epsilon or libm in the predicates; arbitrary-precision integers cannot overflow.

Per feature, any certain containing primitive proves inside; all excluded primitives prove outside;
otherwise the cell is unresolved. Traverse features in reverse source order, retaining colors of
possible later writers until a certain writer masks all earlier features, or background is reached.
Per-channel extrema enclose every possible pointwise opaque color. This preserves union and
last-writer semantics; it never composites averaged alphas. Exact dyadic cell-area sums bound the
true pixel integral. Subdivision asserts nested child intervals, preserving conservative bounds
on resource exhaustion. The proof and code are in DESIGN.md and enclose.py.

Canonical lower/upper bounds are integer numerators with denominator255*4^16. Their correctness
is conditional on the stated coordinate/width model, implementation and CPython exact integer/IEEE
binary64 decoding semantics. Independent controls support that claim; this is not formally verified
software or a certificate for upstream projection/model uncertainty. The old sampled oracle's
rounded polygon normals are not used: the enclosure target is the ideal endpoint-defined stroke.
Float widths/times printed for convenience are not the certificates. Independent Fraction checks
validate the bound arithmetic and thresholds.

## Fixed inputs, caps and precision

Only the four authorized geographic tiles were evaluated, each8x8 pixels. Isolated probes use
original source feature indices1 (dateline) and2 (anchor). Geometry/background hashes are recorded
in each output and match the frozen admission manifest. Candidate images, sidecar arrays and the
blind key were not opened. A strict input audit permits only each job's geometry/background files.

Per tile:2,000,000 visited cells; quadtree depth16;90s adaptive-work deadline;384MiB address space;
105/110s CPU soft/hard caps and120s elapsed hard guard. All four processes returned successfully.
The exact local comparison is midpoint radius<=1/10240=0.00009765625, i.e. full interval width<=
0.0001953125. Both widths and radii are reported; this is a proposed midpoint certificate, not a
retroactive claim that the old sampled estimate has that error. No frozen sampling ceiling or
criterion changed. Adaptive subdivision reached depth12–13 (1/4096–1/8192 pixel boundary cells),
finer spatial resolution than the old1/1024 sample grid, but not a denser point-sampling run.

## Results

| Fixed probe | Max local radius |64-pixel MAE uncertainty U|Paired difference bound2U|Cells|Time(s)|Peak RSS(KiB)|
|---|---:|---:|---:|---:|---:|---:|
|Full dateline Relief r440 (0,32)|0.0000976549|0.0000437456|0.0000874912|635520|5.559|40540|
|Full anchor Map r220 (16,36)|0.0000976534|0.0000207066|0.0000414133|241788|2.634|39916|
|Isolated dateline Map r440 (2,57)|0.0000976520|0.0000156430|0.0000312860|165540|1.474|21076|
|Isolated anchor Map r220 (39,43)|0.0000976520|0.0000110990|0.0000221979|111792|0.944|21148|

The exact integer inequality passes for every one of256 pixel enclosures. Geographic predicates
number1,975,445 /1,525,194 /223,454 /145,799 respectively. Largest per-pixel heap size was11,804 cells;
final unresolved color-boundary leaves across each tile total158,901 /60,501 /41,381 /27,956. They
occupy at most0.012654 /0.018621 /0.006869 /0.013144 pixel area summed over the64-pixel tile. Those
regions remain fully charged to interval width; none is guessed or discarded. Per-pixel unresolved
area/leaf counts and exact lower/upper channels are in tile0.json–tile3.json.

These are host reference computation/resource observations, not rendering benchmarks. Jobs ran
concurrently (up to four); wall times are not isolated CPU timings. No full-frame cost or G4
feasibility is established. Selected first-blocking tiles are not a random/representative view set.

## Independent analytical evidence and failures

Nine analytical controls cover the historical rectangle phase, exact half butt cap, empty cap
contact, a near-horizontal Pythagorean rectangle with rational corners, two coincident-color orders,
a red/blue partial overlap, quarter-disc area/reflection, and disc tangency. Near-horizontal area
is obtained by independent rational polygon clipping in tests only—not a second geographic
integration method. Partial overlap gives exact red1/4, blue1/2, demonstrating ordered opacity.
Disc area is checked against pi/16; a separate exact rational Machin-series proof validates the
pi bracket independently of libm. Another1,512 Fraction point checks validate sufficient cell
classification against independent rational arithmetic.

All final controls pass. The initial harness incorrectly required identical interval endpoints
under disc reflection; heap tie order can stop at different equally valid enclosures. That assertion
failed and is preserved with its source. The corrected control requires both intervals to contain
the analytic area and overlap. This correction preceded the geographic source freeze. Producers
were unchanged after that freeze. A post-run validator separately checks all256 bounds, monotone
RGB565 outcome ranges, aggregate uncertainty fractions, the Machin bracket and producer hashes.
A forced one-cell budget stop returns a valid wide bound, not a false precision claim.

The original rectangle's exact fraction0.5553989016211744 lies in the new interval
[0.5552246272563934,0.555419921875]. Its midpoint has a rigorous radius below1/10240; the old accepted
point estimate0.5546875 lies outside. No successive-grid agreement is used in this certificate.

## Metric propagation and discrete controls

For any fixed geometry-derived mask ofP pixels, a reference-midpoint channel error bound e_pc implies
`|E_X(R)-E_X(midpoint)| <= U=sum(e_pc)/(3P)` for every possible candidate X. Thus a paired A-minus-B
mean-error difference has conservative uncertainty<=2U. The reported64-pixel tile bounds use this
identity without loading candidates. They are not the full preregistered stroke-mask MAE,16-geometry
statistics, gap-closed percentages or an A/B preference. No global inference follows from four tiles.
A local worst-pixel bound and a mean uncertainty are different quantities; both are retained.

Nearest RGB565 quantization is bounded by applying rational floor(M*c+1/2) to both continuous channel
endpoints.255/256 pilot pixels have one determined quantized output. Pixel(7,257) of the full dateline
tile permits green code53 OR54 (red27, blue20 fixed). The expanded green values215/219 differ by4/255:
a small continuous interval crossing a rounding threshold does not justify a tiny quantized error.
The code ranges are conservative supersets; channels need not be jointly achievable.

Sequential RGB565 needs per-feature union-coverage intervals, then discrete8-bit alpha rounding and
ordered integer blending over possible destination codes. Full-color final intervals alone cannot
recover those coverages. Correlations may make simple interval propagation loose. This pilot identifies
that requirement but does not implement or certify the sequential-control pipeline. Likewise,
gap-closed ratios require interval denominators and floor-limited handling, not division of point
estimates or a bootstrap that ignores numerical uncertainty.

## Comparison and one next step

Exact-x/event-split adaptive-y integration could avoid many boundary cells and yield much faster
area calculations for long nearly axis-aligned edges. It must handle union/last-writer interval
ordering, tangencies, changing active boundaries and independently justified y-integration error.
Ordinary successive quadrature estimates would recreate the certification problem. It was not
implemented. The enclosure method already passed the fixed local test, uses simple exact predicates
and has honest failure behavior; there is no present evidence requiring the more complicated
alternative as the immediate next step.

Recommend developing THIS method further, subject to steering authorization: independent code/proof
review, a bounded generalization of mask-level uncertainty accounting and discrete quantization/
sequential-control enclosures, and a separately approved broader reference-feasibility test. First
retain the same candidate-blind boundary and record resource failures. No new samples/tiles/scored
run are assigned by this recommendation. Full-frame scalability, tighter union classification,
executable certificate checking and discrete-outcome tightness remain unresolved. No automatic
protocol adoption, renderer acceptance or subjective visual-preference claim.

## Exact reproduction

Source is tools/aa-enclosure-pilot in the isolated research branch. Original protocols, renderers
and evidence were verified unchanged (33 frozen source hashes and188 recovered record hashes).
All raw bounds, controls, input hashes, limits, logs and source freezes accompany this report.

```sh
python3 tools/aa-enclosure-pilot/controls.py > /tmp/pilot-controls.json
# Actual jobs ran concurrently; each independent command is:
python3 tools/aa-enclosure-pilot/run_pilot.py 0 /tmp/aa-enclosure-pilot-evidence > /tmp/tile0.json
python3 tools/aa-enclosure-pilot/run_pilot.py 1 /tmp/aa-enclosure-pilot-evidence > /tmp/tile1.json
python3 tools/aa-enclosure-pilot/run_pilot.py 2 /tmp/aa-enclosure-pilot-evidence > /tmp/tile2.json
python3 tools/aa-enclosure-pilot/run_pilot.py 3 /tmp/aa-enclosure-pilot-evidence > /tmp/tile3.json
python3 tools/aa-enclosure-pilot/validate_results.py /tmp/aa-enclosure-pilot-evidence
```

The second argument is a recorded output-directory context; job JSON goes only to inherited stdout
to keep the strict input audit simple. The evidence execute.py records the exact parallel invocation.
Python3.14.8, Linux x86-64; no graphics or numerical-library dependency in the enclosure implementation.
The immutable original geometry reader is imported for decoding only. Exact input paths are recorded;
use the archived geometry/background files under the corresponding frozen fixture directory to reproduce.
No production build/tests or Retro68/G4 probe were needed or run for this host-only Python pilot.
