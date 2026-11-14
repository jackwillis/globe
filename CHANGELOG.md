# Changelog

This file will track **published releases**, not every experimental commit.

## Unreleased

- Help for the controls (#40): H or ? (and Apple menu > Controls on Classic) shows the keys and gestures in the information pane on both front ends, from the one table in `globe/controls.hpp` that also defines the shortcut keys. Escape hides it. Host-tested and cross-compiled; not yet run on the G4.
- The Classic About window displays the full project version.
- The unfinished globe edge report (#74) was closed after the owner reported sustained use since the fix with no recurrence. The closure was based on regular use without a structured test.

## 0.1.0 - public preview (2026-11-13)

The first public release of **Poodle Earth** (repository `jackwillis/globe`), for Mac OS 9 on PowerPC.
See [release notes](docs/release-notes-0.1.md).

- Map and Relief looks, progressive software rendering, country and city picking with information
  cards, night side, measurement and units; one Classic window with a sidebar, native menus and
  an About window with data credits.
- Distribution: StuffIt archive only; exact-version source archive,
  manifest and checksums. Application, Finder name and creator code changed from the development
  `GlobeFloppy` / `Glb2` to **Poodle Earth** / `PdlE`.
- Known limits: occasional unfinished globe edge (#74), small embedded data set, no icon yet.
- Version is `0.1.0`; the historical prototype tag `v1` is not a 1.0 release.

## Development (after 0.1.0)

- Offline software globe with Map and Relief looks, progressive rendering and geographic overlays.
- Native Classic Macintosh implementation and digital-first packaging undergoing acceptance and release verification.
- Atlas production, reproducibility and resource-accounting work in progress.

Historical tag `v1` is a prototype tag, not a semantic version 1.0 release. See [issues](https://github.com/jackwillis/globe/issues) for current progress.
