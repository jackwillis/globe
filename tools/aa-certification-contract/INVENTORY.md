# Frozen metric inventory and certification limits

Source precedence: FINAL-PRE-SCORING.md > REVIEW-AMENDMENT.md > PROTOCOL.md, together with actual
quality_only.py, statistics_plan.py, review_instruments.py, sidecars.py and validate_oracle.py.
The owner-approved original margin is0.001953125; required historical precision1/10240 remains.

| Metric / frozen behavior | Uncertainty requirement | Status from present evidence |
|---|---|---|
| Primary normalized RGB MAE, three-pixel geometry neighborhood including endpoints (`quality_only.score`) | Fixed-mask per-channel loss intervals; literal frozen neighborhood bytes must be fixed first | Formulas validated; no geographic candidates/masks scored |
| Full-frame MAE / worst pixel-channel error (`score`) | Sum / maximum of scalar loss intervals; worst error is full frame, not just mask | Formula certifiable; coarse reference alone not admitted as fine |
| Outside-mask changed count | Exact packed candidate/background comparison on fixed mask; no reference error | Exact in a future authorized comparison; no candidate read now |
| Support-only error (protocol, synthetic `validate_oracle`) | Reference-dependent membership and denominator; optional pixels cannot be discarded | Uncertifiable point value when support unresolved; geographic runner does not implement this summary |
| Once-quantized RGB565 control, E_Q(R) in primary ratio | Q tied to SAME continuous R, half-up ties, bit-replication expansion; quantizer-cell loss extrema | Small exact demonstrator validated; no need to force unique codes if intervals prove the decision |
| Sequential RGB565 achievable control E_S(R) | Per-feature union coverages, alpha rounding, ordered packed state propagation, shared-alpha RGB coupling; joint S/R constraints or conservative relaxation | Full-color coarse certificates insufficient; no geographic coverage arrays or full scorer here |
| Local regression per condition | Δ=E_B−E_A with common reference, threshold max(E_A/10,2/1024) also uncertain | Safe interval guard validated; frozen `quality_only` reports errors but does not execute this acceptance gate |
| Excess-error gap closure | Ratio of16geometry mean errors per style; D>10/1024,20% target; once-quantized and sequential reported separately | Robust point residual validated; floor ambiguity yields inconclusive |
| Reference-dependent isolated support | True color contrast>0 in prose vs `any(sampled_ref!=bg)` in sidecar implementation | Exact-positive/zero status not implied by a small finite interval; choose intended estimand explicitly before any changed mask |
| Candidate/reference peak, source ratios, retention (`feature_metrics`) | Same eligible pixel set; variable support membership, reference peak denominator, zero-reference null | Interval formula/control validated; point retention not certifiable when it depends on unresolved membership/denominator |
| Thin-feature visibility (`feature_metrics`) | Original clipped3×3 midpoint probes, ref eligibility≥10% source contrast; optional denominator, deterministic candidate hits | Numerical diagnostic, not a new acceptance gate; feature-weighting/empty-feature aggregation must remain explicit |
| Integrated contrast/ink and missing-quarter synthetic diagnostics | Original `validate_oracle` white-over-black ink=sum(candidate)/sum(ref), missing-quarter=max(ref)≥.25 and candidate0 | Scalar sum ratio / threshold membership bounds possible; no frozen generalized geographic colored-ink formula or acceptance threshold exists |
| Motion/translation stability | REVIEW-AMENDMENT explicitly defers scoring; reversal/translation controls do not measure temporal stability | Deferred, cannot be certified by static-frame bounds |
| Native legibility/material missing stroke | Independent blinded judgments, not numerical uncertainty alone | Cannot be replaced by a certificate or new visibility threshold |

## What requires an amendment, versus what merely lacks data

1. **Reference admission:** invalid successive-grid agreement cannot remain the numerical accuracy
   certificate. Enclosure admission, endpoint-defined target versus rounded sampled polygons, positive
   margin handling and immutable geometry-mask provenance need an explicit versioned amendment.
   No original source/protocol is edited by this proposal.
2. **Reference-dependent support:** true-support prose and sampled-mask implementation disagree at
   numerical boundaries. Preserve the chosen target explicitly. Replacing it by the geometry-fixed
   three-pixel mask, thresholded support or a candidate-dependent mask would change the metric and
   requires approval. If true support is retained, uncertainty can legitimately remain inconclusive;
   an amendment is not permission to silently coerce it false.
3. **Quality verdict mapping:** the frozen script reports ratios and sensitivity intervals but no
   automatic pass. A requirement that the quality percentile lower bound exceed20%, or another mapping
   from sensitivity interval to verdict, must be expressly adopted, not borrowed from timing rules.
  20%, floor10/1024, regression10%/2/1024 and exact5% undefined policy are unchanged here.
4. **Numerically ambiguous bootstrap exclusion:** the frozen implementation receives finite point
   errors and excludes nonfinite statistics. It has no rule for uncertain definedness. The proposed
   all-ambiguous-inconclusive extension needs versioned instrumentation/admission approval, not a new
   post-outcome threshold or favorable selection of replicate subsets.
5. **Unspecified metrics:** geographic integrated colored-ink/missing-material acceptance, motion
   scoring and equal-feature aggregation for undefined visibility are not completed by existing code.
   Any new numerical acceptance gate or estimand needs its own explicit definition. Human missing-stroke
   review remains separate; visibility cannot be promoted into its substitute.

Fixed-mask MAE, full/worst error, local residual and gap residual do not inherently need a new
estimand. They need valid numerical references and approved interval instruments. Sequential control
is already conceptually defined; lack of coverage/outcome bounds is missing instrumentation/data,
not grounds to silently redefine it. Unresolved support/rounding can remain ambiguous even with a
fine target; a full-color fine frame would not by itself supply per-feature coverage or sidecars.

Exact rational propagation certifies the mathematical metrics stated here, not every rounding artifact of the frozen NumPy implementation. Adopting exact/directed interval instrumentation and its percentile arithmetic is an explicit versioned change; numerical tolerances cannot silently grant a borderline pass.
