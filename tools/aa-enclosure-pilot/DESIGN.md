# PE-AA-01 conservative enclosure pilot — frozen scope, 2026-11-18

Candidate-blind research; not an amendment of 5dd566d9 or a scorer.
Inputs are exactly the four previously selected 8x8 tiles and the analytical rectangle.
Reference target: opaque source-order painting of rectangle/disc unions, encoded RGB arithmetic.
Original geometric vertices are exact binary64 dyadic rationals; perpendicular width=1 exactly.
Upstream projection/model uncertainty is excluded. The prior sampled oracle rounds rectangle
normals into floating vertices; this pilot directly encloses the ideal endpoint-defined rectangle,
not the internal roundoff of that sampled implementation.

## Conservative construction and arithmetic

Convert every coordinate by float.as_integer_ratio, then scale to a common power-of-two integer
lattice containing the maximum subdivision depth. All classification and accumulation use Python
arbitrary-precision integers. No sqrt, floating epsilon, libm or rounded polygon normal participates
in geometric decisions. Discs have exact radius1/2. For a rectangle with endpoint a, vector d and
L2=d.d, membership is 0<=dot(p-a,d)<=L2 and cross(p-a,d)^2<=L2/4. Linear dot/cross extrema over a
cell occur at its corners. Bound all those expressions with integer min/max; squared cross extrema
provide sufficient full-cell inclusion/exclusion tests. For discs use exact nearest/farthest box
squared distances to the center. Unknown means unresolved, never a guessed inside/outside.

A feature is certainly inside if one primitive contains the cell, certainly outside if all exclude
it, otherwise unknown. This is conservative but incomplete for a union. Process features backwards:
retain each uncertain later feature's color; a certain feature contributes its color and hides all
earlier features. If none is certain, retain background too. Per-channel min/max of possible pointwise
colors bounds the continuous color everywhere in the cell. No averaged-alpha compositing.

Dyadic cell areas have integer units4^(D-depth) at fixed D. Pixel bounds are sums of color*area units,
with denominator255*4^D. Subdivision preserves exact area and replaces a parent bound with children;
assert all child color intervals are nested in parent intervals. Unknown area remains explicitly
charged on budget exhaustion. Positive-area unresolved boundary cells can be loose; measure-zero
boundary inclusion conventions cannot invalidate a conservative area enclosure.

Canonical outputs are exact integer lower/upper numerators and denominator. Displayed float widths,
radii, resource clocks and reporting are not arithmetic certificates. Midpoint normalized error radius
is (upper-lower)/2; historical precision1/10240 is compared by integer inequalities, not floats.
A zero-width interval is exact for this input model. The method has a mathematical enclosure argument
and exact-arithmetic numerical implementation, but is not formally verified software; correctness
assumes CPython integer/IEEE decoding semantics and the validated implementation. It does not certify
uncertain upstream coordinates or make a claim about rendering A/B.

## Bounds fixed before geographic probes

Four conditions, no substituted tiles:
- full view4-r440-relief, (0,32)
- full view0-r220-map, (16,36)
- isolated feature index1 of view4-r440-map, (2,57)
- isolated feature index2 of view0-r220-map, (39,43)

Per process: maximum2,000,000 visited cells/tile, maximum quadtree depth16,90s adaptive-work deadline,
384MiB address space, CPU soft105/hard110s and elapsed120s hard guard. Depth16 describes spatial
boundary subdivision, NOT a new sampling grid: no frozen sampled-oracle call exceeds1024 and that
experiment is never resumed. Per-pixel largest-uncertainty cells refine first; a process-wide tile
cell/time budget is retained even after stopping. Never discard a difficult pixel. The tile remains
valid but wide when resources stop refinement. Resource times describe reference feasibility only,
not rendering performance. At most four tile processes; no new geographic observations.

Stop a pixel when its maximum enclosure width<=2/10240, giving midpoint radius<=1/10240. Compare
local radius,64-pixel mean-error uncertainty and twice that mean for any paired difference separately.
Neither success nor a narrow tile average changes the original worst-pixel admission gate.

## Controls and known limitations

Independent Fraction-based test-only polygon clipping provides near-horizontal rectangle area;
axis-aligned rectangle/caps, coincident and partially overlapping differently colored features have
closed-form rational areas. Disc area uses a rational decimal bracket for pi and checks enclosure
of that full bracket. Tangencies and reflected equal-area regions are checked. Initial controls
incorrectly demanded identical adaptive endpoints under reflection; priority-queue tie order can
produce different valid endpoints. The failed assertion/source are preserved; corrected test requires
both to contain the analytic area and intervals to overlap. No geographical result drove that edit.

RGB565 once-quantized outcome ranges can be derived by monotone rational nearest quantization of
continuous channel intervals. Sequential composition needs distinct per-feature coverage intervals,
8-bit alpha rounding possibilities and discrete integer blend propagation; final full-color intervals
alone do not recover those inputs. This pilot does not implement a complete sequential scorer.
Exact-x/event-split-y integration is a comparison alternative only, not implemented.
