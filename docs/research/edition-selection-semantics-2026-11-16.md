# EXP-EDSEL-01 — Edition-selection semantics for durable geographic references

**Date:** 2026-11-16
**Status:** Exploratory research, host-only, synthetic data. **Not** an accepted ADR, a Reader contract, a pack format, a resolver implementation, a milestone gate or a hardware claim. [Draft ADR 0005](../adr/0005-entity-identity-and-navigation.md) stays a draft; nothing here marks any of its decisions accepted.
**Context:** #52 (identity, concordance, editions), #63 (Listener/scripting references), #114 (presentation parity), [PR #177](https://github.com/jackwillis/globe/pull/177) (bazaar synthesis), [PR #179](https://github.com/jackwillis/globe/pull/179) (A/D/E composition experiment). Overlap with #48 (external files) is deliberately avoided: this note never loads a file.

## 1. Product question

A user may switch between World and Wisconsin editions, or choose different publishers for different data types. When a script, saved tour, navigation history entry or GUI selection refers to a geographic object, **what must stay the same, what may change, and what must be reported rather than papered over?**

## 2. Preregistration (written and committed before any harness code)

### Model of the system

`reference → (active source configuration) → resolution result`. Three things are easy to conflate and must stay separate:

1. **Entity identity**: Globe-owned canonical reference. Independent of editions, publishers, geometry, names.
2. **Source claim**: a publisher's record about an entity (name, population, geometry detail), reachable only through an *approved concordance* from `(publisher, local id)` to the canonical reference.
3. **Selection context**: which sources are installed and in what priority per data type. This is the thing that changes when the user switches editions.

Hypothesis: if (1) never depends on (3), and every result states which source and revision produced it, then edition switching is safe by construction, and the remaining hazards are *time-of-check* hazards (asynchronous completion) and *recording* hazards (what a saved script stores).

### Predictions (each with a failure criterion)

| # | Prediction | Failure criterion (rejects the toy contract) |
| --- | --- | --- |
| P1 | A canonical reference resolves to the same entity under every installed-edition subset and every source-priority order (including none installed). Only provenance and detail availability vary. | Any reference changes identity, or a result's `ref` differs from the requested ref, under any configuration. |
| P2 | A source-and-revision-qualified query returns exactly that source's claim, or an explicit `source-missing`, `revision-missing` or `detail-unavailable` status. It never returns another source's value. | A qualified query ever returns a value whose provenance differs from the qualification. |
| P3 | A contextual query returns the highest-priority source that has the claim and reports provenance: chosen source and revision, each skipped higher-priority source with its reason, and any conflicting claim from a lower-priority source. | A contextual result lacks provenance, hides a conflict, or reorders when only unrelated sources change. |
| P4 | Five unavailable-like outcomes are distinguishable by status alone: `entity-unknown`, `detail-unavailable`, `source-missing`, `ambiguous`, `retired` (and `split`, which is ambiguous by construction). No status is derived by catching another status. | Two distinct situations produce the same status, or an absent optional source is reported as `entity-unknown`. |
| P5 | Matching names or coordinates never create a join. A record with the same name and coordinates as an entity but only a *proposed* (unapproved) concordance does not contribute to that entity. Name lookup returning several candidates is `ambiguous`, never first-match. | Any proposed/unreviewed link affects a result, or an ambiguous name returns a single answer. |
| P6 | A recorded command stores a readable description plus the authoritative reference. Replay resolves by reference; a stale or contradictory readable hint yields a diagnostic and **never** changes the target. Hint-only (no ID) forms are rejected for saved/replayable use when the name is ambiguous in the recorded edition. | Replay by name changes identity when the catalog changes; a wrong hint rebinds. |
| P7 | A source-configuration snapshot (installed sources with revision digests, per-type priorities, edition) is deterministic: equal configurations give equal snapshot IDs regardless of insertion order, and any change to a source revision, priority or removal changes it. Replay against a differing snapshot reports drift explicitly; pinned queries replay identically across drift while contextual queries are flagged. | Equal configs give different IDs, differing configs give equal IDs, or drift is not reported. |
| P8 | With a request pending, an edition switch (or priority change, or source removal) makes the old completion **unable to overwrite newer state**, and the follow-up resolution is for the *same reference*, never for a name re-resolved in the new edition. Completions are matched on `(token, generation, requested ref, purpose)`. | A stale completion is applied, a completion for a different ref or purpose is applied, or the new generation re-resolves the original *name* to a different entity. |
| P9 | Resolution results are owned values: they contain no live view handle. A pack view obtained under generation N raises/rejects after the generation retires, and a result copied out before retirement remains valid. | A retired view is readable, or a result embeds a view. |
| P10 | A retired reference resolves in one bounded step to its terminal active successor (builder flattened); a split reference returns all successors with status `split` and is never auto-resolved to one; cycles and dangling successors are rejected at publication. | Chain traversal at lookup time, an arbitrary choice for a split, or an accepted cycle. |

### Fixture design (invented; no real data, no real publisher, no real geography claim)

Two synthetic editions and a few canonical entities, using the reserved synthetic range (high bit set) from the [contract](../design/entity-navigation-contract.md#provisional-prototype-constraints). Names resemble the acceptance journey only so the fixture is readable; **no source of truth for Wisconsin, Madison or any real population is asserted**.

- **World** edition: coarse, publisher `world.example`. Lists two settlements both named "Madison" (ambiguous in this edition) and a microstate with a record but no detail. Has no record for the state polygon.
- **Wisconsin** edition: publisher `wisc.example`, with state and city detail, plus a second publisher `census.example` whose city population **conflicts** with `wisc.example`.
- **Missing detail**: an entity that is known to the canonical catalog but has no record in any installed source.
- **Unapproved concordance**: a `world.example` record with the same name and coordinates as a Wisconsin entity, linked only as `proposed`.
- **Retired and split**: a retired entity that merges into one successor; an entity split into two successors; both produced by a flat builder table.
- **Removed edition**: Wisconsin uninstalled while references to its entities are held.
- **Changing priority**: flip `population` priority between `wisc.example` and `census.example`.

### Method and honesty limits

Stdlib Python, in the style of `atlas/research/composition_ade.py` (PR #179): a small model of the *semantics*, not the C++ type layout. It cannot show `sizeof`, allocation behaviour, or pointer lifetime in C++; "borrowed pointer" is modelled as a generation-leased handle. A pass means the **contract is self-consistent and testable on this fixture**, nothing more. It does not validate real concordances, real Wisconsin data, Classic Mac behaviour, or any binary format.

Predictions P1–P10 above were committed before `edition_selection.py` or its tests existed (see git history).

## 3. Results

_(filled in after execution; see below)_
