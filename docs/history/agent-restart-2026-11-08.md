# Agent restart and worktree reconciliation — 2026-11-08

**Status:** Coordination handoff, not a release, merge approval, permission to use hardware, or declaration that local worktrees are clean. GitHub Issues remain the authoritative queue. This note records the observed remote state and the procedure for new Claude Code and Codex sessions; **reinspect the live repository before acting.**

## What landed / what did not

- **Atlas-production research:** `atlas/` now has source locks, edition recipes, semantic QA, naming and geometry investigations, and `atlas/reports/recommendation.md` plus `atlas/reports/semantic-followup.md`. Reported host verification includes reproducible research builds; no production pack migration or G4 memory measurements. #53 remains open pending explicit acceptance/re-scoping; #52 is the cross-release epic. An optional 0.25° GELV research file needs ~57 MB of terrain-cache memory and fails the current 32 MiB guard: **do not install it as a production pack**.
- **Classic Mac UX:** native cursor policy and Map/Relief palette code and hardware checklists were committed; a menu-bar design note exists. #44 and #45 are not automatically hardware-accepted; #46 remains a distinct implementation decision. A clang-format 22.1.8 repair was committed; **verify the latest actual workflow** before asserting CI is green.
- **Typography:** `docs/design/typography.md` and #36/#38/#61 reflect the owner's Helvetica/Palatino **installed fonts, no bundled fonts** decision. libschrift/FreeType/bitmap packs are fallback research. Native font-measure and draw adapters may differ between OS 9 and X11; deterministic placement tests use synthetic metrics.
- **Memory:** #54 coordinates #55 ledger, #59 diagnostics, #56 budget/degradation, #58 Classic Memory Manager, #57 pack eviction, and #60 CI budget. Measure first; do not conflate compressed size, resident bytes, peak allocation and target partition headroom.
- **Release:** digital-first v0.1 (StuffIt if verified; MacBinary and HFS image fallback), physical floppy release paused but compact-build regressions retained. #47 and #12 own packaging/release. No release without owner approval.

## First task for *each new agent session*: recover your local work

Before checkout/pull/merge/staging:
1. Identify machine and checkout. Run `pwd`, `git rev-parse --show-toplevel`, `git remote -v`, `git status --short --branch`, `git worktree list --porcelain`, `git branch -avv`, and `git log --all --decorate --oneline -30`. Fetch remote refs **after** noting local state, without altering working files.
2. Enumerate staged, unstaged and untracked files for *every accessible* worktree. Review `git diff`, `git diff --cached`, untracked paths, and ignored outputs carefully. Identify ownership and whether another session is using the tree. Do not run `git add -A`, `reset --hard`, `clean`, `stash --all`, force-push, or delete branches/worktrees as a shortcut.
3. Record any unique commits not reachable from local or remote main (`git log main..branch`, `git log origin/main..branch` as appropriate), distinguishing already merged, cherry-picked, rebased, experimental and unfinished work. An old remote `codex/issue-7-ci` branch previously contained a unique framebuffer change and a CI note; review before removal. `claude/typography-design` may be fully integrated already; verify, don't assume.
4. Preserve uncertain WIP with its original owner, preferably on a named recovery branch/commit or an explicitly scoped patch *after inspecting the content*. Exclude caches, downloaded source archives, credentials, binaries and generated outputs unless specifically required and reviewed. **Never stage or publish secrets.**
5. Reconcile work that is complete and tested separately from partial experiments. Owner decides merges to main, pushes, releases and destructive cleanup. Document local-only state in your end-of-checkpoint report because GitHub cannot see unstaged changes.
6. Compare actual code, current issues, issue comments and ADRs before planning new code. Do not treat a listed issue as permission to work on the entire backlog.

## Coordination between agents

- **Codex leads:** atlas-production/catalog/source QA, identity and names, host-side data reproducibility, and later separately authorized portable resource/measurement tasks.
- **Claude Code leads:** Classic Macintosh Toolbox/native UX, platform-specific fonts, Tiger/Classic testing and release packaging; later separately authorized target memory diagnostics.
- **Shared boundaries:** versioned typed geographic pack contracts, stable IDs and name/display profiles, portable label-measure contract, resource lifetime, memory ledger schema. Discuss shared header/CMake/roadmap edits before writing; prefer isolated branches/worktrees.
- Communicate durable cross-agent questions through a short section in the relevant GitHub issue or a handoff note with: **request / files or API boundary / evidence / decision needed / owner / current commit**. Read the other's published findings, but do not edit each other's WIP. Avoid simultaneous tests against shared disks/VMs.
- Work in checkpoints: recover; verify current baseline/CI; implement one bounded slice; test; commit locally; report changed files, exact commands, limits, next dependency. Do not silently expand the backlog.

## Suggested next priorities — subject to current issues and owner review

**Codex:** Finish acceptance/re-scoping report on #53; next a **small, reviewed canonical identity/name/concordance and place-selection contract** with explicit human-review exceptions and tests (under #52, not automatic production migration). Once independently approved, tackle #55 + #59 host accounting on a separate runtime workstream; #56 follows measurement. A tiny typed-pack feasibility prototype may come later after contracts and budgets.

**Claude Code:** Verify current CI status and recover UX changes; test/finish #44 and #45; #46 menu bar is the next user-visible task, along with related About/credits #11 if narrowly shared. Then targeted #47 digital-first packaging and #8 native OS 9 acceptance as authorized. #58 Memory Manager research may inform #55 without prematurely adding a memory subsystem. Keep #36/#61 typography as a separate follow-up, not a full font-renderer rewrite.

## Hardware permissions and limits

Claude is the **designated** platform-integration agent and may have SSH access to a Tiger computer capable of Classic testing. Such access is not carte blanche. Before using it, verify the host identity, current logged-in sessions, permitted paths, application-launch method, and exact user-approved scope. Ordinary non-destructive build/transfer/test work is different from rebooting, firmware/configuration changes, uninstalling software, deleting or overwriting user data, modifying partitions, or accessing the separate bare OS 9 G4. Require explicit approval for destructive/persistent changes or machine use beyond the owner's stated authorization. Don't claim bare OS 9 acceptance from a Tiger Classic run.

## Release/integration gates

- Keep production atlas files and decoders unchanged until the typed contract, QA fixtures, load/fallback semantics, licensing and measured memory budget have been reviewed.
- Platform-specific glyph pixels need not match; geographic identity/content and deterministic placement for fixed metrics should.
- Preserve Elm-style Model/Msg/update/Effects, bounded cooperative rendering, offline baseline, tested build paths, renderer goldens and protected target machines.
- If a required resource, Git branch or previous worktree is missing, report it; **never invent prior local-state facts**.
