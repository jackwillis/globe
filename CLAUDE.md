# Claude handoff — Globe

**Refreshed 2026-11-07.** This is the concise project entry point for Claude Code / Claude sessions. For Codex-specific instructions, see [docs/handoff.md](docs/handoff.md). The previous long running handoff was archived in [docs/history/handoff-through-2026-11-07.md](docs/history/handoff-through-2026-11-07.md).

## First: orient yourself, don't assume old status

1. Inspect `git status --short --branch`, `git log -8 --oneline`, and any active worktrees **before touching files**. The GitHub default branch and the owner's local `main` may diverge. Never assume the remote contains the owner's unpublished work or that an old scratch executable is current.
2. Read [the issue tracker](https://github.com/jackwillis/globe/issues) (canonical tasks), [roadmap](docs/backlog.md), [ADR 0002](docs/adr/0002-rendering-shared-code-and-usability.md), and [architecture](docs/architecture.md). The earlier GL backlog in `docs/history/` is archival, not a second queue.
3. Read the **specific issue**, its comments and acceptance criteria before starting. Reconcile with actual code; report conflicts or stale status instead of guessing. Don't opportunistically complete adjacent issues.
4. Coordinate with Codex or other sessions when shared worktrees, build directories, Tiger Classic, or bare OS 9 may be in use. Never overwrite or stop another agent's active process or build.

## Current product direction

Globe is an **offline-first, software-rendered interactive Earth** for NetBSD/macppc X11 and classic non-Carbon Mac OS 9 on a Power Mac G4. It has Map and Relief looks, place/country picking, measurement and day/night overlays, country highlighting, 1°/0.5° progressive elevation on full X11, and the compact Classic Mac application. Navigation should remain responsive; rendering is interruptible and progressive.

**The owner's latest direction is usability:** make the globe an atlas, not merely a fast rendering demonstration. Epic [#35](https://github.com/jackwillis/globe/issues/35) organizes the work. Begin with *specific authorized issues*, especially the Classic Mac cursor [#44](https://github.com/jackwillis/globe/issues/44), looks palette [#45](https://github.com/jackwillis/globe/issues/45) and menu bar [#46](https://github.com/jackwillis/globe/issues/46); then render-tier evaluation [#34](https://github.com/jackwillis/globe/issues/34), help overlay [#40](https://github.com/jackwillis/globe/issues/40), on-map text [#36](https://github.com/jackwillis/globe/issues/36), pointer readout [#37](https://github.com/jackwillis/globe/issues/37), labels [#38](https://github.com/jackwillis/globe/issues/38) and associated atlas controls. These are **priorities to discuss/implement per issue, not blanket authorization to implement all**.

**Release strategy changed 2026-11-07:** digital-first v0.1 Classic Atlas, with a tested StuffIt (.sit) download if feasible, plus MacBinary and HFS image fallbacks; packaging [#47](https://github.com/jackwillis/globe/issues/47), release [#12](https://github.com/jackwillis/globe/issues/12). The 1.44 MB physical floppy edition is **paused as a release target**, although its build/size regression checks remain useful. A later *larger* Classic edition may load external packs [#48](https://github.com/jackwillis/globe/issues/48), run from read-only volumes with Preferences elsewhere [#49](https://github.com/jackwillis/globe/issues/49), include richer data [#50](https://github.com/jackwillis/globe/issues/50) and use a proper disk-image layout [#51](https://github.com/jackwillis/globe/issues/51). Do **not** silently restore the old floppy-first release plan or implement HTTPS on the initial Classic edition.

## Architectural invariants

- Keep the **Elm-inspired Model / Msg / update / Effects** core. Clock and OS events enter as messages, not hidden core calls. Small Model state is mutated through `update()`; don't force a fully immutable-model rewrite.
- Published geographic data, resource identities and per-job camera/render plans should be stable/immutable. Framebuffers, line/terrain workspaces, pixel lists and cooperative job cursors are mutable. No frame-loop allocation, unbounded operation or universal scheduler without evidence.
- Portable C++20 core: no exceptions, RTTI, platform headers, networking or direct clock reads. Preserve strict bounded pack readers, big-endian portability and appropriate copyright/SPDX headers.
- The shared platform-neutral seams already exist: `globe/zoom.hpp` (`ZoomPreview`), `globe/shown_frame.hpp` (`ShownFrame`), `globe/refine_runner.hpp` (`RefineRunner`), shared `binding_for_char` and `same_orientation`. X11 and OS 9 retain their own event loops, wake timers, window/cursor APIs and pixel submission. **ADR 0002 explicitly rejects extracting a generic coordinator just to remove a handful of remaining lines**; revisit #16 only on demonstrated need.
- Render decision in ADR 0002: **lines first**, composite using a line-pixel list; terrain refines circle-outward and the final limb-only repair handles clipped cells. Zoom uses a scaled previous-frame preview, skips redundant coarse pass after a pure zoom, and uses a minimal opaque 10-pixel progress bar. Do **not** resurrect the old paced-after-terrain line reveal or global canonical repaint without discussing the changed visual output and G4 measurements.
- Source/format provenance matters. Never introduce unreviewed geographic sources, third-party code, arbitrary provider hosts, or license assumptions. No developer-operated data server or relay.

## Hardware and verification: honest boundaries

- **Bare OS 9 was exercised on the physical G4 on 2026-11-07**: Map ~991 ms settle, ~150 ms in copying, ~20 ms maximum event-loop turn, ~9 ms maximum render step in the recorded run. Owner reports zoom preview, progress bar and shortcuts working. This is not full acceptance: cursor behavior, menus, palette, resizing, idle, file loading and memory ceiling still require targeted testing. See [OS 9 proof](docs/os9-proof.md).
- Earlier Tiger Classic tests are valuable but *not* identical to bare OS 9; headless NetBSD G4 tests are not GUI presentation tests. Keep platform and version attached to every benchmark claim. Record regression views and raw timing in `docs/perf/`.
- Host basics: `cmake -S . -B build -G Ninja`, `cmake --build build`, `ctest --test-dir build --output-on-failure`. Classic builds/portability: inspect `tools/build-edition.sh`, `tools/build-os9.sh`, `tools/probe-retro68.sh` and [toolchain notes](docs/toolchain.md) before using them. Run format/policy checks appropriate to changed code and note which were actually executed. Some older docs contain obsolete test counts.
- `x11_toolbar` can time out under sanitizers when a fixed wait races a busy machine; don't mistake an uninvestigated intermittent test for a proven product failure or silently exclude it.
- For render changes, verify cancellation, stable final pixels (unless a specifically approved visual change), multiple continuation budgets, memory limits and real G4 responsiveness. The changed limb-pass behavior can intentionally differ at a handful of horizon pixels; use current baselines, not old pins.

## Safety / change boundaries

- **Never push, force push, tag, publish releases, merge remote branches, deploy, or alter owner machines merely because an issue exists.** Request explicit approval for each consequential action. Preserve uncommitted work and existing worktrees. Local narrow commits are okay only within the owner's session instructions. Never modify a shared branch/worktree blindly.
- **Maul is shared hardware**, across NetBSD, Tiger Classic and bare OS 9. Do not compile on it, change display access permissions, overwrite owner's binaries, erase disks, kill Classic/other sessions, or mount/change volumes without authorization. Cross-compile on the development host; if the owner authorizes a hardware test, use fresh distinctive artifact names, coordinate the active OS/session and record exactly what ran.
- Do not commit binary artifacts, source archives, mounted image dumps, private credentials or TLS seed material; generated releases belong in ignored build output until the owner publishes.
- Do not silently write preferences or cache files on the user's machine; read-only/Preferences handling is tracked in #49. Respect TLS trust and entropy policy in [fetch design](docs/fetch-design.md) if/when networking is explicitly in scope.

## How to hand back work

Report: issue(s) addressed, files changed, tests with exact platform and commands, image/pixel or performance deltas, unresolved risk, and the next *specific* issue or decision. Update the relevant issue and canonical documentation when authorized; do not append session diaries to this file. Historical development notes remain in `docs/history/`.

**Single source of truth for what is next:** live [GitHub Issues](https://github.com/jackwillis/globe/issues) + the owner's current assignment, not a stale handoff queue.
