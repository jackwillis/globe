# Changelog

Curated user-visible changes, most recent first. Version-specific release notes
provide context; published [GitHub releases and tags](https://github.com/jackwillis/globe/releases)
identify the authoritative shipped artifacts. Contributors propose noteworthy
changes in PRs; maintainers finalize version sections, dates and artifact details
at publication. Changes after the latest published version stay under Unreleased
until a candidate is identified.

## Unreleased

- Classic: opt-in File → Run Script executes bounded border queries, setters, assertions
  and presentation waits cooperatively, with optional user-selected JSONL output.
  Host tests and Retro68 compilation cover the prototype; native acceptance is pending
  ([#221](https://github.com/jackwillis/poodle-earth/issues/221)).

## 0.1.1 — 2026-11-19

[Download v0.1.1](https://github.com/jackwillis/globe/releases/tag/v0.1.1) · [Release notes](docs/release-notes-0.1.1.md)

- Classic: a memory-refused window enlargement now explains how to raise the
  application's memory in Get Info, once per session. Jack checked the alert on
  the G4 under bare Mac OS 9.2.2 at a 12 MB partition: one alert and beep,
  no repeat in that session, and a new alert after relaunch
  ([#116](https://github.com/jackwillis/globe/issues/116), [#189](https://github.com/jackwillis/globe/pull/189)).
- Classic: QuickDraw hand cursors replace the AppearanceLib dependency that
  prevented Mac OS 8.1 startup. Jack reported PowerPC emulator smoke tests on
  System 7.1.2, 7.5, 7.5.3 and Mac OS 7.6, 8.0, 8.1 and 8.5; these are
  build-specific observations, not exhaustive compatibility certification
  ([#174](https://github.com/jackwillis/globe/issues/174#issuecomment-6969352476), [#175](https://github.com/jackwillis/globe/pull/175)).
- Both frontends show a center-distance scale and a compass rose that follows
  the view's north, including during movement. Scale units follow Preferences;
  Classic uses Monaco for the label. Jack observed these on the G4 in
  development builds ([#81](https://github.com/jackwillis/globe/issues/81), [#140](https://github.com/jackwillis/globe/issues/140)).
- H or ? shows Controls in the information pane; Escape hides it. Classic also
  offers Apple menu > Controls and lists Command alternatives and plain-only
  keys ([#40](https://github.com/jackwillis/globe/issues/40), [#144](https://github.com/jackwillis/globe/issues/144)).
- Classic About displays the full project version
  ([#136](https://github.com/jackwillis/globe/issues/136)).
- The Classic release adds a hybrid ISO-9660/HFS CD image alongside StuffIt.
  See the [CD guide](docs/os9-cd-image.md) for Finder installation and testing details.
- Public website with genuine bare-Mac-OS-9 screenshots, accepted by Jack for
  v0.1.1; onboarding and installation docs were refreshed
  ([#129](https://github.com/jackwillis/globe/issues/129), [#192](https://github.com/jackwillis/globe/pull/192), [#118](https://github.com/jackwillis/globe/issues/118)).
- After v0.1.0, Jack reported sustained use without recurrence of the unfinished
  globe edge. [#74](https://github.com/jackwillis/globe/issues/74) closed on that
  observation, without a structured test or proof of a permanent fix. The
  original release's known limit remains recorded below.

## 0.1.0 - public preview (2026-11-13)

The first public release of **Poodle Earth** (repository `jackwillis/globe`), for Mac OS 9 on PowerPC.
See the [published v0.1.0 release](https://github.com/jackwillis/globe/releases/tag/v0.1.0)
and [release notes](docs/release-notes-0.1.md).

- Map and Relief looks, progressive software rendering, country and city picking with information
  cards, night side, measurement and units; one Classic window with a sidebar, native menus and
  an About window with data credits.
- Distribution: StuffIt archive only; exact-version source archive,
  manifest and checksums. Application, Finder name and creator code changed from the development
  `GlobeFloppy` / `Glb2` to **Poodle Earth** / `PdlE`.
- Known limits: occasional unfinished globe edge (#74), small embedded data set, no icon yet.
- Version is `0.1.0`; the historical prototype tag `v1` is not a 1.0 release.
