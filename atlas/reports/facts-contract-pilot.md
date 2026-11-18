# EXP-FACTS-01 — Synthetic facts-contract falsification pilot

**Status:** Draft research-only result, November 17, 2026. **Refs:** [#137](https://github.com/jackwillis/globe/issues/137), [#52](https://github.com/jackwillis/globe/issues/52), [#53](https://github.com/jackwillis/globe/issues/53). **No runtime implementation or geographic policy approval.**

## Decision and baseline

What minimum semantic checks should a future offline facts resource apply before displaying a population figure as reliable geographic information?

Inspection of live source on November 17 found [src/core/places.cpp](../../src/core/places.cpp) (blob `44ba9a6548d9d3f7382a9d6182f71d9dcf6caa98`) and [src/core/countries.cpp](../../src/core/countries.cpp) (blob `db981c9a425266d4d7f6e2c3747fd1cac8f5a645`) construct generic dataset-description cards. They do **not** currently display structured, qualified population facts. GPLC v1 also does not store population; its presence in a candidate builder snapshot does not make its magnitude a vetted city population.

The experiment therefore probes a **candidate contract in separate Python**, not the current C++ runtime or any real source dataset. No network/source acquisition or Macintosh access occurred.

## Competing mechanisms / explicit prediction

- **Unsafe shortcut:** choose the largest positive population number, regardless of country/entity mapping, source review, date, definition, and editorial selection. This is an intentionally inadequate comparator, not a claim about the current runtime.
- **Candidate contract:** only use a uniquely identified curator-preferred claim for an explicitly reviewed entity, with matching subject, reviewed assertion, approved snapshot source, positive integer value, explicit year, and known population definition. Otherwise omit the population.
- **Predeclared prediction:** ten fixed adversarial cases distinguish the approaches; the candidate must match all manually assigned expected claim IDs, be independent of assertion order, ignore an added unreviewed outlier, and fail closed when the curated claim disappears. A mutation removing the year requirement must be detectable.

## How to reproduce

Standard-library Python on the **build host**, no external sources:

```sh
python3 atlas/research/facts_contract_probe.py > /tmp/exp-facts-01-result.json
```

The Python file contains the complete **synthetic** fixture set and deterministic test. Keep the full JSON output (including all case-level observations, violations and the canonical-input hash) with review evidence. The experiment stops after ten fixed cases, twenty deterministic permutations per case, two further metamorphic checks, and one targeted mutation gate.

## Preliminary local experiment

A standalone prototype of this protocol was run locally in the ChatGPT development container. It reported:

| Measurement | Observed |
| --- | ---: |
| Synthetic cases | 10 |
| Candidate policy agreement with manually authored oracle | 10/10 |
| Naive largest-number comparator agreement | 1/10 |
| Deterministic metamorphic checks | 202/202 |
| Targeted missing-year validation mutation detected | Yes |

The checked-in script reproduces the same protocol with its fixture definitions in one file; **its precise CI/host execution at the PR revision is not yet recorded**, so verify it independently before treating these figures as results for that revision. Repeated fixtures are not independent geographic sampling units. Since the investigators designed the examples and their expected results, these counts are contract-discrimination evidence rather than an unbiased estimate of real-world error rates.

## Limits and next decision

- No actual Wikidata, Natural Earth or other geographic assertion was verified. No new geographic identity or license approved.
- This experiment does not measure false claims in existing Poodle Earth, because facts are not implemented in its current information cards.
- The candidate definitions and source/claim requirements are *proposed*, not accepted editorial policy. An independent reviewer should check whether the guards are sufficient, overly restrictive, or permit important known failure modes.
- The first proposed follow-up is an independent expert-reviewed fixture corpus with at least one ambiguous identity, competing population definitions, multiple years, a missing qualifier, and an accepted fact. Keep the fixture corpus separate from the implementation.
- Before any #137 implementation, decide the reviewed-ID mapping and source-assertion selection contract with #52/#53. Do not infer IDs from names or GPOL ordinals.
- No OS 9 memory, typography, scrolling or hardware compatibility evidence exists here.

## Methodological finding

The minimal *protocol + executable domain-specific oracle + full JSON observations* was useful. The earlier general-purpose paired-label experiment runner was **not needed** for this semantic test. That is evidence for postponing a generic experiment framework until two substantially different real pilots demonstrate a shared capability worth extracting.

**Recommended status:** Keep as research-only, seek independent review, and link findings to #137. It does not close #137 or change the v0.2 release gate.
