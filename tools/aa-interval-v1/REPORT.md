# First certified interval-scoring vertical slice — steering handoff

2026-11-18, Refs #25. Instrumentation only. Isolated worktree
`/tmp/globe-aa-interval`, branch `codex/pe-aa-interval-slice-20261118`,
parent `9a2214c6286c54f3d451956abe105b9d33877845`.

## Results and evidence

On Linux x86-64, Python 3.14.8 (GCC 16.2.1), standard library only:

- 15 independent synthetic tests pass. Direct integer-world enumeration checks
  3645 scalar interval/candidate combinations, all three losses each. Other controls
  include identical candidates for all 256 encoded values, exact analytical
  mask/RGB normalization, 64 heterogeneous shared-reference worlds, antisymmetry,
  constant-difference tightness, all 94 RGB565 channel boundaries from below/at/above,
  threshold equality/crossing (including radius 10^-30), and certificate-to-metric
  synthetic integration. No producer or proposal interval helper is reused.
- Failure controls reject 18 malformed tile mutations, invalid/incomplete metric
  rows, duplicate keys/pixels, noncanonical/nonfinite/oversized encodings, geometry
  domain/order/truncation defects, bad hashes and oversized/symlink evidence files.
  A subprocess verifies candidate opens and writes are denied by the CLI audit hook.
  Valid-but-wide records pass without being called precise or decision-resolved.
- Existing coarse admission validates all 4800 tiles / 307200 pixels and all pinned
  source/input/evidence hashes. Every pixel is precise at original radius 1/2560.
  Literal primary mask count/hash match 74796 / `68c4c2ff...23fcd7`.
  Exact candidate-independent masked midpoint bound is
  `152644143077/1279966020894720`; full-frame bound is
  `152644143077/5257039970304000`; outside-mask width sum is zero. All match the
  historical mask audit. 2177 pixels retain RGB565 code ambiguity. These are
  admission/mask-reconciliation observations, not geographic quality scores.

Final admission used 9.58 CPU seconds / 9.67 elapsed seconds and 26904 KiB peak
RSS, within 384 MiB, 60/65 CPU-second and outer 90-second limits. Synthetic
suite used 0.27 elapsed seconds / 22028 KiB peak RSS (as reported by time); not rendering timings.

Exact commands are in README.md; admission-result.json retains the complete
final admission output. Captured output, resource observations and
instrument hash freeze live in `/tmp/aa-interval-v1-evidence`. Admission's audit
hook permits only pinned sources, records, geometry/background, mask and tile
payloads: zero candidate image inputs and zero geographic quality scores.
Historical evidence was opened read-only; no original engine, experiment, evidence,
mask or fixture was modified. Only this new research directory changes in Git.

## Amendment v1 implementation coverage

| Rule | This slice | Still unimplemented/unverified here |
|---|---|---|
| 1 admission states | Independent structural/hash admission; valid/precise separate | New formal proof or re-execution of geometric producer; decision resolution |
| 2 exact target | Pin reviewed engine/proof/controls; independent binary64/domain and order check | New geometry integration; upstream coordinate uncertainty |
| 3 discrete outcomes | Exact half-up code-range validation and independent tie controls | Coupled Q(R) loss/residuals, ordered shared-alpha sequential packed-state propagation |
| 4 resolution | Existing coarse 1/2560 checked as operational precision | Fine 1/10240 work or resource authorization |
| 5 primary mask | Existing literal bytes/count/hash and source/input/runtime reconciliation | Every other fixture's candidate-blind mask freeze |
| 6 metrics/guards | Fixed-mask E_A/E_B and common-reference E_A−E_B bounds | Coupled H/D/J, all 16 equally weighted IDs and two styles, point/local decisions |
| 7 bootstrap | Nothing executed; semantics unchanged | Frozen 100000 paired draws, conditional bounds, bounded-subset percentiles/reporting |
| 8 diagnostics | No support inference from full-color bounds | Feature coverage, peak/visibility/undefined aggregation; motion remains deferred |
| 9 input contract | Versioned narrow legacy adapter, provenance, complete bounded rational rows and literal mask | General interval format; feature-ordered coverage, candidate/build admission and exact A replay; full-frame worst-error/outside-mask candidate summaries |

Cartesian scalar interval independence is an explicitly conservative relaxation;
shared-reference scalar losses themselves are exact. No midpoint score, bootstrap
or independent reference-noise construction supplies a verdict. The 20% point
criterion, strict floor, local limits, native/blind review requirements and source
provenance remain unchanged. Independent tests are new; independent human/agent
review of this implementation is still needed before freezing it for comparisons.

## Not run and next action

No geographic A/B bytes were opened, scored or hashed anew. No new reference,
fine-frame computation, coverage generation, geographic bootstrap, rendering
benchmark, B tuning, G4 access, production integration, merge or release. CMake,
CTest, Retro68 and CI were not run: changes are isolated stdlib Python research,
with no production/build dependency. This is not hardware acceptance or a full
amendment-v1 scorer.

Before a scored comparison: independent implementation review/freeze; remaining
coupled metrics/discrete controls/statistics/diagnostics and all required masks;
separate authorization/resources for missing reference/feature coverage evidence;
complete source/input/candidate/build admission and exact same-host A replay;
preserved blind native judgments; explicit geographic-scoring authorization.

**Stop for steering review. One next action:** independently review this adapter's
trust boundary and the scalar/shared-reference metric controls, then freeze or
request corrections to this slice before authorizing any further instrumentation.

Repository startup: original main clean at `00be059`; existing worktrees untouched.
Current release milestone v0.1.1, no ready work in that milestone. No stale active
claim over 14 days found. Open PRs #190 and #193 lack a Closes/Refs issue link;
reported only, not changed. Latest release v0.1.0; unrelated state remains untouched.
Optional GitHub Project view not consulted; REST issues/labels/milestones were used.
