# First certified interval-scoring vertical slice — steering handoff

2026-11-18, Refs #25. Instrumentation only. Isolated worktree
`/tmp/globe-aa-interval`, branch `codex/pe-aa-interval-slice-20261118`,
parent `9a2214c6286c54f3d451956abe105b9d33877845`. Focused review follow-up
to c67e4ba, after verifying local/remote heads agree and the worktree is clean.
Read all 82 lines of the supplied Claude review, SHA256
`fe6b63bb4f38612a31d84f0940acf661de37083d7adfed3b7ec4d1fb242f8dc3`.
Review is evidence; only the owner-assigned corrections authorize changes.

## Results and evidence

On Linux x86-64, Python 3.14.8 (GCC 16.2.1), standard library only:

- 26 independent synthetic tests pass. Direct integer-world enumeration checks
  3645 scalar interval/candidate combinations, all three losses each. Other controls
  include identical candidates for all 256 encoded values, exact analytical
  mask/RGB normalization, 64 heterogeneous shared-reference worlds, antisymmetry,
  constant-difference tightness, all 94 RGB565 channel boundaries from below/at/above,
  threshold equality/crossing (including radius 10^-30), and certificate-to-metric
  synthetic integration. No producer or proposal interval helper is reused.
- Failure controls reject 18 malformed tile mutations, invalid/incomplete metric
  rows, duplicate keys/pixels, noncanonical/nonfinite/oversized encodings, geometry
  domain/order/truncation defects, bad hashes and oversized/symlink evidence files.
  A subprocess verifies non-allowlisted and write-mode opens are denied by the CLI hook.
  Valid-but-wide records pass without being called precise or decision-resolved.
- Added full synthetic admit()/bounds() execution, exact 4138-prefix/662-tail
  routing, a hash-pinned valid payload changed after admission, malformed/short
  masks even with matching hashes, noncanonical positive denominators, interior
  half-up checks in admission, width equality/one numerator unit over precision,
  equal-count duplicate-plus-omission, and partial/error/closed completion states.
  Only bounded_read is injected; actual hashing, parser and admission loops run.
  One manifest and at most two tile payloads are retained, not a full archive.
  All six formerly surviving consequential code mutants are now killed by their
  targeted controls; check_mutations.py reproduces this without editing sources.
- Existing coarse admission validates all 4800 tiles / 307200 pixels and all pinned
  source/input/evidence hashes. Every pixel is precise at original radius 1/2560.
  Literal primary mask count/hash match 74796 / `68c4c2ff...23fcd7`.
  Exact candidate-independent masked midpoint bound is
  `152644143077/1279966020894720`; full-frame bound is
  `152644143077/5257039970304000`; outside-mask width sum is zero. All match the
  historical mask audit. 2177 pixels retain RGB565 code ambiguity. These are
  admission/mask-reconciliation observations, not geographic quality scores.

Final admission: 9.94 CPU seconds / 10.01 elapsed seconds,
26992 KiB peak RSS, within 384 MiB / 60–65 CPU seconds / 90 elapsed
seconds. Synthetic suite: 33.30 seconds (33.39 outer elapsed), 32764 KiB peak
RSS, 120-second wall guard. Mutation controls: 17.53 outer elapsed seconds / 35636 KiB peak RSS under
a 120-second wall guard; raw resource logs and mutation-results.json are retained. Admission/mutation observations
were collected under concurrent host load; none are rendering benchmarks.

Exact commands are in README.md; admission-result.json retains the complete
final admission output. Captured output, resource observations and
instrument hash freeze live in `/tmp/aa-interval-review-evidence`. The admission
CLI restricts audited open calls to pinned sources, records, geometry/background,
mask and tile payloads and rejects open write modes/flags. It does not block
unlink, rename, mkdir, truncate, chmod, symlink, subprocess, os.system or ctypes;
it is neither a filesystem/process sandbox nor general write prevention.
The bounds() library path does not install the hook. unique_open_paths_allowed
counts allowed open attempts, not successful reads. Program behavior plus pinned
hashes—not the hook alone—supports the observed zero candidate inputs and zero
geographic scores. No evidence-mutation code is added.
Historical evidence was opened read-only; no original engine, experiment, evidence,
mask or fixture was modified. Only this new research directory changes in Git.

## Amendment v1 implementation coverage

| Rule | This slice | Still unimplemented/unverified here |
|---|---|---|
| 1 admission states | Structural admission under pinned provenance; precision separate | New formal proof or re-execution of geometric producer; decision resolution |
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
provenance remain unchanged. Independent tests and the required corrections are new; steering re-review is
still needed before freezing the corrected implementation for comparisons.

## Not run and next action

No geographic A/B bytes were opened, scored or hashed anew. No new reference,
fine-frame computation, coverage generation, geographic bootstrap, rendering
benchmark, B tuning, G4 access, production integration, merge or release. CMake,
CTest and Retro68 were not rerun locally: changes are isolated stdlib Python research,
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


## Focused response to Claude findings

- M1 / S2 required: corrected README, report and admission JSON to report
  structural_admission and geometric_enclosure_reproved=false, replacing the
  over-readable valid boolean. open_write_denied replaces writes_permitted;
  hook limitations and descriptive engine/verifier labels are explicit.
- S1 required: BoundsStream issues a CompletionReceipt only after verified full
  exhaustion; caught errors/close/partial consumption cannot produce a receipt.
  Streaming/low memory and fixed_mask's independent backstop remain.
- T1 required: new synthetic integration/edge controls cover every requested gap;
  all six surviving code mutants from the review are killed. Original exact
  scalar/shared-reference mathematics and metrics.py remain byte-identical.
- S3 optional, left: no new admission token or mask/metric binding. The completion
  receipt signals stream completion, not consumer authorization; a future caller
  must use the pinned mask. No geographic consuming/scoring path is authorized.
- M1 optional hardening, left: no general sandbox, extra audit events, or guard
  installed in bounds(). This scope corrects claims, retains current restrictions
  and validates hashes; there is no demonstrated arithmetic acceptance defect.
- S4 optional, left: no new fair-quota type or geometry-scale/width-area telemetry
  checks. Reviewed pinned bytes and arithmetic admission are unchanged; no
  demonstrated enclosure/metric correctness issue justifies expanding the slice.
  No optional candidate-format enforcement or workflow expansion is added.

## CI limitation, verified without repairing ancestor code

At reviewed c67e4ba, Checks / Host build, lint and tests failed at Format check:
[run 44464608394, job 135359254875](https://github.com/jackwillis/globe/actions/runs/44464608394/job/135359254875).
Log reports synthetic_driver.cpp:7:20 as the first violation and exit123.
Locally clang-format22.1.8 reproduces 63 diagnostics in that file, lines7–18:

```sh
clang-format --dry-run --Werror tools/aa-experiment/synthetic_driver.cpp
# exit1, 63 diagnostics
git ls-files '*.cpp' '*.hpp' | xargs clang-format --dry-run --Werror
# exit123, same 63 diagnostics
```

The file was introduced at 2955beda16c002e92a38967583bde198a259e8f4. Its Git
blob is 1b6173fe92d84c806e12a63aa44c6edf3155d73f at amendment parent and PR
head alike; no diff in this slice. CI's host-clang/Retro68/packaging jobs passed
at the reviewed revision but do not exercise this Python instrument. The overall
PR is not CI-green; this inherited failure is deliberately left for separate
authorization. Current follow-up check state is recorded in the PR/issue handoff.
