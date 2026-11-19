# Contributing to Poodle Earth

Poodle Earth is a free-software, offline atlas for NetBSD/X11 and Classic Mac OS 9. Bug reports, reproducible test evidence, geographic corrections and scoped code contributions are welcome.

Start with [README](README.md), [architecture](docs/architecture.md), [documentation index](docs/README.md) and [current issues](https://github.com/jackwillis/poodle-earth/issues). Before major pack-format, identity or memory-policy changes, discuss the compatibility boundary in an issue.

## Choose a small task

Start from an issue and check its latest comments and linked PRs for active work.
For agents, [AGENTS.md](AGENTS.md) and [Codex notes](docs/agents/codex.md) define
authority and issue maintenance. Keep onboarding changes separate from release
publication or hardware acceptance. The [user guide](docs/user-guide.md) is the
starting point for reproducing a user report.

## Build and test

Install a C++20 compiler, CMake, Ninja, Python 3 and X11 development libraries.
Xvfb enables the private-display UI tests. On a host checkout, run:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
tools/check-policy.sh
```

The build and all 68 registered tests passed on Fedora 44 with GCC 16.2.1
for this docs pass (the ten X11 tests needed a rerun with private-display
socket access outside the execution sandbox). Run `./build/poodle-earth` on an authorized
X display to explore the Unix frontend. Its toolbar/looks palette and keyboard
handling differ from Classic; a passing host suite is not a claim of GUI parity.

Classic builds use Retro68 via `tools/build-os9.sh --floppy` or
`tools/build-edition.sh floppy`; the historical `floppy` name identifies the
compact embedded-data build, not a requirement to install from a floppy.
Do not build on shared G4 hardware. Platform specifics: [toolchains](docs/toolchain.md). Python/data workflow: [Python setup](docs/python-tooling.md) and [atlas production](atlas/README.md). Host tests and Retro68 compilation do not establish native OS 9 acceptance.

## Pull requests

Use a dedicated branch/worktree, preserve other contributors' uncommitted changes and keep PRs reviewable. State the motivating issue, source or test evidence, exact verification commands, environment, and limitations. Renderer changes need deterministic pixel/coverage tests; geographic changes need source hashes and editorial provenance; allocation changes need memory-budget evidence.

First-party code is GPL-3.0-or-later, with SPDX tags. Data have separate source licenses; see [data/SOURCES.md](data/SOURCES.md) and
[licensing policy](docs/licensing-policy.md). New sources or licences need owner approval. Don't commit private Apple OS/ROM images, third-party commercial assets, credentials, local source caches, private host details or machine-specific build outputs.

Release publication, protected hardware operations and major architectural choices require owner approval. Dated handoffs record history, not standing permission.

For user-visible changes, include a concise release-note candidate in the PR.
[CHANGELOG.md](CHANGELOG.md) keeps curated history; version-specific release
notes explain a release; published GitHub tags/assets identify what shipped.
Maintainers finalize dates and artifact claims at publication.
