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

---

# Part 2 — Results (appended after measurement; Part 1 is unchanged)

Platform for everything below unless stated: x86-64 Intel Xeon 2.10 GHz, 2 vCPUs (shared cloud VM),
Linux 6.18, g++ 13.3 and clang++ 18.1.3, `-O2 -DNDEBUG`, base commit `0b5b16d`. PowerPC evidence is
**GCC 12.4 `powerpc-linux-gnu` cross-compilation and qemu-user 8.2.2 only**. None of it is G4,
NetBSD, Retro68 or OS 9 evidence. Raw data: `docs/perf/rsqrt-20261116/` (index in its README);
code: `tools/rsqrt-research/`. Full provenance: `docs/perf/rsqrt-20261116/PROVENANCE.txt`.

## 2.0 What ran and what did not

| Item | Result |
| --- | --- |
| Baseline host suite, unmodified main (GCC 13, `-O2`) | 68/68 pass |
| Same suite with each scratch variant of `inverse_length` (exact, one-step, Taylor, seeded, guard-moved) | 68/68 pass for the four variants run in full (A, C1, C2, C4); B2 compared by frame sweep only |
| ASan+UBSan (`-O1`), focused tests (render, line_layer, refine, terrain, projection, shaded, zoom, raster, framebuffer, app: 10 tests) + frame sweep, variants C2 and C4 | 10/10 pass, sweep hash equals baseline, no reports |
| ASan+UBSan on the numerical harness (`audit`) | clean; output identical to the unsanitized build |
| Pixel goldens | pass for every variant, **including C1, which changes pixels (see 2.5)** |
| Numerical audit, dense (4.2M points per candidate, long-double reference) and on 34.3M logged production arguments | done |
| Micro-benchmark: g++ `-O2`, `-O2 -fno-math-errno`, `-O3`, clang `-O2` | done (quiet-machine medians, 21 interleaved blocks, order reversed on alternate blocks) |
| Whole-render instruction counts (callgrind), wall time A/B/B/A | done; wall time too noisy to resolve effects under about 5% (ranges of 10-20%) |
| PowerPC assembly probes, GCC 12.4, `-mcpu` generic/603e/750/7400 | done |
| Real renderer TUs compiled for PPC32 (`render`, `raster`, `terrain`, `projection`) | done (library-call census) |
| PPC32 frame sweep under qemu-user versus x86 | done |
| AltiVec/SSE autovectorization probe for batch projection | done (compile only) |
| **Retro68 PowerPC and 68k compile probes** | **not run.** No container runtime, `ghcr.io` and GitHub release downloads return 000/403 from this environment. Commands for the owner in 2.9 |
| **NetBSD/macppc GCC 12.5 cross toolchain** | **not available.** Ubuntu's GCC 12.4 `powerpc-linux-gnu` was used as the nearest proxy (same GCC series; Linux ABI, glibc) |
| **Anything on the G4** | **not run, not authorized.** No G4 access was attempted |

### 2.0.1 Post-registration items (not in Part 1)

Part 1 (committed in `4429b88` before any measurement) is unchanged by this revision. The following were **added after seeing data** and are
exploratory, not tests of a pre-registered prediction. Where they appear below they are tagged **[post-reg]**.

| Item | Where it came from | Status of the evidence |
| --- | --- | --- |
| **B2** (guard moved from `l2 < 0.5` to `l2 < 0.92`, scratch variant 6) | Chosen after the audit showed the 1e-5 bound crosses at `l2` 0.918. Part 1 lists treatments A-E and C2 only; B2 is not among them, and 0.92 was picked by me from measured data. | Host x86-64 only: 1,764-frame sweep identical to baseline; full suite **not** run with B2; **not run on PPC32/qemu**. Its decision-rule-1 checksum condition is therefore met on x86 only. |
| **FMA contraction difference** (x86 vs PPC32) | Found while checking cross-target bit identity for P10, which predicted a `frsqrte`-table hazard, not contraction. | qemu-user, GCC 12.4, glibc, Linux ABI. Not reproduced on NetBSD GCC 12.5 or a 7400. |
| **Band-margin safety argument** (one-signed error stays inside the `0.31 R |a-b|^2` slack) | P5 predicted only where the `1e-4 R` margin holds (confirmed). The argument that band rejection is therefore still safe at larger arcs was added afterwards. | Reasoning from one-signed error, **not** an exhaustive test of `arc_misses_band`; no such test was built. For shipped data (arcs <= 5.7 deg) the margin itself holds, so the argument is only needed for arcs the data does not contain. |
| **G4 cost model** (2.5) | Written after the call counts were known. | Arithmetic on a repository-measured `sqrt` time and my recollection of 7400 FP latency, which is **unchecked against the MPC7400 manual**. A model, not a measurement. |
| C1 and C4 as named variants | Part 1 says only "improved portable". | Their rows in 2.3 are exploratory. |

## 2.1 A. Numerical audit

**P1 confirmed exactly.** `inverse_length(0.5)` returns `1.38671875`; exact is `1.4142135623730951`;
relative error -1.94418%. One Newton step gives `1.25` (-11.61%), so the comment's "under 2e-3 after one"
is also wrong at the edge of its own domain. The first step is `s0 = 1.5 - 0.5*l2`; the second reduces the
error by `e' = -(3/2)e^2 - (1/2)e^3`.

**Reachable domain (P3 confirmed).** With `l2 = |(1-t)a + t b|^2 >= cos^2(phi/2)`:

* Bundled atlas (`tools/rsqrt-research/atlas_arc_stats.py data/atlas.gatl`): 7,653 segments, longest
  **5.696 deg**, 99.9th percentile 4.11 deg, median 0.57 deg, none over 10 deg. The graticule is sampled at 3 deg.
* Measured, not just derived: 34,316,338 `inverse_length` calls over 1,764 camera frames (7 radii from 55 to 2560 px,
  7 latitudes, 18 longitudes, 2 headings, all layers), with the argument logged. **Minimum `l2` = 0.997530894114 (= cos^2 of the
  longest arc), maximum 0.99999965, zero calls below 0.5.** The `1/std::sqrt` fallback is dead code for shipped data.
* The format permits any arc under 180 deg (docs/atlas-format.md), so a different atlas could reach the
  whole documented domain. That is the only way to meet the 2% error.

**Error (relative; negative = result too small).**

| Range of `l2` | Meaning | Worst relative error of B | At radius 440 / 2560 |
| --- | --- | --- | --- |
| 0.5 | 90 deg arc midpoint | -1.944e-2 | 8.6 px / 49.8 px |
| 0.704 | 66 deg | -2e-3 | 0.9 px / 5.1 px |
| 0.856 | 44.6 deg | -1e-4 | 0.04 px / 0.26 px |
| 0.918 | **33.2 deg** | **-1e-5** | 0.004 px / 0.026 px |
| 0.9917 | 10.4 deg | -1e-9 | 4e-7 px |
| **0.99753 (shipped data)** | **5.7 deg** | **-7.85e-12 (dense and logged)** | **3.5e-9 px / 2e-8 px** |

* **Bias:** one-signed. Over 1,048,577 samples of [0.5, 1] the result was never above the exact value and
  never below 1. The drawn point therefore always lies between its chord point and the sphere; it can
  neither cross the horizon nor leave the chord's hull. (Positive values of 1e-16 in the tables are rounding.)
* **The comment's claims** (P2): "under 1e-5 after two" holds only for arcs up to 33.2 deg; "under 2e-3 after one" only up to
  31.2 deg. Both hold for every arc the bundled data produces, so the **claim is true of the reachable domain and
  false of the stated domain [0.5, 1]**: model M1 (documentation describes the wrong domain) is supported, M2
  (visibly wrong arcs) is not, for shipped data.
* **`band_margin_px`'s comment** ("a Newton step good to 1e-5 of the radius", P5): the 1e-4 R margin is met up to
  44.6 deg, as predicted. At 90 deg the error (4.3 px at R 220) exceeds `2 + 1e-4 R`. But, because the error is one-signed, the
  drawn point lies on the segment between the chord point and the true arc point, which I **argue** (**[post-reg]**, reasoning only) is covered by the separate
  `0.31 R |a-b|^2` slack term, so band rejection should be safe regardless. The comment is inaccurate, and I found no harm for shipped data (arcs <= 5.7 deg,
  where the margin holds), but I did **not** build an exhaustive band test, so the claim for longer arcs is unverified.
* **Exceptional inputs** (identical to exact where it matters): `0` gives `+inf` and `-0` gives `-inf`; negative and NaN give NaN; subnormals
  agree. Outside the domain, B is wrong: `B(2) = 0.625` (exact 0.7071), `B(1e308) = +inf`, `B(+inf) = +inf` (exact 0). `l2` cannot exceed
  `1 + 2e-16` in the renderer, so none of this is reachable.

**Propagation trace** (`propagation-trace`, two hops). Change: the value returned by `inverse_length`.
Hop one (grep: one definition, one use): `draw_arc`, `render.cpp:84`. It scales an interior point which `to_screen`
turns into a coordinate for `draw_line`. Hop two: `draw_line` pixels; `RenderStats::lines_drawn` (the step count is computed from `len2` before
`inverse_length` is called, so the count is independent of its value: confirmed, 42,469,546 lines for all variants); callers
`draw_graticule`, `draw_atlas_layer`, `RefinementJob::step` (line layer). `clip_arc_to_front`, the visibility
decision and `arc_misses_band` do not read it. **Structurally compatible but behaviourally sensitive only through pixel
rounding at line coordinates.** That is what 2.5 measures; visibility cannot change by construction and no frame changed visibility in the sweep.

## 2.2 Predictions against observations

| ID | Observed | Verdict |
| --- | --- | --- |
| P1 | 1.38671875, -1.944% | confirmed |
| P2 | 1e-5 bound holds to 33.2 deg (predicted about 34); one-step 2e-3 holds to 31.2 deg | confirmed |
| P3 | longest shipped arc 5.70 deg; logged minimum `l2` 0.99753 | confirmed |
| P4 | 7.85e-12 (predicted 8e-12 estimate, bound 1e-10); always negative; 2e-8 px worst at R 2560 | confirmed |
| P5 | 1e-4 R margin holds to 44.6 deg | confirmed |
| P6 | exact `1/sqrt` gives 0 differing pixels in 541.9M | confirmed |
| P7 | throughput: B 3.4x faster than A (raw), latency only **1.5x** (1.7x net of loop overhead); I predicted 4-5x for latency | **partly refuted** (latency advantage is smaller on this host) |
| P8 | callgrind: exact is **0.47%** fewer instructions at R 220/440 and 1.4% fewer over all radii (x86 has a hardware `sqrtsd`). Wall time within noise | confirmed in direction (effect under 2%) |
| P9 | `frsqrte` is only **guaranteed** to about 1/32 relative error (architecture bound as I recall it, not re-read in the 7400 manual), against 1.2e-3 for the constant seed 1 on the reachable domain. The real table may be better than its bound; its actual error is not measurable under qemu (2.4) | **not confirmed**: a worst-case-bound argument only; hardware-unverified |
| P10 | host/G4 bit-identity risk: not testable for `frsqrte`. A different hazard (fused multiply-add contraction, **[post-reg]**, 2.4) turned up in the same check | `frsqrte` part untestable; FMA part is a post-registration observation |
| P11 | C1 (one step) is **not** pixel-identical (72 px in 17 frames); C2 is identical with 3 fewer flops than B | **partly refuted**: C1 fails the checksum rule |
| P12 | no double lanes in AltiVec; GCC 12.4 did not vectorize the double loop at `-mcpu=7400 -O3`, did vectorize the float loops | confirmed |
| P13 | "a few thousand calls per frame": true at R 220 (2,962) and 440 (8,615), but **up to 62,098 per frame at R 2560** | **partly refuted** (high zoom is far above my estimate) |

## 2.3 B. Comparative benchmark (host x86-64, g++ 13.3 `-O2`, ns per call, medians of 21 blocks)

Inputs: the 34.3M logged production arguments (sampled), and uniform [0.5, 1]. "Net" subtracts the loop that does only
the identity function (0.61 ns throughput, 1.82 ns latency). Throughput = independent inputs; latency = results feed the next input.

| Candidate | Production-domain throughput (net) | Latency (net) | Full-domain throughput | Max rel. error, shipped domain | Max rel. error, [0.5, 1] | Pixels vs baseline (1,764 frames) | PPC hot-path instructions (GCC 12.4) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| A `1.0/std::sqrt` | 3.09 (2.48) | 11.93 (10.11) | 3.13 | 1.7e-16 | 1.7e-16 | identical | call to libm `sqrt` + `fdiv` |
| **B current** | **0.92 (0.31)** | **7.76 (5.94)** | 0.91 | 7.9e-12 | **1.9e-2** | (baseline) | 13 (5 FP ops) |
| B2 guard moved 0.5 to 0.92 **[post-reg]** | 0.91 | 7.78 | 2.64 (exact path for arcs over 33 deg) | 7.9e-12 | **1e-5** | identical (x86 only) | B plus compare and call |
| C1 one Newton step, domain >= 0.99 | 0.61 (0.00) | 3.79 (1.97) | 1.22 | 2.3e-6 | 1.9e-2 (outside 0.99 falls back to B) | **72 px differ in 17 frames** | 9 (2 FP ops) |
| C2 cubic Taylor, domain >= 0.99 | 0.98 (0.38) | 8.05 (6.23) | 1.21 | 1.0e-11 | 1.9e-2 (outside falls back to B) | identical | 15 (4 FP ops) |
| C4 linear seed + 2 full Newton, [0.5, 1] | 1.55 (0.95) | 11.67 (9.85) | 1.56 | 1.3e-9 | **1.9e-6** | identical | 22 (11 FP ops) |
| D `frsqrte` + 2 steps (PPC only) | not measurable | not measurable | | see 2.4 | | not run on a host | 17 |
| D `frsqrte` + 3 steps (PPC only) | not measurable | not measurable | | see 2.4 | | not run on a host | 21 |

Other compilers (raw data in the JSON): clang 18 `-O2` agrees within 5% on every ratio; `-O3` changes nothing;
**`-fno-math-errno` changes A by under 1% (3.08 against 3.09 ns)**. GCC already inlines `sqrtsd` with a cheap errno guard,
so the "errno" cost presumed in E is not there on x86. Run-to-run range per cell is up to 2x in the raw min/max columns because the VM is shared; the medians
are stable to about 2%. The throughput loops of C1 collapse onto the identity loop: the compiler hides C1 behind the loop's other
work, so C1's throughput advantage is not informative.

## 2.4 PowerPC and toolchain constraints (compile and qemu only)

* **No scalar hardware sqrt on any G4-class CPU in GCC 12.4's model.** `-mcpu=603e/750/7400` define `_ARCH_PPCGR` but
  never `_ARCH_PPCSQ`; `std::sqrt` compiles to `bl sqrt` at every setting. That is consistent with the repository's own
  figure (docs/perf/README.md, 1,233 ns per sqrt on the G4, about 576 cycles), which I took as given and did not re-measure. It is a statement about GCC's target model and
  emitted code, not an independent check of the 7400's instruction set.
* **`frsqrte` needs a capability test, not `__powerpc__`.** `_ARCH_PPCGR` is set by `-mcpu=603e/604/750/7400/7450` and **not by the
  generic default** that the project's NetBSD toolchain file uses (no `-mcpu`). On GCC 12.4 there is no scalar builtin
  (`__builtin_ppc_rsqrte` is undeclared); only inline assembly reaches it (`rsqrt_ppc.hpp`, behind `_ARCH_PPCGR`), or
  `-ffast-math -mrecip=rsqrtd`, which the project forbids. A generic PPC build therefore must not assume it.
* **Accuracy is unverifiable here.** Under qemu `frsqrte` is accurate to 1e-16, which is **not** the 7400's table (the architecture
  guarantees only about 1/32). qemu's number says nothing about hardware, and **nothing in this report establishes how accurate the real `frsqrte` is**. If its error were at the full 1/32 bound (a worst case),
  Newton would need three steps for about 1.6e-11, one more than B's two (B starts from 1 with a 1.2e-3 error on the shipped domain).
  This is worst-case arithmetic, not a measurement and not a statement that `frsqrte` is worse than B on a real G4. The instruction counts for D are static GCC output for a function that was never executed on PowerPC hardware.
* **Static cost.** Hot-path instruction counts above include loading constants and a stack adjustment because the probe functions are
  not inlined; in a loop the constants hoist. FP-op counts are the comparable column. B has 5 dependent FP operations on
  its critical path; D with 3 steps has about 13 plus the estimate.
* **Cross-target numerics: an important exploratory result (**[post-reg]**, see 2.0.1).** GCC on PowerPC contracts `a*b+c` into `fmadd` by default
  (`__FP_FAST_FMA`), x86-64 baseline has no FMA. For B, **24,576 of 262,145 grid points differ from the host by up to 2 ulp**
  (A differs in none); with `-ffp-contract=off` the PPC results are bit-identical to x86 for every candidate.
  This matters to the renderer as a whole, not only to this helper: **the full 1,764-frame sweep, PPC32 under qemu against x86, differs
  in 80 frames and 7,287 of 541,900,800 pixels (worst frame 786 px)** with default flags, and is **bit-identical in every frame of this sweep
  with `-ffp-contract=off`** (one sweep, lines only, 640x480, 1,764 cameras; not a proof for other scenes or layers). Replacing `inverse_length` by A, C2 or C4 leaves the PPC frames identical to the PPC baseline; C1 changes the same 72 px in 17 frames
  on PPC as on x86. B2 was **not** run on PPC. Caveats: qemu (software FP, not a 7400), glibc `sin/cos`, Linux ABI, GCC 12.4, not NetBSD GCC 12.5. I did not check what the repository's own G4 runs
  record about host/G4 hash agreement, so I make no claim about whether this difference is visible there; this is a hypothesis for the owner to test on the real toolchain.
  This is unrelated to the question asked in #145, so it is a **separate follow-up for the owner**, not a recommendation to change flags (turning
  contraction off would also forfeit `fmadd` on the G4).
* **Real renderer TUs on PPC32 (GCC 12.4, `-O2`, identical at generic and 7400).** `raster.cpp`: 4 `__fixdfdi`, 4 `__floatdidf`, 2 `__divdi3`,
  2 `__moddi3`, 2 `ceil`, 3 `floor`; `render.cpp`: 4 `sqrt`, 1 `ceil`; `terrain.cpp`: 4 `ceil`; `projection.cpp`: 2 `atan2`, 2 `sin`.
  This **confirms #97's clang snippet result on the real GCC translation unit** (64-bit integer conversions and divides are library calls). It is a
  more promising instruction-level target than any reciprocal square root (2.6).

## 2.5 Rendering consequences (host, 1,764 frames, 640x480, lines only)

* **Frames:** in this sweep (lines only, 640x480, 1,764 cameras; terrain, shading and other resolutions untested), A, B2 **[post-reg]**, C2 and C4 produce frames **bit-identical** to baseline in all 1,764 frames (541.9M pixels) on x86, and A, C2, C4 on PPC32/qemu (B2 not run there). "Identical" means identical on those frames, not proven for every possible input.
  **C1 changes 72 pixels in 17 frames** (worst frame 12 px) on both. Its displacement is up to 0.0059 px; the existing 68-test suite, including pixel goldens, passes
  it. **The existing goldens cannot detect sub-0.01-pixel displacement**; the sweep (504 frames at R 220/440, 0.33 s on the host) can.
* **Geographic/limb:** visibility is decided by `clip_arc_to_front` before `inverse_length` runs, so no feature can change visibility; no frame changed `lines_drawn`.
* **Instruction counts (callgrind, whole process, including my harness's checksum reads, about 36% of all instructions):**

  | R 220/440 | instructions | against baseline | all radii | against baseline |
  | --- | --- | --- | --- | --- |
  | baseline | 7,286,546,377 | | 28,909,892,091 | |
  | A exact | 7,252,294,789 | -0.47% | 28,499,263,740 | -1.42% |
  | C1 | 7,274,289,216 | -0.17% | 28,746,464,139 | -0.57% |
  | C2 | 7,299,579,791 | +0.18% | 29,028,464,255 | +0.41% |
  | C4 | 7,312,214,677 | +0.35% | 29,192,572,543 | +0.98% |

  Relative to the renderer alone (`render_globe` is 38% of the process) these are about 2.6x larger: A -1.2%, C4 +0.9% at R 220/440.
  The function profile of the baseline at R 220/440 (inclusive): `draw_line` 36% of `render_globe`, `clip_arc_to_front` 7%, the rest of `draw_arc`
  (projection, subdivision, normalization) about 22%, clear and disc fill about 25%.
* **Wall time** (interleaved A/B/B/A, fresh processes, core pinned): every variant within 3.5% of baseline at R 220/440 and within 7% over all radii, with
  per-variant ranges of 10-20%. **No difference is resolvable** (I do not claim C1 is faster even though its median was lower).
* **Call counts per frame** (`host-counts-by-radius.json`): R 220: 2,962 calls; R 440: 8,615; R 880: 19,900; R 1760: 41,925; R 2560: 62,098.
  The separate `std::sqrt` that sets the subdivision count (`render.cpp:74`) runs only for arcs over 24 px: 0 per frame at R 220, 16 at R 440, 643 at R 880, 1,737 at R 1760, 2,774 at R 2560.
* **G4 model, not a measurement (**[post-reg]**).** Using the repository's measured 1,233 ns `sqrt` and #97's 18.6 ms moving frame: replacing B by A would add about
  `2,962 x (1,233 + about 70 ns fdiv)` = 3.9 ms per frame at R 220 (about 20% of 18.6 ms) and 11 ms at R 440. B itself costs about
  `5 dependent FP ops x about 5 cycles` = 25-30 cycles, about 0.18 ms per frame at R 220 (the cycle count is my recollection of 7400 double-precision FP latency and **needs checking
  against the MPC7400 manual**). So, if those inputs hold, the **modelled upper bound on anything that makes B free** is about 1% of a moving frame at R 220 and about 0.1% of the 280 ms settle, and the
  accurate path would be modelled at roughly 20x B's cost. No native G4 measurement supports either figure. The subdivision `sqrt` is about 2% of line-drawing time at the highest zoom (2,774 x 1.23 us = 3.4 ms against about 210 ms of line work).

## 2.6 SIMD as a separate hypothesis

* **Batched orthographic projection.** What would be vectorized: `x,y,z` dot products with the camera basis and `to_screen`, about 22% of the CPU frame's instructions at R 220/440
  (above), and only the part that can use whole batches of independent vertices. Layout: `Scene::vertices()` is AoS `Vec3` of `double` (24 bytes); `draw_arc` projects each
  vertex lazily as it walks a polyline, interleaved with `clip_arc_to_front` and line drawing, so a batch would first need a restructured loop (hoist all projections of a polyline,
  then draw) and gather/scatter back. **Precision:** AltiVec has no double lanes, so a 4-wide kernel must use float. A float unit vector carries about 6e-8 relative error, up to about 1.5e-4 px at R 2560; unlike the one-signed Newton error this is two-signed, and by comparison C1's 2.3e-6 relative error already flipped 72 pixels, so I expect a float kernel to flip pixels too (not measured). The repository's invariant is pixel identity with the host (#64, #97). Autovectorization (compile-only, GCC 12.4 `-O3`): **the double AoS loop is not vectorized for `-mcpu=7400 -maltivec`; the float AoS and SoA loops are**; at `-O2`
  and for generic PPC nothing is. x86 GCC `-O3` vectorizes all three with 2-wide SSE2. **A hand-written kernel is therefore not needed to get vector code, but the precision change is the blocker.**
  Justifying an experiment would take: (a) a profile showing projection above 15% of a G4 frame, (b) a decision that float projection is acceptable or a double-precision method, and (c) a sweep showing no visible pixel flips. None holds today.
* **Terrain shading / spans.** Evidence in the repository (#64, #97) and this experiment's GCC census agree: cells are about 4 px wide, 38-47% of scanlines are empty after setup, per-scanline setup has 3 `fdiv` and
  `ceil`, and `raster.cpp` makes `__fixdfdi/__divdi3` library calls. **The bottleneck is algorithmic and instruction-selection** (int32 fast paths, fewer divisions, #97's slices), before vector width. I did not write or time a
  terrain kernel. A palette-gather span would need a scatter/gather AltiVec does not have. RGB555 conversion is the one natural integer fit and SWAR covers most of it (#64).
* **Recommendation:** do not implement either on current evidence. Reconsider only after #97's scalar slices are measured on the G4 and projection or conversion is still large.

## 2.7 C. Architecture assessment

| Option | Verdict |
| --- | --- |
| One purely scalar CPU renderer | **Recommended**, with the documentation corrected |
| Scalar renderer with selected specialized math routines | Not warranted: the only candidate operation (this helper) has no beneficial replacement; C2 ties B, C4 is slower on the host (G4 speed unmeasured), C1 changes pixels, D is unverifiable here and not shown better than B |
| Optional architecture-specific SIMD kernels | Not now (2.6); revisit after #97 scalar slices |
| No changes at all | Acceptable, but the comment is wrong in two places and nothing pins the error contract |

Costs: code size: all portable candidates are 9-22 instructions, within a few bytes of B. Maintenance: any CPU path adds a capability probe, an assembly source and a second numerical contract; `frsqrte` cannot be tested for accuracy without hardware, and its estimate table is implementation-defined within a bound, so it
puts the host/G4 bit-identity the project relies on at risk. Portability: the portable code compiles under C++20 without exceptions/RTTI and uses only `std::sqrt`; nothing probed here requires an optional CPU feature. Reproducibility: C2 and C4 (and A) give frames identical to their own baseline on x86 and on PPC32/qemu in the sweep; B2 was checked on x86 only; the FMA effect (2.4, post-registration) is the larger cross-target risk and is unverified on the real toolchain.

## 2.8 E. Decision recommendation

1. **Immediate: option 1, keep the current approach and correct its documentation.** Two comments are inaccurate: `inverse_length` (domain and error bounds) and `band_margin_px` ("Newton step good to 1e-5").
   Proposed wording is in `docs/perf/rsqrt-20261116/proposed/01-comment-only.patch`. **Not applied to production.** Both patches apply cleanly to `0b5b16d`.
2. **Optional, owner's choice: patch `02-comment-and-guard.patch` also moves the exact-path guard from 0.5 to 0.92** (candidate B2), so the code satisfies its own 1e-5 claim over its whole domain. This is a post-registration variant (2.0.1). Frames are bit-identical for all
   1,764 sweep frames on x86 only (not run on PPC32, full suite not run with it); no shipped arc reaches the new exact path. Its cost is a G4 `sqrt` (about 1.2 us) for arcs over 33 deg that do not exist today. I would take it only if
   other atlases are expected.
3. **Do not**: add `globe::math` (rule 4: one operation, one call site; a single internal helper is enough), adopt `frsqrte` or any `_ARCH_PPCGR` path, or implement AltiVec projection/shading.
4. **Option 5 (defer pending hardware) for the PowerPC question only.** Nothing here needs G4 time. If the owner still wants `frsqrte` characterised, the minimal experiment is in 2.9.
5. **Separate follow-ups, for the owner to file or decline** (not in this PR): (a) fused multiply-add contraction changes 80/1,764 sweep frames between x86 and PPC32 (qemu) and may bear on host/G4 golden identity; decide whether to
   check this on NetBSD GCC 12.5 and what, if anything, to do about `fmadd`; (b) add a deterministic camera-sweep checksum test (the existing goldens miss sub-0.01 px changes); (c) a contract test for the helper's error bound would need the helper exposed in an internal header.
   A sqrt-free exact subdivision step count (`render.cpp:74`) is **not** worth doing (about 2% of line time at the highest zoom, model only).

**Recommendation on the proposed patches (2026-11-16 review).** Submit `01-comment-only.patch` as its own small PR if the owner agrees: it touches only two comments, changes no behaviour, and the corrected statements are
supported by the audit (exact error table, longest shipped arc 5.7 deg). Do **not** submit `02-comment-and-guard.patch` on current evidence: it changes executable code on the strength of an x86-only sweep and a post-registration variant, for arcs the shipped data does not contain. Neither patch is applied here, and a separate PR needs the owner's go-ahead.

## 2.9 What remains unverified, and exact commands

Unverified on G4 or by any target toolchain: native speed of every candidate; `frsqrte` accuracy and the interaction of its estimate table with host/G4 bit identity; whether NetBSD GCC 12.5 contracts to `fmadd` exactly as GCC 12.4 does
and whether the sweep differences reproduce there; Retro68 `-Os` code for any candidate; Retro68 68k behaviour (`std::sqrt` is software there); the MPC7400 FP latencies used in the model.

**Owner-run Retro68 probes** (compile only; inspect every `FAIL` line, do not suppress):

```sh
tools/probe-retro68.sh                       # existing script; needs podman or CONTAINER=docker, ghcr.io/autc04/retro68
# then, inside the same container, for each of powerpc-apple-macos-g++ and m68k-apple-macos-g++:
$TC/bin/$cxx -O2 -std=c++20 -fno-exceptions -fno-rtti -Itools/rsqrt-research -S tools/rsqrt-research/ppc_probe.cpp -o /tmp/probe.s
$TC/bin/$cxx -dM -E -x c++ /dev/null | grep -E '_ARCH_PPC|ALTIVEC|FP_FAST_FMA'   # does the default CPU set _ARCH_PPCGR?
```

**Minimal owner-authorized G4 experiment, only if wanted** (cross-build on the host per docs/g4-benchmark.md; fresh names; no compiling on the G4):
`powerpc-...-g++ -O2 -static -std=c++20 -mcpu=7400 tools/rsqrt-research/rsqrt_lab.cpp -o rsqrt-lab-SHA` (adjust to the NetBSD toolchain file), then `./rsqrt-lab-SHA bench` x 9 fresh processes and `./rsqrt-lab-SHA frsqrte`
(the second reports the real estimate's error; it is only meaningful on hardware), plus A/B/B/A of `rsqrt-trace` variants base, 3, 4 and 1 for the frame hashes (needs the scratch-build scripts). No decision above depends on its result.

**Reproduce** (host): `tools/rsqrt-research/build_variants.sh <rev> /tmp base 0 1 2 3 4 5 6`, then `rsqrt-trace --write` for base and `--compare` for the others;
`g++ -O2 -std=c++20 tools/rsqrt-research/rsqrt_lab.cpp -o lab && lab audit args.bin && lab bench args.bin` (`args.bin` is variant 5's `--log`; 549 MB, not committed);
`tools/rsqrt-research/ppc_asm_report.py powerpc-linux-gnu-g++-12 out/`; `tools/rsqrt-research/simd_probe_report.sh out/`; `tools/rsqrt-research/count_instructions.sh /tmp 220,440 base 1 2 3 4`.

## 2.10 Review pass, 2026-11-16 (after draft PR #180 CI failure)

* **CI failure:** job "Host build, lint and tests", step "Format check (clang-format 22.1.8)" failed (exit 123): the six first-party files under
  `tools/rsqrt-research/` that this PR added were not clang-formatted. No pre-existing file failed. Cause: I validated with the distribution's clang-format 18.1.3, not the pinned 22.1.8.
* **Fix:** `clang-format -i` (22.1.8, in a venv) on exactly those six files; no other file touched. Whitespace only; recorded data was not regenerated (see PROVENANCE.txt).
* **Commands run, Linux x86-64 (2 vCPU VM), after the fix:**

  | Command | Result |
  | --- | --- |
  | `git ls-files '*.cpp' '*.hpp' \| xargs clang-format --dry-run --Werror` (clang-format 22.1.8) | pass (before the fix: exit 123, same six files as CI) |
  | `tools/check-policy.sh` | `policy: ok` |
  | `python3 -m unittest discover -s tests -p test_release_manifest.py` | 4 tests OK |
  | `cmake -S . -B build -G Ninja -DGLOBE_SANITIZE=ON`, build, `ctest` (g++ 13.3.0, cmake 3.28.3) | 68/68 pass |
  | `g++ -O2 -std=c++20 -Wall -Wextra tools/rsqrt-research/rsqrt_lab.cpp`; `lab audit` | compiles clean; audit values match committed JSON |
  | `powerpc-linux-gnu-g++-12 -O2 -mcpu=7400 -S tools/rsqrt-research/ppc_probe.cpp` (GCC 12.4.0) | compiles |
* **Not run in this pass:** clang-tidy 22.1.8 (CI lints `src/` only, which this PR does not touch; CI is the authority), frame sweeps, benchmarks, callgrind, qemu runs, Retro68, NetBSD toolchain, anything on the G4. CI results are recorded on PR #180, not here.
* **Wording corrections made in this pass:** B2, the FMA observation, the band-margin argument and the G4 cost model are labelled post-registration (2.0.1); P9 changed from "confirmed" to "not confirmed" (it rests on a worst-case bound); the statement that the repository's G4 runs show matching hashes was removed because I had not checked it; "identical frames" claims are scoped to the lines-only sweep, and B2 is stated as x86-only.
