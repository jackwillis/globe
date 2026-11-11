# Classic Mac v0.1 readiness audit — 2026-11-10

Base: `main` @ `a0e01f3be96f6556502d62d51320a0e9c1ccc75d` (also `origin/main`). Evidence is from code
reading, host tests and a Retro68 cross-build. **Nothing here was run on a Mac.** Hardware items are
listed for the owner; they are not claimed to work.

## CI state

The latest `Checks` run on main (`a0e01f3`) succeeded with all three jobs executed: host build with
clang, Retro68 compile probe (PowerPC and 68k), and host build, lint and tests. Earlier main runs:
`1c50778` and `3270724` failed the clang-format step and `13d27cc` the clang-tidy step (code
failures, fixed by `2c3d1c1`/`6215ecb`); `d421cad` is a `failure` with **zero jobs** (the workflow
never ran any code, so it says nothing about the code). Two PR runs were `cancelled` (superseded
pushes), not failed.
CI compiles the OS 9 app but cannot run it, so none of the findings below can be closed by CI.

## What exists on OS 9 (code evidence)

One window with globe pane and 220 px sidebar (owner confirmed on the G4, 2026-11-09, #79); looks
list; scrolling wrapped information card; menus (Apple, File, Edit, View) with Cmd keys and
core-driven check/dim state; About window; Preferences window (units, language); Copy Information
(Edit > Copy, TEXT scrap); progressive render, zoom preview and cursor policy. Preferences are not
persisted (#49).

## Findings

| # | Finding | Class | Issue |
|---|---|---|---|
| 1 | A failed window grow (no memory for the larger framebuffer + GWorld) **quit the application**. The old and new buffers coexist during a grow, so a large grow in a 24 MiB partition is plausible. | Release blocker (graceful allocation failure) | #10 — **fixed here** |
| 2 | A failed startup only beeped and exited with no explanation; the partly built application also kept its memory while exiting. | Release blocker (first-use failure, minimum-memory story) | #10 — **fixed here** |
| 3 | The window title is a performance readout (`Globe 12 bands, 991 ms, copy ... turn ... step ...`, plus `ev` codes) on every settle. Useful for G4 measurements, wrong for a release. Needs a build option (default off for release builds, on for proof builds), coordinated with Codex's benchmark instrumentation. | High-value polish | #11, #12 — **fixed in the follow-up PR** (opt-in `GLOBE_TIMING_TITLE`) |
| 4 | The About window says "Data sources and notices: see the ReadMe". The embedded landclass pack is **CC BY 4.0** (RESOLVE Ecoregions 2017) and the ETOPO and Natural Earth credits are only in the ReadMe/DataCredit files. The app should show the credits itself. Overlaps #93 (edits `l10n/en.txt`, `about_window.cpp`), done after #93 merged. | High-value polish; confirm before release that the ReadMe ships in every artifact | #11 — **fixed in the follow-up PR** |
| 5 | `os9/ReadMe.txt` is stale (says it has not run on OS 9; mentions S, Q/Escape, floppy-only text). #76 rewrites it. | Release blocker if #76 is not merged first | #76 |
| 6 | File menu has only Quit: no Close (Cmd-W) for the About and Preferences windows, which are non-modal and can only be closed with the close box. Keys typed with them frontmost still drive the globe, and a bare Q or Escape quit the program. | Polish | #46 — **fixed in the follow-up PR** (File > Close, Cmd-W; bare Q/Esc ignored on OS 9) |
| 7 | No measured partition. `SIZE` is 24 MiB preferred / 12 MiB minimum, never measured with full overlays, resize to the maximum (1600x1200), or Preferences/About open. | Release blocker, needs hardware | #10 |
| 8 | Cursor behaviour, menus, resize, idle and file packaging are unverified on bare OS 9. | Release blocker, needs hardware | #95, #44 |
| 9 | Preferences (language, units) reset every launch. | Later milestone (#49) | #49 |
| 10 | English-only for v0.1 (#93). Tradeoff: the Spanish table is ASCII-only (no accents, "sin conexion"), so it reads as unfinished, and every switch path doubles the surface to test on hardware; but removing it discards working, owner-visible features and a tested path, and the catalog stays either way. **Decision: #93 accepted and merged by the owner's delegation**, keep `l10n/en.txt` and `tr(Str)`, revisit with proper Mac Roman/Unicode in #84. This is the owner's decision; nothing here depends on it. | Decision needed | #93, #84 |
| 11 | Edit > Undo/Cut/Paste/Clear are always dim except for desk accessories. Correct on Classic Mac. | OK | — |
| 12 | `Cmd-+` needs Shift on US keyboards (works through MenuKey because the character is shifted); `Cmd-=` does not zoom. | Later | — |

## Change in this branch (findings 1 and 2)

- `src/platform/os9/main.cpp`: a failed grow beeps and keeps the old window size. `buffers()` creates
  both new buffers before cancelling work or disposing the old ones, so a failed grow leaves the
  application state untouched (checked by reading; no allocation-failure test exists).
- `main()`: the `Application` is scoped so its destructor runs before `StopAlert`; startup failure
  shows an alert (`'ALRT'`/`'DITL'` 128 in `os9/globe.r`) saying that memory is usually the cause and
  how to raise the partition. The alert is English: it appears before preferences exist.

Not changed: rendering core, benchmarks, atlas selection, strings catalog, #93's files other than
`main.cpp`.

## Verification actually run

- Host: `cmake -S . -B build -G Ninja`, `cmake --build build`, `ctest --test-dir build`: 59/59 pass
  (Linux x86-64, Fedora, serial).
- `git ls-files '*.cpp' '*.hpp' | xargs clang-format --dry-run --Werror` with clang-format 22.1.8: clean.
  clang-tidy is not run on `src/platform/os9` by CI and was not run on it here.
- Retro68 PowerPC: `cmake -S os9 ... retroppc.toolchain.cmake` and `cmake --build` in the
  `ghcr.io/autc04/retro68` image: compiles, links, Rez accepts the new resources, and the alert text
  is present in the built `GlobeFloppy.bin`. The tool image here is the same one CI and
  `tools/build-os9.sh` use.
- Not run: any execution on Mac OS 9 or Classic, any forced allocation failure.

## Hardware checklist (add to #95)

Boot bare OS 9.2.2, one build name, record RAM and the Preferred partition.

1. **Low memory start:** in Finder Get Info set Preferred and Minimum to about 4 MB (below the
   working set) and launch. Expect the alert, then a clean return to the Finder, and no crash.
   Restore 24 MB afterwards. (Never leave the partition lowered.)
2. **Grow failure:** with the Preferred size near the minimum, drag the grow box to the largest
   size. Expect either a larger window or a beep and the old size, never a quit.
3. **Partition measurement (#10):** with 24 MB, grow to maximum, open About and Preferences, select
   a country, turn on Night and Measure; use About This Computer to read the largest unused block
   and Globe's used memory. Repeat with 16 and 12 MB and record the lowest that survives.
4. **Cursor (#44):** open hand over the globe, closed hand while dragging, pointing hand over a
   clickable country, arrow in the sidebar, menu bar and over other windows; after switching to
   another application and back, after dragging the window.
5. **Menus (#46):** every View item and key equivalent; Look check marks follow clicking the sidebar
   list; Copy then paste into SimpleText; About, Preferences open and close.
6. **Window:** move, resize at both extremes, exposure after a window passes over it, scroll bar
   active and inactive states in the sidebar when the window is in the background.
7. **Idle:** leave it unfocused for several minutes: no CPU use in the Finder's process list, no
   memory growth.
8. **Title bar (finding 3):** note what the title shows after each settle.

## Memory model for #10 (estimate, corrected for #106)

Host ledger measurements and code reading put the 1° terrain cache at about 3.66 MB, with additional
atlas/scenes, embedded data, code and Toolbox state. Pixel storage covers the **globe pane**, not
the fixed 220-pixel sidebar. Eleven bytes/globe-pixel approximates four framebuffer+coverage
allocations (primary, overlay, zoom and line layer,8.5), GWorld (~2 plus row padding), and default
line-pixel list (~0.25). Small row/scratch/block-header costs need additional heuristic headroom.

For illustration only, using an estimated 4.7 MB fixed footprint and decimal MB:

| Content window | Globe pane | Pixels | 11 B/pixel | With estimated fixed footprint |
| --- | --- | ---: | ---: | ---: |
| 628x428 (640x480 screen) | 408x428 | 174624 | 1.92 MB | 6.62 MB |
| 1012x716 (1024x768 screen) | 792x716 | 567072 | 6.24 MB | 10.94 MB |
| 1600x1200 (maximum content) | 1380x1200 | 1656000 | 18.22 MB | 22.92 MB |

The old whole-window calculation unnecessarily shrank a 628x428 window to 480x327 when its 408x428
globe already fitted the 174624-pixel budget, reducing globe area to 85020 pixels. Portable tests
cover exact fit, one byte below, exhausted budgets, odd dimensions, minimums and maximums. The
maximum pane's steady-state estimate alone does **not** prove that it exceeds a 24 MiB partition,
nor establish that the partition is sufficient for all workloads.

FreeMem is a conservative startup hint, sampled after terrain/scenes but before window/control
allocations. The installed Retro68 allocator uses NewPtr, so it samples the relevant current
Classic heap. It reports total free bytes: fragmentation can prevent a large allocation; zone
growth or purgeable storage can make the snapshot conservative. MaxMem is not a drop-in total
budget: it moves/purges storage and reports the largest block. See [Inside Macintosh, Memory
Manager](https://dev.os9.ca/techpubs/mac/pdf/Memory/Memory_Manager.pdf), pp.2-66/2-75, and the
[Retro68 allocator](https://github.com/autc04/Retro68/blob/master/libretro/malloc.c).

Subtracting 1 MiB is **not a reserved block**. Later resizes may retain old and new framebuffer,
GWorld, zoom or overlay allocations simultaneously, exceeding the 11 B/pixel steady-state model.
Actual allocations remain fallible; the minimum-size fallback can still fail to launch. This
startup heuristic does not bound future peaks. Neither it nor host tests establishes the 12/24 MiB
SIZE claims: bare OS 9 #10/#95 partition, fragmentation and resize measurements remain required.
