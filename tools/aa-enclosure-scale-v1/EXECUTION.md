# Enclosure scale-v1 — pre-execution freeze, 2026-11-18

Original scored run5dd566d9 and pilot33d7d09f remain immutable. This is a separately versioned
candidate-blind feasibility extension only, never a scorer or protocol amendment.
Claude's complete125-line review was read before edits.

Stage1 passed original analytical controls with explicit precision assertions, six independent
raw-chain analytical topology controls,96 randomized mixed-chain scenes (3456 exact Fraction point
color checks),12 randomized exact integrated axis-rectangle/color tests, three killed topology
mutants, input rejection and fair one-cell-per-pixel budget tests. The four original anchors pass
at midpoint radius1/10240 with overlapping old/new enclosures. Producer domain: finite coordinates
within+/-1e6, <=262144 vertices/32768 chains/8192 features, colors0..65535, dyadic denominator<=2^128;
unsupported fine-denominator coordinates are rejected before primitive construction. Exact arithmetic
proof is inherited from the pilot. The BVH uses exact conservative primitive boxes; ordered feature
colors are reconstructed by original source index, while union primitive order is immaterial.

Precision correction: radius1/10240 has the SAME numerical value as the margin-approved successive-grid
threshold0.00009765625; it bounds midpoint error and is a DIFFERENT, STRONGER claim than grid agreement.
It is10x smaller than the original looser1/1024 tolerance, but no old stop is retrospectively certified.
No frozen threshold or1024 point-sampling ceiling is modified. Depth16 is adaptive cell subdivision.

All32 geographic conditions receive a GEOMETRY-ONLY4800-tile primitive-overlap complexity census,
not a full-frame reference. A primitive's conservative box touching a tile contributes one count.
The scale-up set uses four preselected geometry/radius anchors: view0-r220,view1-r440,view4-r440,
view7-r220, each in Map and Relief (8 fixture conditions). For each select five density strata:
empty(0), minimum positive, median positive, p95 positive and maximum. Ties use minimum SHA256 of
seed20261118|fixture|stratum|linear_tile_index, with no outcomes consulted.40 coarse jobs plus the
four original pilot anchors at coarse precision; first eight median/p95/max jobs in fixed ordering
repeat at fine precision; the two isolated pilot feature tiles repeat at fine precision as white
union-coverage over black for alpha ambiguity. Total54 Stage2 jobs plus4 Stage1 anchors=58 jobs,
44 unique fixture/tile coordinates. selection.json lists every coordinate and exact rule; it and
source hashes are frozen before the first scale-up job.

Precision targets are1/2560 and1/10240. No threshold can be tuned after results. Per tile384MiB address
space, CPUsoft85/hard90s, worker alarm89s and parent process timeout90s with explicit kill/wait.
Each pixel receives100000 cell visits and75/64 seconds; unused quotas are NOT transferred. Pixel
classification/refinement times, predicates, states, precision, cells/depth and unresolved area are
retained. Each completed pixel is atomically checkpointed, so a terminated job keeps its completed
pixel enclosures; unfinished pixels remain explicitly unstarted/unknown, never silently complete.

Only one job runs at once; CPU accounting is process CPU, independent of wall scheduling. Geographic
jobs including Stage1 are capped at900 aggregate CPU seconds, reserving300 of the20 CPU-minute envelope
for tests, census and coordinator overhead. Before launching a job, reserve a full90s; stop and retain
unstarted jobs when insufficient CPU remains. No per-tile budget fallback/retry. The cap bounds spent
computation, not convergence. Record failures and interrupted JSON/checkpoints. No renderer benchmark.

Measured setup/index building, initial BVH/root classification and refinement costs are distinct.
Density is only a cost proxy. Estimation will use geometry-count-weighted interpolation of the five
observed density anchors, with cross-fixture min/max scenario curves; not a population confidence
interval or a guarantee. Censored jobs make extrapolations lower bounds. Fine/coarse ratios will be
reported only for matched tile/mode jobs. Full-fixture/32-condition costs are projections, NOT measured
full-frame outcomes; no full reference or quality comparison is permitted.

Coverage jobs bound isolated feature union area, so alpha codes are exact endpoint outcomes of
floor(255*coverage+1/2). Full sequential RGB565 propagation is not implemented. Continuous U/2U is
only a fixed-mask uncertainty statement; no reference-dependent support mask is used for scoring.
No candidates, keys, G4, production changes, PR merge/release or automatic adoption.
