# PE-AA-01 v1 rule traceability and documentary verification

2026-11-18. Paths below are repository-relative. Historical files are read-only evidence.
Original sources refer to commit 5dd566d9; proposal to 4c8bfc74; corrections to 5144047c.

| v1 rule | Frozen protocol/source | Reviewed derivation/control and precise change |
|---|---|---|
| 1. Admission states | tools/aa-experiment/REVIEW-AMENDMENT.md reference convergence; PROTOCOL.md | tools/aa-certification-contract/PROPOSAL.md; mask-review/AMENDMENT-DECISION.md §1: validity/precision/resolution replace unsupported grid certificate |
| 2. Geometry | aa-experiment/PROTOCOL.md reference semantics; reference.py; oracle_rows.py | contract/INVENTORY.md and mask-review/REPORT.md: exact dyadic endpoint model, old floating bytes not asserted identical |
| 3. Quantization | aa-experiment/reference.py; review amendment sequential control | contract/intervals.py and test_contract.py: half-up ties, coupled quantized loss and ordered shared-alpha blend |
| 4. Radius/margin | aa-experiment/REVIEW-AMENDMENT.md; FINAL-PRE-SCORING.md | mask-review/AMENDMENT-DECISION.md remaining choices: owner now retains 1/10240 as operational radius, not gate |
| 5. Mask | aa-experiment/quality_only.py neighborhood | mask-review/masked_audit.py, REPORT.md and masked-audit.json: literal bytes/count/hash/provenance, no mask substitution |
| 6. Point/local rules | PROTOCOL.md quality target; REVIEW-AMENDMENT.md floor/regression; quality_only.py | contract/intervals.py; mask-review/test_review.py: shared residual, absolute 2/1024 mutant killed, exact tightness controls |
| 7. Statistics | aa-experiment/statistics_plan.py; REVIEW-AMENDMENT.md paired units; FINAL-PRE-SCORING.md undefined policy | mask-review/SUBSET-METHOD.md, subsets.py, test_review.py: floor equality mutant killed; 180 cases/1728 subsets, exact extrema |
| 8. Diagnostics | aa-experiment/review_instruments.py and sidecars.py | contract/INVENTORY.md; mask-review/test_review.py: uncertain-support peak-upper mutant killed; future positive-area semantics expressly authorized, old sampled semantics preserved |
| 9. Inputs/provenance | aa-experiment/final-source-freeze.json; quality_only.py file expectations | contract/PROPOSAL.md; mask-review/AMENDMENT-DECISION.md: interval adapter required, no scorer implemented |

In this table `contract/` means tools/aa-certification-contract/ and `mask-review/` means
tools/aa-certification-mask-review/. Evidence JSON files are in the existing reviewed evidence
package, not regenerated here. Full paths and source SHA256 values are in
certification-source-record-v1.json.

## Reviewed evidence retained, not rerun

All three targeted mutants failed in the reviewed correction evidence; corrected controls and
original suite passed. Exact tightness checks included paired [-1/2,1/2] and quantized singleton
8/85. Bounded-subset extrema matched exhaustive 1728 subsets in 180 synthetic/adversarial cases.
These are historical verified observations, not new tests executed by this documentation task.

The existing coarse-mask audit gives exact u_M=152644143077/1279966020894720 (about
0.0001192564), versus full-frame 152644143077/5257039970304000. Its concentration demonstrates
why full-frame uncertainty cannot substitute for primary-mask uncertainty. It reveals no A/B gap.
Neither that observation nor the amendment changes the approved point or regression thresholds.

## Verification performed for this version

Static source/formula review against the sources above and supplied complete Claude review;
SHA256 manifest of documentary dependencies; git tree comparison requiring only new documents.
No numerical test, mask generation, geographic computation, candidate/image read, scoring or timing
was performed. Existing mutation and enumeration results are cited, not represented as rerun.
No contradiction requiring an estimand change was found: exact semantics changes are explicit,
historical artifacts are preserved, and the point/local rules remain unchanged. New interval
instrumentation, coverage evidence and execution authorization remain prerequisites, not assumed.
