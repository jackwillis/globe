# Coupled quality-decision mathematics v1: steering handoff

2026-11-19, Refs #25 (does not complete it). Claude Code. Worktree
`/home/jack/code/globe-aa-decision`, branch `claude/pe-aa-coupled-decision-20261119`,
parent PR #195 head `a4360af6030a7e13cf8eacac9cbc9d42e6138b5d`. New directory
`tools/aa-decision-v1/` only; nothing historical, no adapter, amendment, threshold,
CI or renderer file is touched. Synthetic evidence only: no candidate bytes, geographic
scores, reference generation, bootstrap, fine run, G4 or production.

## What is implemented

1. **Coupled R / Q(R) loss bounds.** Reference channels are split into half-open
   quantizer cells (closed below, open above, exact tie → upper code, top cell closed
   at 1). E_Q, D and H are optimized per channel from one R, with attainment flags for
   open-cell limits. H is not formed as `N − D/5` from separate intervals.
2. **D and H** per geometry, then the 16-ID means per style (ratio of means kept;
   `G = N/D` is a conservative corner quotient only).
3. **J per condition**, `min(E_B − 11/10·E_A, E_B − E_A − 2/1024)`, preserving both
   branches of the original maximum. Lower bound exact; upper exact when one branch
   governs, else a labelled conservative relaxation.
4. **Strict floor eligibility and verdicts**: D.lower > 10/1024; floor equality is
   undefined; PASS/FAIL/INCONCLUSIVE with reasons; equality at H = 0 and J = 0 passes.
5. **Independent controls** (26 tests) and **21 in-memory mutation controls**.

## Evidence (Linux x86-64, Python 3.14.8, stdlib only)

- `python3 -B -m unittest discover -s tools/aa-decision-v1 -p 'test_*.py'`: 26 tests
  pass, 6.4 s, 21.6 MB peak RSS.
- `python3 -B tools/aa-decision-v1/check_mutations.py`: baseline passes and all 21
  mutants are killed (`mutation-results.json`, about 150 s). It edits no source.
- Controls (expected values are hand arithmetic or a lattice enumerator that never
  calls `scalar_range` or `cells`):
  - 400 random windows × 6 quantities (E_A, E_B, E_Q, N, D, H), including values and
    attainment flags and windows that cross ties; more than 20 have an unattained limit.
  - Hand ties: tie belongs to upper code; `[0,1/62]` has sup E_Q = 1/62 unattained; top
    cell; bit-replication values.
  - Identical candidates: N = [0,0] exactly for 40 random wide references and a fully
    unknown one, and `H == −D/5` exactly, so B = A is a provable FAIL, not inconclusive.
  - Coupled H equals the sum of independent per-channel enumerations and is strictly
    inside the relaxed `N − D/5` for the recorded case (it is never outside it).
  - **False midpoint verdicts**: one case where the midpoint reference gives PASS and
    one where it gives FAIL; both interval verdicts are INCONCLUSIVE with H straddling
    0, and both signs are realized by actual reference worlds.
  - Thresholds: H = 0 passes, one unit below fails; H.upper = 0 attained is not a
    failure, one unit lower fails; D at 3824/3825/3826 units of 1/(765·512): 3825 is
    exactly 10/1024 and is floor-limited; D crossing the floor is inconclusive even
    when H.upper < 0; D ≤ 0 yields no ratio.
  - J: floor-branch and 10%-branch equality and ±1 unit; J.lower = 0 with J.upper > 0
    is not a violation (found by a surviving mutant); 120 mixed-branch worlds
    enumerations check the exact infimum and a sound supremum; unknown reference with
    identical candidates passes.
  - Ratio of means differs from the mean of ratios on 16 heterogeneous exact geometries;
    Map and Relief are never pooled; invalid IDs/styles/frames are rejected; reference
    quantities agree with `aa-interval-v1.fixed_mask` on random frames.
- One real finding: the first mutation pass left `J.lower > 0 → ≥ 0` alive. A control
  was added and the 21-mutant run then killed all.
- `bash tools/check-policy.sh` and `sha256sum -c tools/aa-decision-v1/source-freeze.sha256`
  results are recorded in the PR.

## Not implemented (exact list)

- Any geographic candidate reader, A/B scoring, exact same-host A replay, candidate
  and build hash admission.
- Bootstrap (100000 draws, bounded-subset percentile enclosure, undefined-replicate
  accounting), sequential/packed RGB565 compositing controls, support/peak/visibility
  diagnostics (amendment §§7–8), the full-frame MAE, worst pixel-channel and
  outside-mask summaries (§9), and remaining masks (only view5-r220-relief's is frozen).
- Joint-feasibility tightening across pixels/channels, a proof that `J.upper` is tight,
  and a hull with Q-jump gaps removed.
- Scale: the cost is about 1.9 ms per uncertain masked pixel on synthetic frames
  (about 146 s per condition, extrapolated, not measured); no caching of repeated
  pixel tuples and no measurement on the 74796-pixel mask or the real certificates.
  Rows from `bounds()` are accepted by contract but this slice never ran against the
  real evidence.
- Native legibility, material-stroke and all other unchanged acceptance requirements.

## Decisions requested from steering

1. FLOOR_LIMITED is reported as INCONCLUSIVE (undefined, never PASS, never FAIL),
   following "no percentage conclusion". Confirm, or specify a FAIL/undefined state.
2. May an unattained limit decide strictly (for example D.lower = floor as an
   unattained infimum, or H.upper = 0 unattained)? Implemented literally: no.
3. Is `numerical_conjunction` wanted, or should the two styles and 32 guards be
   reported only separately?
4. Accept the labelled `J.upper` mixed-branch relaxation, or require an exact solver?
5. Issue #25 still has `status: in review` for PR #195; I left the label unchanged.

Inherited CI failure: the `Checks` job fails clang-format on
`tools/aa-experiment/synthetic_driver.cpp`, from ancestor commit 2955bed. Unrelated;
not modified.

**Stop for steering review.** Next narrow step after approval: independent review of
`decision.py` and the frozen test set, then freeze or request corrections.
