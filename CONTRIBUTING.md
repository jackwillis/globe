# Contributing to Globe

Globe is a free-software, offline atlas for NetBSD/X11 and Classic Mac OS 9. Bug reports, reproducible test evidence, geographic corrections and scoped code contributions are welcome.

Start with [README](README.md), [architecture](docs/architecture.md), [documentation index](docs/README.md) and [current issues](https://github.com/jackwillis/globe/issues). Before major pack-format, identity or memory-policy changes, discuss the compatibility boundary in an issue.

## Build and test

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
tools/check-policy.sh
```

Platform specifics: [toolchains](docs/toolchain.md). Python/data workflow: [Python setup](docs/python-tooling.md) and [atlas production](atlas/README.md). Host tests and Retro68 compilation do not establish native OS 9 acceptance.

## Pull requests

Use a dedicated branch/worktree, preserve other contributors' uncommitted changes and keep PRs reviewable. State the motivating issue, source or test evidence, exact verification commands, environment, and limitations. Renderer changes need deterministic pixel/coverage tests; geographic changes need source hashes and editorial provenance; allocation changes need memory-budget evidence.

First-party code is GPL-3.0-or-later, with SPDX tags. Data have separate source licenses; see [data/SOURCES.md](data/SOURCES.md). Don't commit private Apple OS/ROM images, third-party commercial assets, credentials, local source caches, private host details or machine-specific build outputs.

Release publication, protected hardware operations and major architectural choices require owner approval. Dated handoffs record history, not standing permission.
