# Repository modernization and developer experience — v0.2 planning proposal

**Status:** Nonbinding, owner-requested planning note, 2026-11-13. **Not an accepted directory tree, release gate or refactor authorization.** Tracking issue: [#126](https://github.com/jackwillis/globe/issues/126). Related: [#118](https://github.com/jackwillis/globe/issues/118) (public documentation), [#121](https://github.com/jackwillis/globe/issues/121) (agent state), [#125](https://github.com/jackwillis/globe/issues/125) (changelog), [#114](https://github.com/jackwillis/globe/issues/114) (presentation boundary) and [#122](https://github.com/jackwillis/globe/issues/122) (Unix packaging).

## Why consider this now?

The portable C++20 core, two native frontend builds, a reproducible atlas production pipeline and historical performance records are valuable. They should remain understandable as the project adds source-based portability, new user interfaces and external data packs. The maintenance problem is primarily **finding the authoritative information and building the right target with little ceremony**, rather than an urgent need to eliminate top-level directories.

The existing tree already has reasonable boundaries:

| Current root | Present responsibility | Question to validate |
| --- | --- | --- |
| `src/`, `include/` | Core, renderer, pack readers, shared interfaces and X11/OS 9 platform code | Keep as conventional CMake layout or co-locate interfaces by subsystem? |
| `os9/` | Standalone Retro68 CMake entry point/resources | Can entry points be simplified without breaking Macintosh forks and resource builds? |
| `atlas/`, `data/` | Source-driven research/builder versus committed runtime data | Is builder/runtime provenance visible and reproducible without moving generated inputs? |
| `l10n/` | Localization assets | Retain an independent resource root or move under a consistent assets layout? |
| `tests/`, `tools/` | Portable/X11 test fixtures, scripting, toolchains and packaging | Can naming and entry-point documentation be made consistent first? |
| `docs/` | Architecture, specifications, public guides, experiments, performance evidence and dated notes | Which files are normative, exploratory, archived or now superseded? |

**Baseline evidence:** root [CMake configuration](../../CMakeLists.txt), [architecture](../architecture.md), [toolchain notes](../toolchain.md), [documentation index](../README.md), [agent contract](../../AGENTS.md), [Unix research](../research/cross-platform-frontends-and-distribution.md).

## Design goals and non-goals

- **Build is simple without GitHub Actions:** `cmake -S . -B build -G Ninja`, `cmake --build build`, `ctest --test-dir build --output-on-failure` remain the ordinary supported host path, with explicit dependency prerequisites. A platform-specific wrapper may simplify Retro68 but cannot hide what it does.
- **One shared geographic core, honest platform builds:** frontends own windows/input/font drawing/clocks/pixel submission; maintain the deterministic `Model / Msg / update / Effects` boundary. No forced unified widget framework or accepted ADR rewrite.
- **Predictable output and packaging:** distinct build trees; optional CMake presets/toolchain files for host, Classic, NetBSD, Unix/FLTK experiments and later Windows; install and stage only real supported targets. pkgsrc `.tgz` packages are a NetBSD packaging goal, not generic tarballs.
- **Navigation and history:** one short public entry point for users, one developer entry path, one index of current architecture, and explicitly marked research/old evidence. Preserve links, benchmarks, decisions and source provenance.
- **Small reviewable changes:** reduce needless path indirection and duplicated instructions without a cosmetic codebase rewrite.

Non-goals: rewriting the renderer, introducing an all-platform build system, shipping untested binaries, moving every folder for aesthetics, changing v0.2 feature scope, deleting historical measurements, or modifying the owner's G4.

## Candidate organizational strategies

### A. Conservative: documentation and build ergonomics first (preferred for first experiment)

Keep most existing roots. Reclassify and index `docs/`, with proposed groups `user/`, `developer/`, `architecture/` (format specs included), `design/`, `adr/`, `research/`, `perf/`, `history/`. Keep an index in `docs/README.md` and compatibility stubs or updated links wherever public URLs have circulated. Consolidate duplicate instructions only after verifying which facts are current.

Add a short source-build matrix and optional CMake presets / thin wrappers **only where they remove steps**. Keep build toolchains openly documented. This approach buys most discoverability improvement with little risk.

### B. Fewer top-level roots (only if inventory demonstrates a real benefit)

Evaluate whether `os9/` and `l10n/` belong under a more cohesive platform/resource subtree, and whether some scripts should be grouped by responsibility. Do **not** pre-approve moving `src/`, `include/`, `atlas/`, `data/`, `tests/` just to hit a directory count. The Retro68 CMake entry point, generated resource paths, data builder inputs and relative links are migration hazards. Produce an exact proposed tree, affected-path inventory and before/after build commands before this option is selected.

A dual-purpose `platforms/` abstraction or huge `common/` directory is not automatically an improvement; let real future Win32/FLTK adapters demonstrate whether shared code needs extraction.

## Concrete pre-change inventory

1. Enumerate every CMake target, test, build container/toolchain, optional dependency, generated file, release packaging script and data path. Categorize by platform/host and distinguish **compiled**, **native-tested** and **released**.
2. Enumerate docs by audience and authority: current normative instructions, Accepted ADR, Draft ADR, proposal, benchmark/result, dated history. Find contradictions with `main`, released tags, GitHub issue state and relevant native acceptance.
3. Find broken or stale relative links, obsolete hardcoded repository URLs, inaccurate version strings, duplicate state snapshots and private/machine-specific references. Keep a recorded decision for each proposed move/removal.
4. Check what a new contributor actually does: clean dependency setup, first build, tests, running the Unix frontend, Classic cross-build; record steps, failures and time/complexity without fabricating a test run.
5. Decide whether the target is fewer directories, less documentation ambiguity, fewer required dependencies or faster onboarding. Track those measures separately.

## Proposed small PR sequence (subject to owner review)

1. **Documentation information architecture:** classify and index existing docs; mark history; fix links and current-state claims while preserving dated research.
2. **Developer entry path:** one short host build/test guide plus a target/packaging matrix; trial CMake presets only if they simplify a reproducible command.
3. **Build target boundaries:** after a path-impact audit, consolidate narrow CMake or script duplication while retaining platform-specific toolchains; keep exact source archives and resource forks intact.
4. **Optional directory moves:** only after selecting a target layout, in small mechanical PRs with no concurrent renderer/frontend feature changes.
5. **Independent frontend proof:** a small FLTK Unix prototype may run beside the existing Xlib test/reference frontend; it does not require first completing #83's polish. Determine real parity, memory and usability before recommending replacement.

The existing [cross-platform research](../research/cross-platform-frontends-and-distribution.md) is exploratory; none of its possible ports are an automatic v0.2 deliverable.

## Verification and acceptance

- Before/after documented host build and CTest; compiler/lint/policy checks as applicable; compare registered test counts, deterministic pixel/trace hashes and data-manifest checks. **Run**, don't infer, when an implementation PR happens.
- Keep OS 9 Retro68 compile/resource packaging and fork-preservation test available. Only separately approved genuine hardware sessions can establish OS 9 acceptance.
- Verify Markdown links, `#include` paths, generated-file inputs, CMake references, script invocation paths, installation and packaging outputs. Treat moved public docs as a compatibility issue.
- Record the source revision, exact commands, environment, observed results, omissions and unchanged platform/release guarantees in each PR. CI passing does not prove untested OS support.
- The first phase is accepted only if contributors can find information and build more easily **without regressing existing tests** or introducing mandatory tools.

## Owner decisions before implementation

- Approve a final proposed directory tree and the first narrow migration slice; preserving existing roots is a valid outcome.
- Decide if/when the Unix UI should switch from Xlib to FLTK after a measured small prototype; never remove Xlib and its tests merely because it looks dated.
- Choose which new platform/package targets to maintain after actual test evidence, particularly pkgsrc NetBSD variants and Windows XP.
- Decide any release-scope changes separately. This note itself does not authorize them.
