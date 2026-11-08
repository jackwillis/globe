# Atlas contract session handoff — November 8, 2026

## Durable recovery state

Worktree: `/tmp/globe-atlas-contract`. Branch: `codex/atlas-contract`.
Base: `52ccb33dc6db1c8f19ccded422260e399defeae1` (fetched `origin/main`).
Implementation/research checkpoint: `2afaebe` — source-scoped identities and equal-area selection.
A subsequent local documentation commit records this handoff; recover its full hash with
`git log -2 --oneline codex/atlas-contract`. This branch is **not merged or pushed**. No remote
publication, issue edits, release, production-pack change or hardware access was performed by this session.

The implementation commit contains all code, small fixture data, reports and test results.
Full selected settlement outputs are deliberately untracked outside the repository at
`/tmp/globe-selection-a` and `/tmp/globe-selection-b`. Regenerate them with the commands in
`atlas/README.md`; they are not needed to recover the code or inspect the committed measurements.
The linked worktree is in temporary storage: do not rely on that directory for long-term retention.
The named Git branch/commits are retained in the owner's repository independently of its files.

## Workspace reconciliation

At entry `/home/jack/code/globe` was on `main`, observed at `be0278d`, matching then-cached
`origin/main`. It had no staged changes and two unstaged files:
`include/globe/render.hpp` and `src/render/render.cpp` (typed refinement phases).
No nonignored untracked files were present. Ignored content was source archives, edition outputs,
Python caches and build directories; it was not staged or cleaned.

The preexisting `stash@{0}` at `2637e7e` contains six files including the same renderer changes,
CMake and platform work. It remains untouched. Renderer WIP ownership was uncertain, so it remains
in its original worktree rather than being applied, committed or discarded by this session.

Initial branch heads (all ancestors of the then-current main; **zero unique commits**):

| Branch | Head |
| --- | --- |
| codex/atlas-audit | 39e46f1 |
| codex/issue-7-ci | 9c78bc9 |
| codex/issue-7-fix-ci | 4183835 |
| codex/os9-cooperative-scheduling | f8b8159 |
| claude/classic-mac-ux | e90ce28 |
| claude/os9-parity | bac789a |

The only other initial worktree was
`/home/jack/.codex/worktrees/progressive-resolution/globe`, branch
`codex/os9-cooperative-scheduling`; it was clean when inspected. The old atlas-audit branch exists
but has no active worktree. No historical branches were deleted.

After observing local state, an authorized `git fetch origin` succeeded outside the read-only
Git-metadata sandbox. The restart handoff missing from the initial checkout was fetched/read via
GitHub and is present in base `52ccb33`. All relevant issue bodies and comments were read, including
a successful retry for #38. #53 explicitly requires owner final acceptance and stays open.

**Concurrent changes observed:** another session advanced local main to `e3de54a`, three commits
ahead of fetched `origin/main` at `52ccb33`: menu implementation `18b08b5`, Tiger launch evidence
`4e40036`, packaging findings `e3de54a`. Their commit summaries and changed-file boundary were
inspected. They change platform/menu/packaging files and no `atlas/` or `data/` files. No merge or
checkout in the owner's tree was performed by this atlas session. The new
`/home/jack/code/globe-menubar` worktree on `claude/menu-bar` at `4e40036` was clean and its work
already reachable from local main. The two original renderer WIP files remain unstaged on main.

At handoff the atlas worktree is clean after the handoff commit; check live status before any new
work. The branch is one atlas implementation commit plus this handoff ahead of `52ccb33`, and does
not include the three later local-main platform/packaging commits. No conflict was encountered;
final merge belongs to Jack. A final observation found both `main` and `origin/main` at `e3de54a`
(the other session published or refreshed those refs); this atlas branch is therefore two local
commits ahead and three commits behind `origin/main` after this handoff commit. This session did
not publish those platform changes. Reinspect refs because the other session may continue advancing them.

## Results and validation

- Small identity contract: 35 explicitly allocated project IDs; 35 locked source excerpts across
  three layers; 43 unreviewed typed assertions, 71 exact/fallback name records, 35 independent points.
  QID anomalies, map units, code aliases, disputed types, capitals and microstates remain explicit.
- Selection: actual equal-angle/equal-area/equal-count candidate sets with unchanged capital and
  source-country rules. Equal-area alone is not a demonstrated general improvement. Equal-count
  distance supplements improve largest source-settlement gaps but require editorial review.
  All candidates preserve all 214 national-or-alternate source-capital records, including 200
  national-flag records, 225 source country codes and eight required microstate/island cases.
- Existing research evidence: 20 source hashes verified, 21 retained layer hashes/counts/sizes
  verified, 58 semantic cross-report checks passed. No entire exploratory rerun or new source fetch.
- New/retained research tests: 24 catalog + 9 selection + 8 audit + 9 concordance = **50 passed**.
  Run `python3 atlas/research/test_{catalog,selection,audit,concordance}.py -v` individually.
- Clean Linux x86-64 CMake/Ninja build at `/tmp/globe-contract-build`; **50 existing CTest tests
  passed** on base `52ccb33`. Forty passed in the sandbox; ten X11 tests could not open their
  virtual displays there, then all ten passed on an authorized unsandboxed private-Xvfb retry.
  The suite does not include Claude's later menu test; no claim is made about that newer main.
- Two catalog extractions are identical. Two selection reports and all 12 full JSONL/gzip outputs
  are byte-identical on the recorded Python/zlib environment. No new runtime conformance claim.
- `git diff --check`, Python compile checks and `sh tools/check-policy.sh` passed.

Exact environment, hashes, commands, scoped warnings, host builder RSS/time and repeat checks are
in `contract-verification.json`. Existing test/build logs remain `/tmp/globe-contract-*.log`.
Actual JSONL/gzip bytes and spherical distances are measurements. The 32-byte settlement record
and name scenario is an **estimate**, not runtime RAM. Python peak RSS measures build-host process
memory. G4 memory/timing, glyph coverage and geopolitical completeness remain unmeasured/unreviewed.

## Decisions and next work

Read `identity-selection-checkpoint.md` for the #53 acceptance/re-scoping proposal and cross-agent
request. No acceptance boxes, source corrections, sovereign inventory or policy were approved.
Read `catalog/README.md` for identity/provenance/time/review invariants. No source IDs are globally
deduplicated; no external record is silently repaired.

Jack owns the next decision: review the 35 fixture entities and exceptional source associations,
then authorize expansion to a reviewed political/territory/capital inventory. Keep source-label
points separate from proven interior/picking anchors; independent spherical picking and hydrographic
coherence remain separate follow-ups. Review actual settlement additions/removals before adopting
any selection rule. The existing edition recipes remain unchanged.

Dedicated request to Claude (owner of #36/#38/#61): confirm consumable encoding profiles,
missing-glyph signaling and alternate-anchor needs. Atlas provides identity/source refs, exact names,
language/field provenance, explicit loss/review flags, source role/ranking and representative points.
The platform measures the actual chosen string in installed fonts; no fonts or glyph packs ship.
Current branch for that request is `codex/atlas-contract`, implementation `2afaebe`.

Memory work reached a design checkpoint only: `memory-ledger-design.md` specifies fixed-storage
counters, correct simultaneous peaks, move-only ownership tickets, failure/overflow semantics,
allocation-site boundaries and diagnostics metadata. Smallest next #55 slice is the isolated
accounting primitive and ownership tests. Call-site migration and shared headers/CMake require
coordination; no #56 degradation, #57 eviction or #60 CI budget work was begun.
