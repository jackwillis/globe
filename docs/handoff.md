# Codex handoff — Globe

**Current as of 2026-11-07.** This is the **active Codex operating guide**, not a running transcript. The former long handoff is preserved verbatim in [history/handoff-through-2026-11-07.md](history/handoff-through-2026-11-07.md); earlier archives also remain. [../CLAUDE.md](../CLAUDE.md) gives Claude the same project-wide rules. No issue is authorized solely by being listed below.

## 0. Before any action

1. Inspect `git status --short --branch`, `git log -8 --oneline`, and active worktrees. **Do not assume remote `main` and the owner's local working tree agree.** If code or files differ from this handoff, trust the inspected code and ask/record the discrepancy.
2. Read the assigned [GitHub issue](https://github.com/jackwillis/globe/issues) including latest comments, labels and acceptance criteria. [Roadmap](backlog.md) is navigation, not the work queue; the historical GL backlog in `history/` is archive only. Never auto-pick the next task from a stale note.
3. Read [ADR 0002](adr/0002-rendering-shared-code-and-usability.md), [ADR 0001](adr/0001-m2-to-m5-decisions.md), and the relevant implementation note (`refinement-order.md`, `os9-scheduling.md`, `os9-proof.md`, `progressive-resolution-plan.md`, or `fetch-design.md`).
4. Set a narrow scope. Look for other Claude/Codex worktrees and any owner changes before editing. Coordinate shared Tiger/bare-OS-9 tests; do not interrupt or overwrite another session.

## Product state and next direction

**Globe** renders an offline Earth with portable C++20 software rasterization, an Elm-inspired message/effect core, X11 on NetBSD/macppc and native non-Carbon Mac OS 9 through Retro68. Map and Relief styles are implemented; country/city picking, overlays, zoom preview, progress bar, 1° coarse and optional 0.5° fine elevation are present (the latter bundled in full X11). The work is moving from graphics proof-of-concept to **usable atlas**.

**Read the issue first.** Owner priorities recorded 2026-11-07, not blanket work authorization:
- Usability epic [#35](https://github.com/jackwillis/globe/issues/35): native Mac cursor [#44](https://github.com/jackwillis/globe/issues/44), looks palette [#45](https://github.com/jackwillis/globe/issues/45), menu bar [#46](https://github.com/jackwillis/globe/issues/46); then UX/help [#40](https://github.com/jackwillis/globe/issues/40) and atlas text/labels/readouts [#36](https://github.com/jackwillis/globe/issues/36)–[#39](https://github.com/jackwillis/globe/issues/39).
- Performance-tier evaluation [#34](https://github.com/jackwillis/globe/issues/34) is **research with measurements and a decision**, not permission to implement three complete renderers or silently persist calibration.
- Architecture [#16](https://github.com/jackwillis/globe/issues/16): shared runner/shown frame/zoom and keys are already implemented. The owner/ADR decided **not** to add a generic coordinator/host-interface solely to remove tiny duplicated `apply()` and picking paths; do not reopen without need.
- CI and release: inspect live issue status, especially [#7](https://github.com/jackwillis/globe/issues/7) (previously closed), OS 9 acceptance [#8](https://github.com/jackwillis/globe/issues/8), G4 performance [#9](https://github.com/jackwillis/globe/issues/9), memory [#10](https://github.com/jackwillis/globe/issues/10), packaging [#47](https://github.com/jackwillis/globe/issues/47) and release [#12](https://github.com/jackwillis/globe/issues/12).
- **Digital-first release decision:** v0.1 is now the Classic Atlas, favoring a tested StuffIt archive with MacBinary/HFS alternatives. A physical 1.44 MB floppy is **paused as a release target** but continues as a size/build check. Later external-pack/full Classic edition [#48](https://github.com/jackwillis/globe/issues/48)–[#51](https://github.com/jackwillis/globe/issues/51) is separately scoped. No v0.1 networking.

## Current architecture: preserve before expanding

- **Portable application state:** `include/globe/app.hpp`, `src/core/app.cpp` use `Model`, typed `Msg`, deterministic `update(Model&, Msg)`, and declarative `Effects`. Clock values enter via messages; input/camera generations and refine level completion govern cancellation. A small mutable model behind one update boundary is intentional; don't force pure immutable copies or a new state-management framework.
- **Stable published geography, mutable workspaces:** atlas/pack source bytes must outlive borrowed views. Per-job camera/quality/plan input is stable; terrain projections, line pixels, framebuffer, coverage bitmap, and refinement cursors are mutable and bounded. One job per platform. Resource replacement needs cancellation/quiescence and peak-memory accounting; see [#26](https://github.com/jackwillis/globe/issues/26).
- **Rendering:** circle-outward terrain reveal; **line layer first**, composed above terrain via a pixel list with fallback; no after-terrain timed sweep. The last terrain repair handles clipped limb cells only; some horizon pixels intentionally differ from earlier baselines. Scaled zoom preview from the last rendered view, skip redundant coarse level after pure zoom, and a minimal opaque progress bar. Read [ADR 0002](adr/0002-rendering-shared-code-and-usability.md) and [refinement notes](refinement-order.md) before changing any order, drawing semantics, pacing, or pins.
- **Shared frontend code:** `globe/zoom.hpp` (`ZoomPreview`), `globe/shown_frame.hpp` (`ShownFrame`), `globe/refine_runner.hpp` (`RefineRunner`), `binding_for_char`, and `same_orientation`. Keep Xlib/Toolbox windowing, clocks, wake/event loops, cursor APIs, `XPutImage` versus GWorld/`CopyBits` and platform presentation throttles separate.
- **Targets:** portable core has no OS headers, direct clock or network I/O, exceptions/RTTI, uncontrolled allocation or unbounded synchronous rendering. Preserve endian-checked strict parsers and explicit code/data licenses; any new first-party source needs the repository's GPL-3.0-or-later SPDX and copyright header.

## Testing and quality gates

Start with the smallest relevant tests, then verify the portable build and cross-target boundary:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Check current scripts/flags rather than assuming the archived environment:
- `tools/probe-retro68.sh` checks the portable source subset for PowerPC and 68k compatibility; **68k compile does not mean 68k app/runtime support**.
- `tools/build-edition.sh` and `tools/build-os9.sh` build/package Classic targets; only run with a suitable configured Retro68 environment and relevant task. Check binary forks, size/report and format/policy checks after source additions.
- Code formatting/static analysis: use repository-supported clang-format/clang-tidy workflows, noting host lint does not cover Toolbox-only code. Watch sanitizer timing sensitivity in `x11_toolbar` (fixed 700-ms wait can fail under load); reproduce before labeling a regression.
- Rendering refactors: maintain deterministic trace and final-frame tests where output is unchanged. When output intentionally changes (e.g., limb clipping), update pins only with a documented decision, independent/oracle tests and a recorded G4 cost. Test job cancellation, stale/duplicate completions, multiple continuation budgets, viewport resize and foreground/background transitions. **No promises of wall-time bounds from counting work units alone.**

**Hardware evidence:** bare OS 9 on the real G4 (2026-11-07) demonstrated Map at ~991 ms to settle, ~150 ms CopyBits total, ~20 ms max turn and ~9 ms max step, with zoom preview, progress bar and shortcuts owner-observed. This **does not** establish full native acceptance: cursor/menus/palette, resize/expose/idle and memory floor still need checks. Tiger Classic previously ~2.75 s and ~0.73 s copy for a particular build. These numbers are distinct workloads/environments; never collapse them into a single benchmark. See [proof](os9-proof.md), [scheduling](os9-scheduling.md), [G4 timings](perf/g4-2026-11-06.txt).

## Maul and publication — strong safeguards

- Maul is the owner's **shared** physical G4, with NetBSD 11, Tiger Classic and native OS 9. **Never compile on maul**, change display permissions, alter network/firmware, erase/mount/repartition media, delete owner's tip binaries, or start/quit another session's Classic without explicit owner approval. Cross-build on the host; transfer authorized tests with **unique, descriptive artifact names**, track provenance, and avoid overwriting anything.
- No push, forced update, tag, GitHub release, remote merge, or deployment unless **explicitly authorized for that operation**. Narrow local commits are appropriate only as allowed by the current assignment. Never rewrite or discard uncommitted user changes; do not edit an active other-agent worktree.
- Do not store private TLS entropy seeds, secrets, system screenshots or generated binary/disk archives in Git. Local files under `/tmp` or an ignored build directory are evidence, not release artifacts to commit.
- A proposed Preference write, cache install or external pack import is a user-machine change, not implicit permission. For eventual acquisition, preserve validated HTTPS chain, hostname, time and provisioned entropy; no insecure fallback or hosted relay. Read [fetch design](fetch-design.md) first.

## Finish a scoped assignment

In the final response and relevant issue (if authorized), include:
1. Issue number, change summary and exact commits/paths.
2. Tests actually run (commands, target OS/CPU, pass/fail and known noncoverage).
3. Rendering output, bundle size, memory and performance impact when relevant.
4. Remaining limitations, decisions needing the owner, and smallest next step.

Do not append an indefinite new diary to this document. Put a specific architectural decision into an ADR, measured runs into `docs/perf/`, hardware observations into `docs/os9-proof.md`, issue-specific progress into that GitHub issue, and noteworthy old narrative into `docs/history/`.

**Canonical task queue:** [GitHub Issues](https://github.com/jackwillis/globe/issues), not older GL checklists or this handoff's examples.
