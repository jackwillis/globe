# Experimental methods and evidence workflow for Poodle Earth

**Status:** Proposed, non-normative research method; owner review required.  
**Recorded:** November 17, 2026 (America/Chicago).  
**Scope:** Development-host research and evidence practices; no release, hardware, source/license, or implementation authorization.

## Purpose

Develop Poodle Earth using cheap, falsifiable, reproducible experiments when they can change an engineering or product decision. The goal is **better decisions**, not more process or a second backlog. Preserve negative results: rejecting an optimization or disproving a design assumption can be a successful experiment.

Keep six things separate: **source facts → explanatory model → prediction → observation → interpretation → owner decision**. Do not convert agent consensus, a benchmark, or a speculative research outcome into an accepted product or release decision.

## Existing authorities and work

- [AGENTS.md](../../AGENTS.md) governs assignment, permission to implement, the G4, other agents' worktrees, licensing, branch and release authority. This proposal changes none of it.
- [Rendering experiment program](rendering-experiment-program.md) already owns the rendering/portability experiment registry, causal-model examples, hardware evidence levels, matched A/B/B/A benchmarks, and outcome policy. **Extend it; do not replace or duplicate the registry.**
- [G4 benchmark procedure](../g4-benchmark.md), [performance reports](../perf/) and applicable GitHub issues own native measurements and acceptance. Host benchmarks and cross-compiles do not prove OS 9 behavior.
- [Atlas research](../../atlas/README.md), [atlas production boundary](../adr/0003-atlas-production-boundary.md), and [Python tooling policy](../python-tooling.md) already establish reproducible research and standard-library-first, separately pinned optional scientific tooling.
- [Entity and navigation contract](../design/entity-navigation-contract.md) owns the proposed **United States → Wisconsin → Madison → Back** acceptance journey; do not redefine it here.
- GitHub issues/PRs/accepted ADRs/live milestones remain authoritative for scope, evidence, decisions and current status. Link to them rather than copying a backlog into this note.
- [jackwillis/claude-plugins](https://github.com/jackwillis/claude-plugins) already offers *representing-and-intervening*, *causal-analysis*, *what-to-test*, *is-it-tested*, *propagation-trace*, *subtraction-audit* and [paired known-answer skill evaluations](https://github.com/jackwillis/claude-plugins/blob/main/docs/internal/evaluation.md). Reuse them.

## Product context, not a change to release scope

The owner-endorsed November 16 direction in [#35](https://github.com/jackwillis/globe/issues/35#issuecomment-6938328678) is to ship independent useful slices while considering the integrated **select → read qualified offline facts → navigate to a related place → Back** experience as a v0.2 graduation test. [#148](https://github.com/jackwillis/globe/issues/148) owns the separate approved native multi-window direction. Neither turns every related research idea into a required release feature.

High-value open questions include the semantic truth of sourced fact cards ([#137](https://github.com/jackwillis/globe/issues/137)), geographic identities and editorial review ([#52](https://github.com/jackwillis/globe/issues/52), [#53](https://github.com/jackwillis/globe/issues/53), #151–153), label hierarchy and place symbols ([#38](https://github.com/jackwillis/globe/issues/38), [#88](https://github.com/jackwillis/globe/issues/88)), pointer readout accuracy (#37), compact workspace behavior (#148), and reduced-motion/navigation tradeoffs (#20/#41). #116 is being handled in a separate session and is outside this work.

## Minimal experimental cycle

1. **Frame the decision.** Name the question, relevant user or system goal, and meaningful alternatives (including no change). Use Goal–Question–Metric where applicable.
2. **Model and predict.** State a favored and competing explanation; a predeclared primary outcome, guardrails and failure criterion. For a causal claim, define treatment, counterfactual, experimental unit and the estimand; use a DAG when causal identification is genuinely at issue. A deterministic parser test does not need a causal DAG.
3. **Choose the cheapest distinguishing test.** Consider an independent oracle, small reviewed or synthetic fixture, targeted mutation, metamorphic property, property-based generator, matched host benchmark, factorial experiment, cognitive walkthrough or observed human task. Method follows uncertainty, not vice versa.
4. **Check instrument feasibility before the final freeze, where warranted.** For an expensive or approximate reference, run a bounded, candidate-blind preflight on representative inputs at the *actual proposed decision precision*, coverage, computation/memory ceilings and stop rules. Verify the measurement boundary, known-answer controls and error or uncertainty justification. A looser preflight does not establish feasibility at a stricter final tolerance. If the instrument fails, redesign or report infeasibility *before* freezing; a preflight is not a scored candidate comparison. Simple deterministic tests need only a proportionate oracle check.
5. **Predeclare and freeze the protocol.** Specify inputs, controlled factors, randomization/blocking where useful, decision margin, reference uncertainty requirements, admission gates, thresholds, stopping rule, missing/failed-trial policy, exact toolchain and environmental restrictions *before scored observations*. No post-hoc relaxation, cherry-picked fixture subsets or unrecorded amendment.
6. **Run and preserve evidence.** Record commit, reproducible commands, input hashes, seed, OS/compiler/hardware/emulation, individual observations including failures, outliers and incomplete runs, and independent correctness checks. Do not access the G4, acquire a new source, or alter another agent's work without authorization.
7. **Classify validity before analyzing effects.** First establish that the instrument and protocol support the intended inference. Distinguish valid evidence that supports/refutes the hypothesis, valid but insufficient evidence, instrument-limited reference/convergence failures and protocol-invalid runs. The last two are *not* evidence against the candidate. Where valid, report paired differences and uncertainty, guardrail failures, counterexamples and alternative explanations. Repeated frames on one computer are not independent machines; host tests are not OS 9 acceptance.
8. **Recommend, then obtain an owner decision.** Outcomes: *adopt*, *reject*, *revise and repeat*, or *defer*; always record the evidence-validity classification separately from the decision. A successful experiment does not authorize implementation, merges, source approval or release-scope changes.

### Proportionality check

Before adding an independent implementation, statistical procedure, review round, or expensive control, name the specific failure mode it would detect, the cheapest credible alternative, and what observation would make further effort unnecessary. Use the smallest method that can support the decision. A demanding reference can be less scientific if its precision is infeasible and the limitation is discovered only after the protocol freeze.

### Choosing a method

| Decision problem | Useful first method | Key limitation |
| --- | --- | --- |
| Are geographic facts truthful under changed or missing inputs? | Adversarial fixtures, metamorphic properties, targeted mutations and independent expected outputs | Synthetic fixtures cannot establish worldwide factual accuracy |
| Are geographic IDs stable across atlas editions? | Metamorphic and differential tests against explicit concordance fixtures | Requires editorial identity decisions where disputed |
| Which labels are most useful at 640×480? | Matched blocked runs, then factorial studies; visual/user inspection | Collision score is not the same as human comprehension |
| Is the Information workspace understandable? | Cognitive walkthrough and observed tasks with counterbalanced order | A handful of users gives exploratory, not population-level, evidence |
| Does a rendering optimization improve actual performance? | Existing rendering registry and matched native protocol | CPU-side time is not full displayed-frame latency |
| Is further research worthwhile? | Approximate value-of-information and sensitivity analysis | Estimated probabilities are assumptions, not measurements |

## Evidence contract (small and domain-local)

Every substantive experiment's owning issue or PR should link a small protocol and result stored with the relevant domain. Use the existing rendering registry for rendering, atlas research for geography, and issue-specific docs for UX. At minimum preserve:

- **Identity:** experiment ID, owning issue, status, date, investigator, commit.
- **Question:** decision, hypotheses and alternative, method, experimental unit, population/domain.
- **Protocol and feasibility:** treatment/control, fixed and varied conditions, random seed, primary outcome, guardrails, decision margin, instrument-validation preflight where needed (representative input, tolerance, precision, resource ceiling and known-answer checks), admission conditions, stop rule, missing/failed-trial policy.
- **Provenance:** input fixture/source licenses and hashes, exact commands, relevant build and toolchain, machine/OS/emulation, authorization boundaries.
- **Observations:** full raw results, nonconvergent/partial/invalid trials and reasons, independently checked expected behaviors, analysis instructions, actual versus predicted.
- **Interpretation:** evidence classification (*supported/refuted on valid evidence*, *valid but insufficient*, *instrument-limited*, or *protocol-invalid*); uncertainty, counterexamples, alternative explanations, generalization limits, recommended next step. An incomplete measurement cannot become a negative candidate verdict.
- **Decision:** *recorded separately by the authorized owner*, not embedded as an automatic conclusion.

Simple Markdown/JSON/CSV is enough. No general database, telemetry service, new runtime dependency, or second issue tracker.

### Cross-session case: PE-AA-01 (#25)

A November 18 interagent peer review of the antialiasing experiment identified a specific process failure: an initial numerical-reference preflight demonstrated a tolerance of 1/1024, but the later approved decision margin required a convergence delta of 0.00009765625 without validating that stricter target on representative geographic geometry. The first 15 geographic references reached the fixed 1024-sample ceiling inconclusively. Exact A replay, independent instruments and blinding protected against a false renderer verdict; the infeasibility was nevertheless discovered late. This is an **instrument-feasibility finding**, not evidence for or against AA renderer B.

The PE-AA-01 steering session retains ownership of the experiment. This note records its transferable methodological lesson only: validate reference feasibility at the *actual* intended precision before a scoring freeze, preserve inconclusive runs, and obtain separate approval for any new protocol. Do not change #25's thresholds, ownership, artifacts or existing acceptance criteria from this document.

## Proposed, optional tooling

**Host-only Poodle Earth runner:** A standalone Python proof of concept was produced during the November 17 research discussion. It validates a protocol, generates a deterministic paired-condition schedule and reports conservative descriptive comparisons. Its demonstration data are **synthetic** and are not Poodle Earth performance or usability findings. Five basic unit tests passed in that prototype; it has not been integrated or independently evaluated. Import only pieces that prove useful after a real pilot. First improvements should be source/fixture hashes, complete provenance, explicit failure accounting and evidence export. Honor the existing pinned optional Python scientific environment; standard-library utilities should remain lightweight.

**Reusable Claude plugin (separate possible PR):** Candidate narrowly triggered skills *design-experiment* and *review-evidence* should extend, not restate, the existing systems-analysis and break-things skills. Test them against matched unassisted prompts on known-answer and adversarial cases. Use blinded or independently scored outcomes where feasible. Skill text is not evidence of skill effectiveness.

## First pilot: #137 geographic-facts correctness

**Research only.** A bounded synthetic pilot now exists as draft [PR #191](https://github.com/jackwillis/globe/pull/191); it does not establish runtime geographic truth. Before further work, recheck #137 assignment, comments and related PRs. Use existing permitted or entirely synthetic facts; no source acquisition or production-pack migration. Ask whether changes in entity identity, population date/definition, qualified missing values, conflicting assertions, or malformed provenance can silently create confident false cards. Check independent expected outputs and deterministic fail-closed behavior; record adversarial cases and what the current software does versus what a future contract should require.

**Potential result:** a bounded facts-validation specification and list of missing test guards for #137. **Not established by this pilot:** global geographic correctness, approval of source rights or geographic identities, G4 performance, OS 9 acceptance, or completion of #137.

## Proposed sequence

1. Review this note through a documentation-only PR; do not change milestones, AGENTS.md, or other agents' work.
2. Run one bounded #137 pilot, preserving its raw inputs, negative cases and output separately from this methods note.
3. Evaluate which pieces of the prototype were reused; import only the smallest tested runner seam if it helped.
4. Evaluate candidate skills in the separate claude-plugins repository using independent known-answer cases.
5. Return to v0.2 grooming with actual evidence. Classify release requirements, independently shippable slices, polish and research only through owner review.

**Process success means:** a new agent can find the hypothesis and original observations, reproduce the result, explain its limits, and identify the separate approved decision without reading a past chat transcript.
