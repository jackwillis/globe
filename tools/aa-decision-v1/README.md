# PE-AA-01 coupled quality-decision mathematics v1

2026-11-19, Refs #25. Instrumentation only, synthetic evidence only. Parent: PR #195
at `a4360af6030a7e13cf8eacac9cbc9d42e6138b5d`; controlling protocol: certification
amendment v1 `9a2214c6286c54f3d451956abe105b9d33877845`. **No geographic scoring
authorization.** There is no candidate reader, CLI, reference generator or bootstrap.

`decision.py` identifies `PE-AA-01-coupled-decision-v1`. It consumes the same rows
as `aa-interval-v1` (`index, RGB lower ints, RGB upper ints, denominator`), a literal
0/1 mask and expanded RGB bytes, and returns exact `Fraction` ranges. Nothing here
changes a threshold, the historical protocol, the amendment, the adapter or renderer B.

## Quantities (per geometry/style condition, normalized by 3n over the primary mask)

| Name | Definition | Notes |
|---|---|---|
| E_A, E_B | `abs(X - R)` | X = expanded candidate / 255 |
| E_Q | `abs(Q(R) - R)` | Q(R) = once-quantized R, expanded, / 255; same R |
| N | E_A − E_B | shared R: identical candidates give exactly [0,0] |
| D | E_A − E_Q | shared R |
| H | N − D/5 = 4/5·E_A − E_B + 1/5·E_Q | optimized from ONE R per channel |
| F1 | E_B − 11/10·E_A | first branch of J |

Per style, over 16 equally weighted geometry IDs: N, D, H are the means of the
per-geometry ranges (independent references across geometries make the Minkowski
mean exact). `G = N/D` is the ratio of means, reported only as a conservative corner
quotient when D.lower > 0, never used for a verdict. Map and Relief are separate
criteria; `decide()` returns both and never pools them.

## Coupling and exact half-up ties

Code q owns reference values `[(2q−1)/2M, (2q+1)/2M)`: closed below, open above, so an
exact tie belongs to the upper code; the top cell is closed at 1. M = 31, 63, 31 for
R, G, B; codes expand by `(v<<3)|(v>>2)` and `(v<<2)|(v>>4)`. For each channel
interval `[L,U]` the reference is split into those half-open cells and each quantity
is a piecewise-linear function of R inside a cell. Extrema come from the cell ends and
kinks (A, B, expanded code) in that cell. R and Q are never chosen independently.

Each endpoint also carries an `attained` flag. A supremum at an open cell end is a
limit, not a value: e.g. on `[0, 1/62]` the supremum of E_Q is `1/62`, approached
below the tie and never attained. Sums keep a flag only if every term attains. The
value set can have gaps (Q jumps); decisions use the hull endpoints, so a hull that
straddles a threshold is INCONCLUSIVE even if the actual set avoids it.

## Decision rules (amendment v1 §6, exact rationals, no epsilon, no midpoint)

Denominator (per style, D): `FLOOR_LIMITED` if D.upper ≤ 10/1024, `ELIGIBLE` if
D.lower > 10/1024 (strict), otherwise `CROSSING`.

| Point criterion | Verdict |
|---|---|
| ELIGIBLE and H.lower ≥ 0 | PASS (equality at 20% passes) |
| ELIGIBLE and H.upper < 0 | FAIL |
| ELIGIBLE otherwise | INCONCLUSIVE (H straddles 0) |
| FLOOR_LIMITED | INCONCLUSIVE, reason floor-limited: no percentage conclusion |
| CROSSING | INCONCLUSIVE |

Attainment flags are reported but are not used to loosen a verdict: a bound equal to
a threshold keeps its literal rule (e.g. D.lower = floor is not eligible even if that
infimum is a limit). Whether an unattained limit may decide strictly is a question for
steering, not decided here.

Local guard (per condition, A's frozen primary mask): `J = E_B − E_A − max(E_A/10, 2/1024)
= min(F1, −N − 2/1024)`, so both branches of the maximum remain. `J.lower` is the exact
minimum of the branch infima. `J.upper` is exact when one branch provably governs
(E_A.upper ≤ 20/1024 or E_A.lower ≥ 20/1024); otherwise it is the conservative minimum
of the branch suprema, labelled `upper_exact: False`. PASS if J.upper ≤ 0 (equality
passes), FAIL (violation) if J.lower > 0, else INCONCLUSIVE. Tightness of the relaxed
upper bound is not proved; no looser case was found in the author's randomized searches.

`numerical_conjunction` is the three-valued AND of required numerical criteria (any
FAIL → FAIL, all PASS → PASS, else INCONCLUSIVE). It is not acceptance of B: blinded
native legibility, material strokes and every other original requirement remain.

## Relaxations (stated, never claimed as jointly achievable extrema)

- Cross-pixel/channel correlation induced by geometry (inherited from aa-interval-v1).
- `J.upper` in the mixed branch; `G_conservative`; hull gaps across Q jumps.
- Reference enclosure validity is conditional on the pinned reviewed producer, as in #195.

## Reproduction (Linux x86-64, Python 3.14.8, stdlib only)

```sh
sha256sum -c tools/aa-decision-v1/source-freeze.sha256
python3 -B -m unittest discover -s tools/aa-decision-v1 -p 'test_*.py' -v
python3 -B tools/aa-decision-v1/check_mutations.py   # writes nothing; compare mutation-results.json
bash tools/check-policy.sh
```

See [REPORT.md](REPORT.md) for results, omissions and open questions.
