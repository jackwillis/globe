# Repository and CMake modernization audit — November 19, 2026

**Status:** Archival summary of **Claude's read-only host audit** against \`origin/main\` at \`bd3df23\`, with *subsequent changes explicitly separated*. Original Claude memo title: "Poodle Earth repository and CMake audit (read-only)", dated 2026-11-19. Research archive: [#213](https://github.com/jackwillis/poodle-earth/issues/213). Existing scope owner: [#126](https://github.com/jackwillis/poodle-earth/issues/126). **Neither this summary nor the audit authorizes broader refactoring.**

The exact locally supplied memo was 247/248 lines in the conversation and had observed SHA-256:

    bc52882f6a956919491eb4bcba6f5524fa24fc012397e3024e95127eeedf82a4

Its full raw text remains an owner-held conversation attachment; this document is a selective durable synthesis, **not a re-run** of the original audit.

## Snapshot observations (historical, not current main claims)

Claude built in a scratch space **outside the repository** on Fedora 44, but did **not** run the full CTest suite for the initial audit. It reported:

- Host Release configure+Ninja build approximately **15 seconds** in its observed environment; a **~710 KB** \`globe-x11\` Release executable with embedded atlas/elevation/countries/places and no separate required runtime data.
- **68 registered tests**, including 10 X11-specific tests. Listing them with \`ctest -N\` is not passing them; the later #209 PR had separate 68/68 passing evidence.
- No root \`install()\` rule **at the historical snapshot**: \`cmake --install\` staged nothing. This was the most concrete native packaging blocker.
- Python/X11 optional discovery could allow a configure/build to pass while silently omitting the actual GUI target.
- Root versus \`os9/CMakeLists.txt\`: duplicated portable source enumeration and generated-resource command definitions. OS 9 intentionally omitted \`src/core/trace.cpp\`; any consolidation must preserve that difference and compiler behavior.
- Bench/profile targets compiled by default, including one codec bench outside \`GLOBE_BUILD_TESTS\`.
- No CTest labels: selecting X11 often used name regexes. An optional \`CMakePresets.json\` did not exist.
- Source generation via \`tools/embed_atlas.py\` is deterministic but expands binary data into sizeable C++ text. No demonstrated reason to sacrifice cross-toolchain portability for alternatives such as \`.incbin\`.
- \`enable_testing()\` placement and \`CMAKE_SOURCE_DIR\` assumptions were lower-priority maintainability notes, not established blockers for normal builds.

**Audit limits:** No NetBSD/macppc, G4, Retro68, or FLTK build; initial audit listed but did not run CTest. It did not diff all active PRs. Repo/CI structure and counts were time-specific.

## Architectural inventory and preserved boundaries

| Area | Role |
| --- | --- |
| \`include/globe\`, \`src/core\`, \`src/data\`, \`src/render\` | Portable C++20 geographic engine and strict data readers |
| \`src/platform/common\` | Shared CLI/pixel/clock seams without native GUI policy |
| \`src/platform/x11\`, \`src/platform/os9\`, \`src/platform/bench\` | Native adapters and independent performance tools |
| \`os9/\` | Separate Retro68 CMake/toolchain/application resources |
| \`atlas/\`, \`data/\`, \`tools/\` | Source provenance/builders, committed runtime geography, tooling |
| \`tests/\`, \`docs/\`, \`site/\`, \`l10n/\` | Verification, current+historical evidence, public site, strings |

The existing portable/platform boundary was already clear and checked in \`tools/check-policy.sh\`. Sharing source files between host and OS 9 via separate CMake entry points is **correct**, not a conceptual duplicate engine.

## Directory strategy comparison

**Option A (recommended for first concrete changes):** Retain current roots, add only narrowly useful \`cmake/\` helpers or a \`packaging/\` directory as real implementations justify them, and improve docs/index/build diagnostics incrementally. No file moves required.

**Option B (not recommended without new evidence):** Move \`os9/\`, \`l10n/\` or many \`tools/\` scripts; an estimated 80–130 renames would touch exact paths in CI, Classic packaging, AGENTS, manuals, agent worktrees and historical references, while producing **no demonstrated Unix package benefit**.

The audit's core lesson: **installability and explicit dependencies matter more than making the repository tree cosmetically smaller.**

## Ordered candidate improvement slices — status and hazards

| Finding / proposed slice | Why it matters | Status at archive |
| --- | --- | --- |
| Add \`GNUInstallDirs\` + install X11 executable and notices | Required installation tree for native packagers | **Implemented and merged in [#209](https://github.com/jackwillis/poodle-earth/pull/209)** at \`cfa07a83\`; 68/68 host tests, Xvfb and three CI jobs reported |
| Public \`poodle-earth\` command and docdir | Stable package-facing name, retaining internal \`globe-x11\` target | **Draft [#211](https://github.com/jackwillis/poodle-earth/pull/211)**; host/clang/Retro68 CI green in this conversation, but custom staged-install evidence was still requested before merge |
| Explicit X11/Python failure and optional benchmarks | No false-green frontend-less build; faster package build | **Proposal**, coordinate with draft FLTK PR #184; do not force X11 into independent Classic toolchain |
| CTest labels for \`core\`, \`x11\`, \`builder\`, \`bench\`, \`policy\` | Reliable subsystem test selection | **Proposal**, independent small PR possible |
| Shared core source list and embed helper | Reduce host/Classic drift | **Proposal**, coordinate with external atlas startup PR #181 and verify Retro68 compile and unchanged Classic output |
| Optional CMake presets | Reduce dev/target setup overhead if actually needed | **Low-priority proposal**; CMake preset schema version must respect consumers' CMake floors |
| GCC 12 portability CI | Guard NetBSD compiler floor and C++20 portability | Existing issue [#102](https://github.com/jackwillis/poodle-earth/issues/102), separately owned |
| Large root CMake split, object/interface libraries, new embed format | Speculative refactoring with extra Classic risks | **Explicit defer**; no evidence of net benefit |
| pkgsrc and Linux native packages | Actual distribution/acceptance outcome | Separate [#122](https://github.com/jackwillis/poodle-earth/issues/122) and candidate [#212](https://github.com/jackwillis/poodle-earth/issues/212) release planning |

An installation rule is **not** a pkgsrc, Flatpak, AppImage, Debian or RPM release; authentic target installation, actual runtime behavior, ABI/library floor, package metadata and licensing still require acceptance.

## Proposed follow-up for #126, not an assignment

A future owner-authorized audit follow-up could verify remaining live CMake hazards against **current main** and compare one small explicit-dependency/test-label patch. Preserve the full Retro68 build path, no hardware access, and don't expand this historical record into a new epic.

**No changes authorized here:** No directory reshuffle, release-milestone edit, source/build refactor, automatic merges or G4 actions. This page preserves *why* #209 was prioritized, and keeps the proposed improvements discoverable without treating the original time-specific audit as current code truth.