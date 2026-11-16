# Reciprocal square root, PowerPC acceleration and CPU-specific kernels — experiment record (#145)

**Status:** Non-normative research record, 2026-11-16. Refs #145 (also #64, #97, #2). Nothing in
this document changes production behaviour, release scope, an ADR or a supported platform. Agent-authored
analysis is evidence, not authorization (AGENTS.md section 2).

**Method:** the owner's `systems-analysis` skills (`representing-and-intervening`, `causal-analysis`,
`propagation-trace`, read from `jackwillis/claude-plugins` at the commit recorded under "Provenance"):
Represent → Predict → Intervene → Observe → Update. Scale of formality: this is a controlled
software comparison on one machine, so a DAG is used only where a confounder is plausible (the
benchmark section), and `propagation-trace` is used for the one shared interface touched by any
candidate (`inverse_length`).

> **This first section was committed before any measurement was taken.** Part 1 is the
> pre-registration. Parts 2 onward are appended in later commits and never edit Part 1. Where an
> observation contradicts a prediction, the contradiction is reported next to it.

---

# Part 1 — Pre-registration (written before measurement)

## 1.1 Represent: what the code does

`inverse_length(l2)` (src/render/render.cpp) returns an approximation of `1/sqrt(l2)`:

```
if (l2 < 0.5) return 1.0 / std::sqrt(l2);
s = 1.5 - 0.5*l2;            // one Newton step from s0 = 1
s *= 1.5 - 0.5*l2*s*s;       // second Newton step
```

It normalizes interior points of a great-circle arc. `draw_arc` interpolates `v = a(1-t) + b*t`
between the two (unit-length, clipped) arc endpoints and scales by `inverse_length(|v|^2)`.

For unit `a`, `b` at angle `phi`, `|v|^2 = 1 - 2t(1-t)(1 - cos phi)`, so
`l2 in [cos^2(phi/2), 1]`. `l2 < 0.5` iff `phi > 90 deg`.

Newton for `y = l2^(-1/2)` has relative-error map `e' = -(3/2)e^2 - (1/2)e^3` (signed, with
`s = y(1+e)`), and the seed `s0 = 1` has `e0 = sqrt(l2) - 1`. All errors are therefore **negative**
(the approximation is always low, the interior point is pulled toward the sphere's centre).

Competing model of "why the comment and the code disagree":

* **M1 (preferred):** the comment describes the *reachable* domain (short arcs); the code header
  says "for l2 in [0.5, 1]". Only the stated domain is wrong, and the rendering is unaffected.
* **M2 (alternative, the issue's implied reading):** the approximation is materially wrong for
  real inputs and draws visibly shrunken arcs.

Frame check (representing-and-intervening, frame-agreement gate): the issue frames the finding as
a "numerical audit finding to verify". The alternative that would produce the same observed
discrepancy is M1, in which a documented domain is far wider than the data ever reaches. M1 and M2
differ on one measurable quantity: the largest arc angle `draw_arc` is ever given.

## 1.2 Predictions (numbers fixed before running anything)

| ID | Claim under test | Prediction | Refuted if |
| --- | --- | --- | --- |
| P1 | At `l2 = 0.5` the code returns 1.38671875 | exactly that value (hand calculation: 1.25 * (1.5 - 0.5*0.5*1.5625)); relative error -1.944% | any other value |
| P2 | Comment "<2e-3 after one step, <1e-5 after two" | false on [0.5, 1]; one step is 11.6% off at 0.5. Two-step error is below 1e-5 only for `phi <= ~34 deg` (`l2 >= ~0.917`) | the 1e-5 bound holds below `l2 = 0.9` |
| P3 | Production arc angles | bundled atlas segments <= ~6 deg, graticule 3 deg, so `l2 >= 0.9975` and "exact path" (`l2 < 0.5`) is dead code for shipped data | any shipped arc over 20 deg |
| P4 | Error on the production domain | max relative error <= 1e-10 (leading-order estimate 8e-12), always negative; screen displacement < 1e-6 px at radius 440 | max error > 1e-9 |
| P5 | Band margin `1e-4 * R` (`band_margin_px`) | holds for `phi` up to ~45 deg, so band rejection is safe for all shipped data | margin exceeded for `phi < 30 deg` |
| P6 | Rendering effect of using exact `1/sqrt` instead | framebuffer checksums identical on every sampled camera. Expected pixel flips: displacement 1e-9 px times about 4e4 sampled coordinates is about 4e-5 flips per frame | any checksum differs on a production trace |
| P7 | Host speed of current helper vs `1.0/std::sqrt` | current is faster in an isolated throughput loop (x86 sqrtsd+divsd, about 4-5x on dependency chains); the hardware-sqrt host is NOT a model of the G4 (sqrt is 1,233 ns there, docs/perf/README.md) | current is not faster in latency mode |
| P8 | Whole-render effect on host | `inverse_length` is under 1% of frame time; any variant moves whole-render median within noise (< 2%) | a variant changes whole-render median by more than 3% reproducibly |
| P9 | `frsqrte` as the seed | on the reachable domain the constant seed 1 (error < 1.3e-3) is a *better* seed than the 5-bit `frsqrte` estimate (error up to 1/32), so `frsqrte` plus refinement needs more steps than the existing code for the same bound | frsqrte-seeded result needs fewer flops for equal error on `l2 in [0.99, 1]` |
| P10 | Host/G4 bit identity | `frsqrte` is an *estimate* whose table is implementation-defined within an error bound; using it in a pixel-affecting path would put host and G4 goldens at risk, unlike plain IEEE arithmetic | not testable without hardware; recorded as an unverified claim |
| P11 | Portable improved candidate (C) | a one-step or quadratic seed over a *declared narrow domain* (`l2 >= 0.99`, `phi <= ~16 deg`) matches the current error on the production domain with fewer flops, with an exact fallback outside | C cannot hold 1e-9 on `l2 >= 0.99` |
| P12 | SIMD | AltiVec has no double-precision lanes; every projection in the renderer is `double`. A float batch kernel would change numerical results, so it cannot meet the "pixel identical" bar of #145/#64 without a precision-contract change | AltiVec double support found (it does not exist on 7400/7450) |
| P13 | Benefit of any rsqrt change at whole-render level on the G4 | below noise, because `inverse_length` is called only for interior points of arcs longer than `max_chord_px / radius` (a few thousand calls per frame) while a settle costs hundreds of milliseconds | native measurement needed to refute; none is available here |

## 1.3 Treatments

A accurate `1.0 / std::sqrt(x)`; B current helper, unchanged; C improved portable (declared
narrow domain); D PowerPC `frsqrte` plus refinement (compile and static analysis only; no hardware);
E compiler-generated: optimizer-friendly source, and `-fno-math-errno` scoped to the harness only.
Extra: C2 second-order Taylor in `1 - l2` (no Newton, no divide).

## 1.4 Variables, controls, confounders (benchmark design)

Treatment: which function computes the scale. Outcomes: (a) numerical error; (b) per-call time
(throughput over independent inputs; latency over a dependency chain); (c) whole-render time and
framebuffer checksum; (d) emitted code size.

Confounders held by design, not adjusted for: machine, compiler binary and flags (one TU per
variant, identical flags, compared within a binary where possible), input generator and seed, CPU
frequency and neighbours (this is a shared 2-core cloud VM: **thermal and neighbour noise is the
main threat**, handled with interleaved A/B/B/A blocks, a warm-up, and reporting medians and ranges),
dead-code elimination and constant folding (inputs from a runtime-seeded array; results folded into
a checksum consumed after timing; asm barrier on the loop output), branch-predictor warm state
(inputs fixed per run order), and unrealistic input distribution (the benchmark uses both the
production distribution `l2 in [0.9975, 1]` and the declared full domain `[0.5, 1]`, reported separately).

Not controlled and therefore not claimed: native G4 timing, cache behaviour of a 467 MHz G4,
Retro68 `-Os` code generation, bare OS 9 anything. Host timing is **not** G4 evidence.

## 1.5 Decision rules (fixed before results)

1. A portable change to `inverse_length` is only *proposed* if: max screen displacement over the
   production domain is <= 0.01 px at radius 440 **and** every pixel checksum in the full-render
   traces equals the baseline **and** the stated error bound is proven (analytic) and tested.
2. A micro-benchmark win alone never justifies a change. Whole-render time must not regress; a
   whole-render gain smaller than the run-to-run range is reported as "no measurable effect".
3. A CPU-specific path is only recommended if it (i) is behind a capability probe with a portable
   fallback, (ii) preserves cross-platform bit identity of goldens, and (iii) has a native
   measurement showing a whole-render gain. Without (iii) the answer is "defer pending hardware".
4. A `globe::math` layer is recommended only if at least two call sites need the same operation with
   *different* precision contracts. Otherwise: no library.
5. A correctness-sensitive `std::sqrt` (picking, camera orthonormalization, rasterizer coverage,
   boundary tests) is never replaced on the strength of this experiment.
6. No test is deleted or loosened; no golden is re-pinned.

## 1.6 What I will not claim

Native-G4 performance, Retro68 behaviour, `frsqrte` accuracy on a real 7400 (qemu's `frsqrte` model
is not the hardware table), or OS 9 behaviour. If the Retro68 and NetBSD toolchains cannot be
reached from this environment, the report says so and gives the exact owner-side commands.
