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

Predictions P1–P10 above were committed (`a56c04e`) before `edition_selection.py` or its tests existed (see git history). The text above this line is unchanged since that commit.

## 3. Reproduce

```sh
python3 -B -m unittest discover -s atlas/research -p 'test_edition_selection.py' -v
```

Stdlib only; reads no file, writes nothing, needs no network.

## 4. Results

**Host:** Linux x86_64 (Fedora), Python 3.14.8, 2026-11-16, on `origin/main` `0b5b16d` plus this branch. Host-only; no G4, Retro68 or Classic Mac run.

**47 tests, 47 pass** on the final revision. Count by area: identity stability 3, qualified queries 5, contextual queries 5, distinct statuses 3, no inferred identity 5, semantic recording and snapshots 10, pending resolution 7, owned results and leases 3, retirement table 4, removed edition 2.

| Prediction | Outcome | Evidence on this fixture |
| --- | --- | --- |
| P1 identity stable | **Held** | All 16 ordered source subsets (one test computes the expected count, 16 × 10 refs × 2 datatypes = 320 queries), including none installed. Every `Result.ref` equals the request; statuses stay in {ok, detail-unavailable}. |
| P2 qualified never falls back | **Held** | Removing the pinned source gives `source-missing` while the *contextual* query for the same ref still returns another source's value (the two behaviours differ on purpose). Wrong revision gives `revision-missing`, never an upgrade. Pin is independent of priority order. |
| P3 contextual provenance | **Held** | Result lists chosen, shadowed and `source-missing` sources in priority order and the disagreeing lower-priority claims. Priority flip changes value and provenance, not identity. Adding an unrelated source does not reorder. |
| P4 distinct statuses | **Held** | Six situations give six statuses. An absent optional source never yields `entity-unknown`. |
| P5 no inferred joins | **Held** | A same-name, same-coordinate record with only a *proposed* concordance contributes nothing; a second proposed link to an entity with no approved record leaves it `detail-unavailable` and its name unfindable. "Madison" is `ambiguous` in both editions (two cities in World; city and town in Wisconsin). "Springfield" is unique in each edition and ambiguous in their union. |
| P6 recording | **Held** | `Name~XXXXXXX` (7 Crockford characters carry 32 bits; round trip over the fixture and 0 / 1 / `0xFFFFFFFF`). A contradictory hint yields `hint-mismatch` and keeps the ID's entity. Malformed or out-of-range suffix is an error, never a name lookup. Replay after a rename follows the ref. Hint-only form is ambiguous where the ID form is not. |
| P7 snapshots | **Held** | Insertion order does not change `snapshot_id`; removal, priority, edition and revision each do. Pinned records replay identically across three configurations. A contextual record replayed on a different snapshot returns the new value *with* `snapshot-drift`, or `snapshot-drift` with no value under `strict`. |
| P8 stale completions | **Held** | See below. |
| P9 owned results, leases | **Held, with a modelling limit** | Results are plain values (recursive type check over all 16 configurations × 12 refs). A view read after `retire()` raises; a value copied before retirement survives. This models the contract; it cannot show C++ lifetime safety. |
| P10 retirement | **Held** | Chains flatten to the terminal active ID; a split stays a split; cycles, dangling successors, ID reuse, dangling or nested split members are rejected. Lookup is one dictionary read. |

### Asynchronous resolution (P8) in detail

1. Navigate to a World-only entity; the effect handler finishes under the World configuration; the user switches to Wisconsin before delivery. The old completion is dropped (`dropped` in the bounded log; model otherwise unchanged). `update` re-issues the effect for the **same reference**; the Wisconsin edition would resolve the *name* "Springfield" to a different entity, and the test shows that never happens. The re-issued completion commits the original entity with `detail-unavailable`.
2. Forged completions (token, generation, ref, purpose, or a result for another ref) are each dropped; only the matching one commits.
3. A second navigation supersedes the first; a late first completion is dropped.
4. A failed resolution (`entity-unknown`) leaves selection and history alone.
5. All 27 patterns of three configuration changes interleaved with delivery, plus a late duplicate replay, never commit anything but the requested reference.

### Mutation check (does each test fail for the stated reason?)

Seeded defects, each applied alone and the suite re-run; all 15 were caught:

| Seeded defect | Caught by | | Seeded defect | Caught by |
| --- | --- | --- | --- | --- |
| accept proposed concordance | 4 tests | | hide conflicts | 1 |
| pinned query falls back | 3 | | snapshot ignores priority | 2 |
| drop generation check | 1 | | snapshot ignores revision | 2 |
| drop both ref checks | 1 | | split collapsed to first successor | 4 |
| ignore purpose | 1 | | view never stale | 1 |
| reissue rebinds to new-edition name | 2 | | first-match for ambiguous name | 6 |
| name wins over ID suffix | 3 | | skip cycle check | 1 |
| failed navigation commits | 1 | | | |

### Corrections and honest limits recorded during the run

- The first run failed one test (`n > 500`). That was an arbitrary bound of mine; the true count is 320. Replaced with the computed count. No prediction failed.
- The first mutation pass **survived** three mutants: the proposed-concordance fixture record sat after an approved record and was masked (test and fixture strengthened), one ref check duplicates another (redundant defence; mutated both), and one mutant was a no-op (`^ 0`, discarded). One further mutant died on a `NameError`, which proves nothing; it was replaced by the real rebinding mutant above.
- Two mutants (`msg.ref` and `msg.result.ref` checks) are redundant with each other on this fixture. Whether to keep both in C++ is a design choice, not something this shows.
- Nothing here validates real concordances, real Wisconsin or World data, any population figure, binary layout, `sizeof`, allocation behaviour, or Classic Mac behaviour. A pass means the contract is self-consistent and testable on a toy.
- The fixture implements exactly one conflict policy: first claim in priority order wins, lower disagreeing claims are listed. It is a probe, not a recommendation about real geopolitical disputes.

## 5. Findings: what must stay stable

1. **Identity is edition-independent.** The canonical `EntityRef` and the retirement table live in the always-embedded catalog, so a held reference is never `entity-unknown` merely because an edition was removed. The cost: the embedded catalog must contain every identity any published edition can name, or editions must declare identities the catalog lacks as `entity-unknown` honestly. The first is the toy's assumption; it is a real size question for #52/#26.
2. **Source records are reachable only through approved concordances.** Name, coordinates and nearness are never a join key, including inside one publisher's pack.
3. **Two query modes, different guarantees.** *Contextual* = "what does my current setup say", may change, must report provenance. *Qualified* = "what did this source at this revision say", must not change and must not fall back. A script or tour must say which it is.
4. **Five (six) failure states are different facts** and each deserves a different message and a different automated response (see the table below).
5. **Recording is a pair.** A readable hint is for people; the reference is for machines. Hints never rebind. The configuration snapshot matters only for contextual records; for pinned records the pin is the snapshot.
6. **Asynchronous work is bound to `(token, generation, ref, purpose)`.** Switching edition invalidates in-flight completions *and* re-issues for the same ref. It never re-resolves a name.

| Status | Meaning | Suggested user-facing response |
| --- | --- | --- |
| `entity-unknown` | Reference is not in the catalog (or name matches nothing) | Report; do not guess |
| `detail-unavailable` | Entity known; no installed source has this detail | Keep identity, say what is missing |
| `source-missing` | A *qualified* query names a source that is not installed | Say which source; offer nothing else |
| `revision-missing` | Source installed at another revision | Say both revisions; never upgrade silently |
| `ambiguous` | Several entities fit | List candidates; never first-match |
| `retired` | One terminal successor | Show successor, mark as redirected |
| `split` | Several successors | List them; ask |
| `snapshot-drift` (strict replay only) | Contextual record on a different configuration | Offer replay-with-warning or pin |

## 6. Proposed contract additions (for owner review; none adopted)

Additions to the [entity/navigation contract](../design/entity-navigation-contract.md) and, if accepted, to draft ADR 0005. All are proposals.

1. **Two query modes** in the resolver seam: contextual `(ref, datatype)` and qualified `(ref, datatype, source, revision)`. A qualified result must never contain another source's value.
2. **`Resolution.status`** extends from available / detail-unavailable / unknown / retired-ambiguous to the eight values above; `source-missing` and `revision-missing` apply only to qualified queries.
3. **Provenance fields** on every resolution: chosen source and revision, and a bounded list of skipped or shadowed sources with reason and any conflicting claim. Bound the lists (the toy has at most three sources).
4. **Generation stamp** on every effect and completion: `ResolveEffect{token, generation, ref, purpose}`; update accepts a completion only when all four match the pending request. A configuration change bumps the generation and re-issues the same `(ref, purpose)`.
5. **Source-configuration snapshot** (edition, installed `(source, revision, digest)`, per-datatype priority) with a deterministic ID. Stored in contextual recorded commands; compared at replay; mismatch is reported, never silent.
6. **Semantic command record**: `{verb, ref, readable hint, mode, snapshot-or-pin}`. Saved or copied forms always carry the reference. Textual form `hint~ID` with a strict terminal grammar and no name-only fallback when a suffix is malformed.
7. **Concordance status** stays `approved | proposed` in the published catalog; only `approved` is consumable by the runtime.
8. **Retirement table** semantics as already drafted (flat, builder-validated, never ID reuse); add the test cases from P10.
9. **No view or pointer in a result**: restate as a test, using a generation-leased view in the fixture, until the C++ equivalent exists.

## 7. Open owner decisions

1. **Embedded catalog scope.** Does the always-present catalog hold *all* identities any edition may name (this note's toy assumption)? If an add-on edition can introduce new entities, how does a held reference to one behave when that edition is absent: `entity-unknown` (honest but alarming) or a catalog-level stub?
2. **Default mode for saved scripts and tours.** Contextual with snapshot (tracks the user's setup; drift warns) or pinned (reproducible; can go stale)? Recommendation to test with users: *reference always; pin only for facts/values the author wants frozen*.
3. **Strict versus loose replay policy** on `snapshot-drift`, and whether the Listener should surface it.
4. **Conflict policy** for contested claims: this fixture lists them and picks by priority. Real disputed-classification policy is #52/#53 work.
5. **Navigation commit on `detail-unavailable`.** The toy commits identity; real navigation needs an anchor. Which source supplies the anchor when detail is missing?
6. **Hint-name escaping and checksum** in `hint~ID` (literal `~`, optional check character). The toy rejects any malformed `~word` suffix, which implies names containing `~` need an escape rule.
7. **CI.** The harness is not wired into `.github/workflows/checks.yml` (that would enable automation, and PR #179 edits the same file). Add a step after #179 merges, or keep these as run-on-demand research?
8. **Keep both ref checks?** The `msg.ref` and `msg.result.ref` checks overlap in the toy; keep both as defence in depth or one?

## 8. Not done, by design

No interpreter, production resolver, pack-format change or edition UI; no data source or licence; no G4 or hardware access; no overlap with #48 (nothing loads a file). Release scope, milestones, labels and ADR statuses are unchanged; ADR 0005 stays Draft.

## 9. Post-review addendum (2026-11-16): order-independence of selection

**Provenance of this section.** Sections 1–8 above are the original experiment and are not rewritten. An [independent review of PR #182](https://github.com/jackwillis/globe/pull/182) at `dac0114` found two defects that the original predictions did not cover (the original P1–P10 held on the fixture as written; they did not test these inputs). This section is a *second* preregistration, committed before the harness was changed. Items below are labelled **[review]** where they exist only because of that review.

### 9.1 The two defects (both reproduced from reading the code, not yet from a test)

- **[review] Defect A, installation order as priority.** `make_config()` ends with `ids = [s.id for s in installed]` and uses it as the default priority for every datatype. Installing `(wisc, census)` versus `(census, wisc)` therefore changes which Madison population wins with no user choice. The original `test_snapshot_id_is_order_independent` avoided this by passing equivalent explicit priorities.
- **[review] Defect B, first-match inside a source.** `query()` collects every approved record of one source for the entity and takes `claims[0]`; the pinned path likewise returns the first matching record. Two local records in one source, both approved for one entity with different values, collapse to whichever comes first, and the `conflicts` field never sees it.

### 9.2 Policies under comparison (tested; none adopted)

Finding A: **A** explicit priority required whenever overlapping claims disagree; **B** a default derived from declared publication metadata (here a `supersedes` relation, not publisher-name sorting); **C** overlap without precedence stays unresolved. Two controls: the legacy install-order default (known defective) and alphabetical source-ID order (deterministic but arbitrary).
Finding B: **A** builder-side rejection of conflicting records before publication; **B** query-time refusal that lists all candidates; both compared on the same inputs.

### 9.3 Predictions and failure criteria

| # | Prediction | Failure criterion |
| --- | --- | --- |
| P11 | **Control:** the legacy install-order default is order-dependent (the review is right), reproduced in a test. | The legacy default is order-independent on the Madison fixture, which would mean the review's claim is wrong. |
| P12 | With no implicit priority, every permutation of the same installed set gives identical results, provenance, conflicts and snapshot ID, for 3 and 4 sources. | Any difference between permutations. |
| P13 | Disagreeing claims from several sources with no explicit priority give `precedence-unresolved` and **no value**, listing every candidate. A single claimant, or claimants that agree, resolve without a priority. | A value is returned for a disagreement without explicit precedence, or a harmless single-claimant query is refused. |
| P14 | An explicit priority naming all disagreeing claimants picks the winner and changes the value when reordered, with the entity reference unchanged. A priority that omits a disagreeing claimant is `precedence-unresolved`, not a partial win. | Reordering leaves the value unchanged where claims disagree, the reference changes, or an incomplete priority picks a winner. |
| P15 | A derived default from `supersedes` is install-order-independent when the relation is total and acyclic over the installed sources, and absent (falling back to unresolved) when sources are unrelated, partially related or mutually claim supersession. Alphabetical ID order is also order-independent but flips under a publisher rename, so it is not authority. | A derived default depends on install order; mutual or partial declarations pick a winner; alphabetical order is rename-stable. |
| P16 | Within-source contradictory claims: the builder check rejects them, and the query-time check (if the builder is bypassed) returns `source-conflict` with every candidate, for contextual and pinned queries, for every ordering of that source's records. Equivalent duplicate records are accepted and give the same result in every order. | Any ordering returns a value for a contradiction, the contradiction does not appear in the result, or equal duplicates are rejected or order-sensitive. |
| P17 | A record with only a *proposed* concordance never creates or hides a within-source conflict. | A proposed record changes a result. |
| P18 | Two installed sources sharing an identity: byte-identical duplicates collapse to one; any other duplicate (different revision or content) is rejected at configuration construction regardless of order. Duplicate IDs in a priority list are rejected. | A duplicate is silently accepted, or acceptance depends on order. |
| P19 | The original 47 tests, P1–P10 and seeded-mutant coverage are preserved. Only two documented test edits are expected: helper rename where a test *intends* an explicit user ranking, and one test that installed two sources with the same ID. | Any original assertion weakened, or any original mutant now surviving. |

**Recommendation prediction (to be confirmed or refuted):** the smallest contract is explicit priority plus unresolved-by-default (policy A+C), with declared metadata (B) offered only as a *suggestion that the settings UI materializes into explicit, stored priority*, never computed implicitly at query time; and builder-side rejection for within-source conflicts with a cheap query-time refusal as defence in depth. No global geographic-authority ranking is proposed.

### 9.4 Reproduce

```sh
python3 -B -m unittest discover -s atlas/research -p 'test_edition_selection.py' -v
```

### 9.5 Results (Linux x86_64, Python 3.14.8, host-only, branch `research/edition-selection-semantics-20261116`)

**73 tests, 73 pass.** The original 47 are retained: 46 unchanged apart from the helper rename below, and one rewritten (see edits). New: 22 for the two findings and 3 for replay and hint consequences, plus 1 added when an original test was split in two.

| Prediction | Outcome | Evidence |
| --- | --- | --- |
| P11 control | **Held** (the review was right) | `install_order_config((WS, C))` gives 270 and `((C, WS))` gives 280 for the same entity. Reproduced, not just read. |
| P12 permutations | **Held** | All 6 permutations of 3 sources and all 24 of 4: identical results for every entity and datatype, identical snapshot ID, with no priority and with an explicit priority. |
| P13 unresolved | **Held** | Madison population from World 100, Wisconsin 270, Census 280, no priority: `precedence-unresolved`, no value, all three candidates, listed sorted by source ID. A lone claimant and agreeing claimants resolve without a priority. |
| P14 explicit priority | **Held** | All 6 rankings give the first-ranked source's value (100, 270 or 280); the reference never changes. Priorities that omit a disagreeing claimant (three cases) give `precedence-unresolved`, not a partial winner. |
| P15 declared metadata | **Held, with a stated limit** | A total, acyclic `supersedes` chain derives the same order from all 6 install orders. An unrelated source, a mutual declaration, or no declaration yields `None` (falls back to unresolved). Alphabetical order is install-order-independent but a publisher rename flips the winner. The limit: a `supersedes` claim is publisher-asserted metadata; the toy cannot tell a legitimate refinement from a self-promotion. |
| P16 within-source conflict | **Held** | `validate_source` rejects a contradictory source and names it; equal duplicates pass. With the builder bypassed, contextual and pinned queries return `source-conflict` listing every candidate, for all 24 and 6 orderings of the records; equal duplicates give one identical result for every ordering. A pinned query on a different, clean source is unaffected. |
| P17 proposed concordance | **Held** | A contradicting record with only a proposed link neither creates a conflict nor hides one. |
| P18 duplicate identity | **Held** | Same ID with a different revision or content is rejected in both orders; byte-identical duplicates collapse to one with the same snapshot ID; a priority naming a source twice is rejected. |
| P19 preservation | **Held, with documented edits** | See below. |

### 9.6 What changed in the harness

- `make_config` no longer derives a priority: priorities exist only if the caller supplies them. `ranked_config` states an explicit user ranking; `install_order_config` is kept as a labelled negative control.
- `Config` rejects duplicate source identities (collapsing byte-identical ones) and duplicate priority entries, and keeps `installed` sorted by ID, so installation order is not representable in a configuration.
- `Source.digest` is independent of record order. `Source.supersedes` is optional metadata read only by `derive_declared_order`, never by `query`.
- Contextual `query` ranks only by the explicit priority. New statuses: `precedence-unresolved` and `source-conflict`. A lone claimant is its own source and needs no rank.
- Previously an installed source missing from the priority list was silently ignored. Now, if its claim disagrees, the result is unresolved.

**Edits to original tests (all for stated reasons):** (1) 46 tests renamed `make_config` to `ranked_config` because they intend an explicit user ranking; (2) `test_agreeing_lower_source_is_not_a_conflict` used the same source ID twice, which is now rejected, so it uses a distinct mirror ID; (3) `test_unrelated_source_does_not_reorder` added World, which overlaps Madison, and relied on unranked sources being ignored; it now adds a genuinely non-overlapping source, and a new test asserts the overlapping case is unresolved. No assertion was weakened; the first behaviour change (3) is intentional.

### 9.7 Mutation check

27 seeded defects (the original 15 re-run on the new code, plus 12 new), each applied alone; **all 27 caught**. New mutants: install order as priority (5 failing tests), first-match inside a source (5), no unresolved branch (4), incomplete priority picks partial winner (2), drop duplicate-source check (1), installed tuple keeps given order (1), unsorted claims let record order decide (1), pinned query bypasses source-conflict (2), `supersedes` ignores cycles (1), digest depends on record order (1), lone claimant loses its source (3). Caveat: two of these (incomplete priority; unranked disagreeing claim ignored) are the same code mutation listed twice, so 26 are distinct, and the "no unresolved branch" mutant is caught partly by errors, not only by assertion failures.

### 9.8 Negative findings and corrections made during this pass

- **A real defect in my first fix, found by the new tests:** a lone claimant reported `source=None` and lost its provenance. Fixed; covered by a test and a mutant.
- **Three of my own expectation errors** (sort order of `wisc` before `world`, twice; a catalog rebuilt without earlier approvals) were test mistakes, not contract failures.
- **Usability cost, not a bug:** installing an overlapping edition makes previously resolved values `precedence-unresolved` until the user ranks it, and a contextual record saved before the install then replays as unresolved (with `snapshot-drift`; `strict` refuses). Pinned records are unaffected. This is the intended safety behaviour and a real UX burden.
- **Heavy-handed by design:** a source that contradicts itself blocks the contextual query for that entity and datatype even if ranked below a clean source. That is simplest and loudest; a gentler rule (skip and report) would need a policy decision.
- **Unresolved names produce a reference-only hint:** with unranked sources that disagree on a name, the readable form is `~ID`, still replayable.
- **Not tested:** unresolved handling for `lookup_name` (name matching considers every installed source's names, which is order-free but not priority-aware), more than three datatypes, bounded sizes of candidate lists, and anything about real data.

### 9.9 Updated recommended semantics (proposals, not accepted architecture)

1. **No implicit precedence anywhere.** Installation, enumeration and record order never choose a value. `Config.installed` is a set.
2. **Priority is explicit stored user choice** (per datatype), part of the configuration snapshot. It matters only where installed claims *disagree*. Policy A plus C.
3. **Unresolved is a first-class result.** `precedence-unresolved` lists every candidate with source and revision and gives no value. Policy B (declared `supersedes`) is at most a *suggestion* a settings UI may offer; accepting it writes an explicit priority. It is never evaluated at query time, and a mutual or partial declaration offers nothing.
4. **One claim per source, entity and datatype.** Builder-side `validate_source` rejects contradictions before publication (the contract); the reader refuses with `source-conflict` rather than choosing (defence in depth, a single scan, no engine). Equal duplicates are tolerated.
5. **Duplicate source identity** is rejected unless byte-identical.
6. **Recording:** unchanged from §6, with two additions: a contextual record stores the priority (it already does via the snapshot), and replay may return `precedence-unresolved` after an install even though no rank was changed.

### 9.10 Remaining owner decisions (adds to §7; none settled here)

1. Whether to adopt "explicit priority, otherwise unresolved" as the product default, or offer a *confirmed* declared-metadata default at install time. No global geographic-authority ranking is proposed and none should be implied.
2. Who may publish `supersedes` and how it is reviewed (it is self-asserted metadata).
3. Whether a self-contradicting low-priority source should block the entity (this toy) or be skipped with a warning.
4. How an install flow asks the user to rank when a new edition overlaps and disagrees, and how this appears in the Listener transcript.
5. Whether pinned records should be the default for saved tours (they survive installs; contextual records do not).
6. Bound on candidates/provenance entries stored per result (the toy keeps all).
7. CI (below).

### 9.11 Implications for edition-selection UX and Listener replay

- **Selection UX:** the first overlap between two editions needs a one-time ranking decision, per datatype; until then the product must display "sources disagree" instead of a number. Reordering is a user action and should be visibly distinct from installing.
- **Listener:** readable forms keep their `hint~ID` shape; a replayed contextual command can come back unresolved, so the transcript needs an unresolved/drift line, not a silent different answer. Commands that must be reproducible should be pinned.
- **Scripts and tours:** reference always; pin values the author wants frozen; contextual otherwise, accepting that an install may make it ask.

### 9.12 CI

Not enabled. [PR #179](https://github.com/jackwillis/globe/pull/179) is still open and edits the same workflow. After it merges, one possible step, for the owner to approve, placed next to its composition step:

```yaml
      - name: Edition-selection semantics (stdlib, research)
        run: python3 -B -m unittest discover -s atlas/research -p 'test_edition_selection.py' -v
```

It reads and writes nothing, so it is cheap and not a release gate. The reviewer's point stands: until then GitHub Checks does not certify these 73 tests; they are author-run.
