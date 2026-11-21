# PE-AUTO-01 — Decision OBS-01: Observability over recollection

**Status:** **Owner approved — 2026-11-20.** Scientific acceptance methodology only.  
**Record revision:** OBS-01/1 (2026-11-20).  
**Scope:** The principle and its direct acceptance-workflow implications. **This record does not adopt PE-AUTO-01 v2.1 or amend any release gate, runtime contract, or accepted ADR.**

## Decision

### Observability over recollection

> **Acceptance evidence should be collected at the point where the relevant event or state is observable, using bounded instrumentation with defined semantics, provenance, and limitations. Objective measurements must not be replaced by retrospective human estimates when trustworthy instrumentation is available. Human judgment remains essential for experiential qualities and for observations that cannot reasonably be automated.**

Complementary operational guidance:

> **Design acceptance workflows to minimize manual repetition, transcription, and reconstruction. Ask the owner to judge the software, not to serve as its measurement equipment.**

Instrumentation is warranted by a consequential failure mode or uncertainty, not by mere measurability. This decision does **not** require instrumenting every measurable property.

## Normative interpretation

Acceptance evidence **must**:

- Distinguish instrumented measurements from human estimates; describe estimates as such.
- Identify observation source and boundary, units, and relevant provenance.
- Mark missing or unavailable measurements **not measured**, never passed.
- Avoid treating command acceptance, Model state, and native drawing/presentation as interchangeable observations.
- Respect resource bounds, responsiveness, and explicit filesystem/output consent.
- Separate evidence collection from owner-controlled hardware acceptance, waivers, merges, and releases.

Acceptance procedures **should**:

- Capture objective measurements when relevant events occur, not reconstruct them afterward.
- Prefer machine-produced evidence to manual transcription and minimize repeated owner actions.
- Preserve raw observations when independent analysis or reanalysis matters.
- Evaluate instrumentation interference when it could affect the claim being tested.
- Reuse existing clocks, counters, and diagnostics where practical.

### Evidence and oracle boundaries

Instrumentation records observations at defined boundaries; an independent oracle assesses whether those observations support a correctness claim. Observability completeness is **not** test adequacy.

A passing Model assertion does not prove QuickDraw pixels. A measured `wait settled` duration does not prove rendering correctness, actual scanout, or perceived responsiveness. A few `StackSpace()` samples do not prove worst-case stack headroom. A completed `pe-script/1` transcript does not, by itself, establish successful close/flush/persistence or release readiness. Do not infer any of these stronger claims from weaker evidence.

## Operating rule for acceptance agents

**Do not use the owner as a substitute for available machine instrumentation.**

For future ChatGPT, Claude, and Codex sessions:

1. Identify the exact approved candidate, script/scenario, target, and intended observation.
2. Ask the owner to perform only the necessary approved actions.
3. Obtain available transcripts and diagnostics **before** requesting recollections.
4. Independently validate completeness, expected results, provenance, and the observation's limits.
5. Ask the owner about appearance, usability, or unexpected behavior that the evidence cannot adequately establish.
6. If a duration or resource measurement was not instrumented, record **not measured**; do not solicit a substitute numeric guess.
7. Present findings and gaps separately from the owner's acceptance and release decisions.

The goal is fewer low-value questions and stronger evidence, not the elimination of human testing.

## Motivation: native Classic acceptance, 2026-11-20

The owner-led first limited native acceptance of [#221](https://github.com/jackwillis/poodle-earth/issues/221) / [draft PR #222](https://github.com/jackwillis/poodle-earth/pull/222) on a physical Power Mac G4 running bare Mac OS 9.2.2 established that the script runner could execute scenarios and save JSONL that was independently parsed on the host. See the [native acceptance report](https://github.com/jackwillis/poodle-earth/issues/221#issuecomment-7052571215).

However, its `pe-script/1` records did not quantify successful wait durations or internal event-loop/resource behavior. The owner's approximate timing recollection was **not** an instrumented measurement. [#224](https://github.com/jackwillis/poodle-earth/issues/224) proposes timing observations; **this decision does not authorize or implement #224**. Unperformed native checks remain unverified; historical evidence is unchanged.

## Document lineage and authority

The related source documents were inspected in the owner's file library. At the time of recording, neither was tracked on the repository's `main` branch:

| Source document | SHA-256 of original file bytes | Approval status |
| --- | --- | --- |
| `PE-AUTO-01_research_protocol.md` | `92fd518c428beab628e75683ece58f2ed36f2da6dc6b62202e6d90157d3571d9` | Original **proposed** research draft; hash independently verified |
| `PE-AUTO-01_protocol_v2.1_reconciled.md` | `bcf8c916c96183c62b896d600248947328f6096427aa5416635a93f20d8d7a7b` | **Proposed**, distinct reconciliation; hash independently verified |

OBS-01 is a **separately approved, narrow decision** related to the proposed v2.1 §4 (*Oracles and false passes*) evidence contract. It does not silently promote that document, its hypotheses, experiments, external-control proposals, or stages to an adopted protocol. This record preserves the source drafts by reference, not by altering or republishing them.

The existing [scientific experiment registry](rendering-experiment-program.md), [benchmark procedure](../g4-benchmark.md), and [metrics/diagnostics issue #59](https://github.com/jackwillis/poodle-earth/issues/59) remain responsible for their own scopes. No scripting schema, benchmark, instrumentation, source-data policy, or release requirement is changed by OBS-01.

**Authority:** Owner's explicit 2026-11-20 methodological approval. Further protocol adoption, runtime implementation, G4 operations, merges, and release decisions require separate authorization under [AGENTS.md](../../AGENTS.md).
