# Bounded-subset percentile propagation — proposed instrument, not a new gate

Supersedes only the earlier proposal's blanket `any ambiguous => inconclusive` suggestion.
The original proposal/source/evidence remain unchanged. No geographic bootstrap was executed.

For each already specified paired-ID resample and style, recompute ratio of mean excess errors,
not the mean of per-geometry ratios. Retain all16geometry IDs and Map/Relief together. Repeated
occurrences of an ID use the SAME uncertain reference; no independent reference noise is drawn.
The frozen100000draw seed/schedule is unchanged in principle; these tests use synthetic supplied
schedules, not a replacement geographic RNG.

Let B be definitely undefined (D.upper<=10/1024), A definitely defined (D.lower>10/1024), and U
ambiguous. For an ambiguous draw, compute a G enclosure conditional on D>floor. For example,
use the closure [floor,D.upper] with a valid numerator enclosure; equality in this closure is
only for infimum/supremum, never proof of eligibility. Missing conditional ratio bounds block
an interval rather than being ignored.

- If |B|/n>5%, inconclusive by the frozen rule.
- If |B|/n<=5%<(|B|+|U|)/n, threshold eligibility is unresolved: inconclusive.
- If (|B|+|U|)/n<=5%, every independently optional membership subset respects the threshold.
  Exactly5% is allowed. The original point denominator must also be proved strictly eligible.

For each k=0..|U|, the smallest .025 quantile over k optional draws is obtained by adding the k
smallest G.lower values to the mandatory A.lower values. The largest .975 quantile is obtained
by adding the k largest G.upper values to A.upper. Minimize/maximize over ALL k, recomputing rank
and linear interpolation at each resulting sample size. These are endpoint envelopes, not a
selection of one favorable subset whose interval is then reported.

Proof: for fixed k, choosing the k smallest optional values makes every merged order statistic
no greater than any other k-subset. Linear interpolated quantiles are monotone in sorted values.
Reverse the inequality for the k largest values. Enumerating every k covers every possible size.
The minimum lower endpoint and maximum upper endpoint therefore enclose the percentile interval
for every feasible defined subset and reference realization.

Per-draw interval classification can relax correlations between replicate memberships. The
implementation includes ALL optional subsets, even if the shared geometric reference makes
some joint choices impossible. This can widen the envelope but cannot exclude a feasible world;
it is not an independence assumption or a claim of globally tight reference feasibility. The
returned `joint_feasibility_relaxed` flag makes this explicit. Tests include shared-latent worlds.

Implementation sorts endpoints once, then obtains each quantile's one/two order statistics by
binary partition of two sorted arrays. It never materializes2^|U| subsets or repeatedly sorts
n-length lists. Complexity O(n log n+|U| log n), memory O(n), input bounded to100000replicates.
When the5% threshold is resolved eligible, |U|<=5000 at the frozen schedule. Runtime at the full
schedule is an algorithmic bound, NOT a measured geographic bootstrap performance claim.

Independent tests exhaust1728membership subsets across180random/adversarial cases (up to five
ambiguous draws) and match exact rational endpoints, not containment alone. They cover ambiguous
extreme values at either tail, exactly5%, above5%, threshold straddling, point-floor equality,
nonintegral5%counts, duplicate/tied endpoints and16-ID paired ratio-of-means. The2000replicate,
100ambiguous stress case evaluates101sizes. A separate original-suite test covers heterogeneous
paired shared-reference worlds. Quantile arithmetic uses exact1/40,39/40 and rational linear
interpolation; it does not claim bitwise identity with NumPy float percentile artifacts.

Bootstrap intervals remain conditional selected-view sensitivity evidence, not95% population
coverage guarantees and NOT an added20% acceptance gate. A valid subset envelope can still be
wide or scientifically inconclusive. No threshold, point target, candidate or fixture is changed.
