# PE-AA-01 certification amendment v1

2026-11-18. **Owner-authorized decisions recorded; submitted for steering review.**
This separately versioned supplement governs future certified-quality instrumentation only.
It authorizes no reference generation, candidate inspection, scoring or production acceptance.
See [precedence](pe-aa-01-certification-precedence-v1.md) and
[traceability](pe-aa-01-certification-traceability-v1.md). Historical evidence remains unchanged.

## 1. Reference admission and three distinct states

Replace successive-grid agreement as an accuracy/admission certificate with independently
verifiable rational per-pixel, per-channel enclosures of the continuous target: L <= R <= U.
Use exact integer numerators and positive denominators; normalize encoded RGB to [0,1].

- **VALID:** supported inputs, target semantics and provenance are verified, and bounds enclose
  the target. Missing/partial/failed pixels are explicitly unknown, never implicit background.
- **PRECISE:** a valid enclosure meets its separately declared operational midpoint-radius target.
  A valid wide enclosure is not a numerical failure or evidence against B.
- **DECISION-RESOLVED:** propagated bounds prove the applicable decision, including eligibility.
  Meeting a precision target need not resolve a decision; wider bounds can sometimes resolve it.

A threshold-straddling comparison is inconclusive. Numerical implementation assumptions and
independent verification must support the enclosure claim; empirical agreement alone cannot.

## 2. Exact endpoint-defined target

Interpret every original finite binary64 fixture endpoint exactly as its dyadic rational value,
without further coordinate rounding. Preserve width-one strokes, radius-1/2 interior join discs,
feature union, butt caps, closed edges, duplicate-closure handling and original opaque last-writer
feature order. Preserve the frozen background and source RGB565 bit expansion, normalized by 255.
The continuous target integrates samplewise opaque color, not averaged-alpha compositing.
Historical floating-normal/sample outputs are not asserted bit-identical to this ideal target.
Unsupported inputs must be rejected explicitly; no silent geometry repair or fixture change.

## 3. Exact discrete outcomes

Once-quantized Q(R) uses exact half-up rounding floor(M*r + 1/2), M=31,63,31, including ties.
Expand five-bit codes by (v<<3)|(v>>2), six-bit codes by (v<<2)|(v>>4).
Keep reference/quantizer dependence when bounding losses; independently choosing R and Q is not
an exact achievable outcome. Enumerate all possible codes at an unresolved rounding boundary.

The sequential control preserves original feature order and frozen integer blending. For
coverage a, q=floor(255*a+1/2); the SAME q drives all channels. For expanded source s and
destination d, z=(q*s+(255-q)*d+127)//255 and packed channel=(z*M+127)//255.
Preserve the frozen q=0 destination and q=255 source endpoint behavior. Carry packed-state sets
through ordered features; do not combine independently favorable channel/alpha outcomes.
These exact semantics are distinguished from historical float64 implementation artifacts.
Sequential compositing is a separately reported control, not the continuous opaque target.

## 4. Operational resolution

Retain 1/10240 as the planned fine-reference midpoint-radius target, subject to separate resource
authorization. Historically min(1/1024, 0.05*0.001953125)=1/10240 specified a successive-grid delta.
Equal numerical values do not make delta agreement an accuracy certificate: an enclosure radius
is a different, stronger claim. Preserve owner margin 0.001953125. The planned radius is neither
an additional universal validity gate nor an acceptance threshold.

## 5. Frozen primary mask

Freeze literal quality_only.neighborhood output, its source/dependency hashes, geometry hash,
runtime/floating-point provenance, dimensions, byte encoding, count and output hash BEFORE
candidate inspection. Do not replace its floating construction with idealized dilation/support.
For view5-r220-relief the frozen mask is 640x480 row-major uint8 0/1, 307200 bytes, 74796 included
pixels, SHA256:

`68c4c2ff39983467819eb8f4d06a6e04713c946431b1f7e08b5bc6f0bb23fcd7`

The mask-generating quality_only.py SHA256 is
`30b87445f821bfd1d58cb6a200fdda60cc77ff0b9437b035a42a103ada728dea`.
Its complete input/dependency/runtime provenance is in the reviewed masked audit. Every other
fixture requires its own candidate-blind mask freeze; this amendment does not generate any.

## 6. Unchanged point criterion and local guards

For each geometry i and condition/style s, E_X is normalized RGB MAE over that fixture's frozen
primary mask: sum(abs(X-R))/(3*mask_pixel_count). A/B are expanded frozen candidate pixels;
Q is once-quantized R. Preserve 16 equally weighted geometry IDs, Map and Relief separately.
For each style define N=mean_i(E_A-E_B), D=mean_i(E_A-E_Q), G=N/D, H=N-D/5.
The original 20% point target is satisfied only when D.lower>10/1024 and H.lower>=0.
With proved denominator eligibility, H.upper<0 proves failure; otherwise unresolved is inconclusive.
D.upper<=10/1024 is floor-limited/undefined; a crossing denominator is inconclusive. Equality at
20% passes; equality at the denominator floor is undefined. Do not clip negative excess error,
drop individual floor-limited geometries, pool pixels across geometries or average individual ratios.

For every one of the 32 conditions, J=E_B-E_A-max(E_A/10,2/1024).
Here baseline error means A's MAE under that condition's frozen PRIMARY mask.
J.upper<=0 proves the regression guard; J.lower>0 proves a violation; otherwise inconclusive.
Equality passes. Both terms of the original limit remain. A condition cannot be pooled away.

Propagate the SAME uncertain reference through all losses and quantizer outcomes. Scalar
absolute-loss extrema include interval endpoints and candidate-value kinks; quantized residuals
also split at exact quantizer boundaries. Certified shared-reference H/J bounds are required;
bootstrap midpoint estimates or independent reference-noise draws cannot supply them. Conservative
relaxations must be identified as such, never described as jointly achievable extrema.
Numerical satisfaction does not automatically accept B: original blinded native legibility,
material-stroke and other unchanged acceptance requirements remain distinct.

## 7. Bootstrap and separately reported controls

Retain the frozen paired 16-ID resampling policy: 100000 draws, seed 20261118, Map/Relief retained
together, ratio of means per style. Repeated IDs share their reference, not new independent noise.
Keep the 5% undefined-replicate threshold (exactly 5% allowed), strict denominator floor and
frozen schedule/provenance. Report counts per style, including definite and possible undefined.

For n draws let B be definitely undefined (D.upper<=floor), A definitely defined
(D.lower>floor), U ambiguous. If |B|/n>5%, the interval is inconclusive. If
|B|/n<=5%<(|B|+|U|)/n, threshold eligibility is unresolved: inconclusive.
Otherwise use the reviewed bounded-subset percentile enclosure. For every k=0..|U|, take the
.025 quantile of A.lower plus the k smallest optional lower bounds, and the .975 quantile of
A.upper plus the k largest optional upper bounds. Minimize lower/maximize upper across all k,
recomputing ranks with exact rational linear interpolation at probabilities 1/40 and 39/40.
Optional bounds must be valid conditional on D>floor. Using the closure at the floor for an
infimum/supremum does not establish eligibility there. Missing conditional bounds are inconclusive.
The point denominator must also be proved eligible. Report joint-feasibility relaxation where
optional subsets include shared-reference-infeasible combinations; never choose a favorable subset.

These intervals remain selected-view paired sensitivity evidence, not a population-coverage
claim. Bootstrap intervals and sequential RGB565 controls are separately reported evidence;
neither becomes a new acceptance gate. Bootstrap inconclusiveness does not itself negate an
otherwise resolved point/local result, and does not excuse missing required reporting.

## 8. Support, peak and visibility diagnostics

Intended future isolated-feature support is positive-area feature-union coverage a>0 AND positive
source/background contrast s>0. With coverage bounds, a.lower>0 proves support, a.upper=0 excludes
it, and otherwise eligibility is uncertain; zero contrast is ineligible. Preserve historical
sampled sidecars as explicitly labeled sampled-support observations (any(sampled_ref!=background));
do not reinterpret their bytes as exact positive-area support.

Retain separate candidate/reference peak contrast, source ratios and peak retention over the SAME
eligible pixels, permitting different maximizing positions. Possible-support pixels remain in the
possible peak upper bound. Zero reference peak gives undefined retention, not an epsilon quotient.
Uncertain support or zero eligibility requires explicit definite/possible/undefined reporting.
Preserve original thin-feature probes, contrast eligibility and equality behavior; diagnostics
cannot substitute for blinded native legibility review. Further isolated-feature certification
requires separately authorized per-feature coverage evidence. Full-color bounds alone cannot
recover coverage of hidden features. Motion stability remains explicitly deferred as before.

## 9. Minimum future interval-certificate input contract

A separately frozen adapter/verifier must require:

- Protocol/engine/verifier versions and source hashes; exact fixture identity, geometry ID/style,
  dimensions/pixel convention, geometry/background/source-order hashes and numerical model.
- Bounded canonical integer encodings for per-channel rational bounds, positive denominators,
  0<=L<=U<=1, unique tile/pixel identity, complete required coverage and explicit unknown/failure
  states; no partial certificate promoted to complete. Independent verification evidence.
- Operational radius, precision and decision statuses separately; quota/depth/resource stops,
  supported-domain checks, numerical assumptions and exact-arithmetic provenance.
- Frozen literal mask bytes/hash/count and source/input/runtime provenance; exact quantizer,
  expansion/blend definitions. Later candidate/build hashes and exact same-host A replay must
  precede any separately authorized comparison; blind identities remain private.
- Additional feature-identified, ordered coverage enclosures for sequential/isolated controls;
  coupled alpha/packed outcomes and unresolved membership/zero denominators explicitly retained.

Retain the frozen full-frame MAE, full-frame worst pixel-channel error and outside-mask reporting
as separate summaries: propagate sums/maxima conservatively, without substituting them for the
primary-mask rules. Undefined feature aggregation must remain explicitly labeled, not silently
resolved by dropping features.

Exact bounds govern decisions; floating displays must not shrink them or create epsilon passes.
The original quality_only.py consumes convergence.json, continuous.npy and single packed control
arrays; it cannot consume interval certificates unchanged. Do not disguise certificates as those
historical outputs. No new geographic scorer is implemented or authorized here.

## Remaining requirements before an admissible comparison

Implement and independently review/freeze the interval adapter/verifier and metric propagation,
including discrete controls and failure handling. Freeze every remaining primary mask. Obtain
separate resource/execution authorization for needed references and isolated-feature coverage;
verify all input/source hashes, complete certificates and exact A replay. Preserve blinding and
record native judgments separately. Obtain explicit geographic quality-scoring authorization.
No fine run, full 32-condition feasibility claim, threshold waiver or production acceptance follows.
