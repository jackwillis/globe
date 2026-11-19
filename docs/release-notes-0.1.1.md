# Poodle Earth v0.1.1 — draft release notes

**Draft for Jack's review, November 19, 2026. Not a release announcement.**
No final v0.1.1 candidate or artifacts were identified for this documentation
pass. The changes below describe main through `289106c` and this documentation
PR; reconcile them against the selected candidate before publishing.
The [v0.1.0 public preview](https://github.com/jackwillis/globe/releases/tag/v0.1.0)
remains the published download. Its original history is preserved in the
[changelog](../CHANGELOG.md) and [v0.1.0 notes](release-notes-0.1.md).

## Changes since v0.1.0

- Classic explains a memory-refused window enlargement and points to Get Info,
  once per session. Jack verified the layout and behavior under bare Mac OS
  9.2.2 at a 12 MB partition ([#116](https://github.com/jackwillis/globe/issues/116)).
- QuickDraw hand cursors remove the AppearanceLib startup dependency that broke
  the original download on Mac OS 8.1. Jack's preview smoke tests reached
  PowerPC System 7.1.2 through Mac OS 8.5 in Infinite Mac; see the
  [build-specific matrix and limits](https://github.com/jackwillis/globe/issues/174#issuecomment-6969352476).
  This is not a new blanket platform-support guarantee.
- A distance scale and north-following compass rose remain visible during
  movement. The scale follows metric/imperial units and uses Monaco on Classic
  ([#81](https://github.com/jackwillis/globe/issues/81), [#140](https://github.com/jackwillis/globe/issues/140)).
- H or ? opens a Controls list in the information pane; Escape hides it.
  Classic's Apple menu offers the same list, including Command equivalents
  ([#40](https://github.com/jackwillis/globe/issues/40), [#144](https://github.com/jackwillis/globe/issues/144)).
  About now displays the full version.
- The release packager can produce an emulator-friendly HFS/ISO CD image in
  addition to StuffIt. See the exact acceptance limits below.
- The accepted public [website](https://jackwillis.github.io/globe/) and genuine
  bare-OS-9 screenshots introduce the app. This docs pass refreshes the README,
  [user guide](user-guide.md), bundled ReadMe and contributor entry points.
  Screenshot build `110a197` is identified in the
  [provenance record](images/os9/README.md); screenshots are not final-candidate
  acceptance evidence.

## Installation and ISO evidence

StuffIt remains the standard Classic download: expand the `.sit` on the Mac,
keep the accompanying documents, then launch from a writable disk. The ISO
alternative avoids StuffIt: attach it as a CD in a compatible PowerPC emulator,
open it in Finder and copy the application and documents to the guest hard disk.
Use Finder so the application's resource fork and metadata survive.

Jack tested `PoodleEarth-PR172-2074034-local-hfs.iso` (source
`20740340c939bb5eda5185f5f288650db50850ee`, SHA-256
`1f4326ce52904f1f152f1d35ecca958974b7ed5cd5ce13bf1931913efb538021`):

- Burned physical CD on a Power Mac G4 under bare Mac OS 9.2.2: app, files and
  Get Info worked; memory settings were read-only until copied to hard disk.
  The native report did not separately specify the launch location.
- ClassicMac 4.7.0, emulated G4, Mac OS 9.2.1: launched from the mounted CD,
  copied to the guest desktop and launched the copy.

These are Jack's reports for that exact test image, not a v0.1.1 candidate test.
His [text-only retest waiver](https://github.com/jackwillis/globe/issues/171#issuecomment-7002974798)
allows these tests to carry forward only for subsequent text-only changes;
executable, Finder metadata and packaging changes are excluded. The
[CD guide](os9-cd-image.md) records provenance and host packaging checks.
Final artifact qualification and publication remain in
[#171](https://github.com/jackwillis/globe/issues/171).

## Limits carried forward

The compact atlas still has 1:110 million-scale outlines, one-degree terrain,
304 cities, ASCII names and English UI. Window size is bounded by application
memory (24 MB preferred, 12 MB minimum by default). There is no saved preferences
file, network fetch, custom app icon or 68k application. Unix/X11 remains a
separate source-build frontend with different controls; host tests do not
establish Classic hardware acceptance or full frontend parity.

Jack closed the unfinished-edge report after sustained use without recurrence,
not after a structured proof of a permanent fix. The original v0.1.0 limitation
remains part of its historical notes.

## Fill in at the final candidate

| Release detail | State |
| --- | --- |
| Release date and tag | Pending |
| Candidate source revision and matching app version | Pending; version bump is separate work |
| StuffIt, ISO and corresponding-source filenames | Pending |
| Asset byte sizes and SHA-256 checksums | Pending; use the actual release manifest |
| Release/download URLs | Pending; do not substitute test-build URLs |
| Exact candidate test evidence and waiver applicability | Pending reconciliation in #171 |
| Website download/version update and live asset checks | Pending final release artifacts |

No binary build, tag, release publication or repository rename is part of this
PR. The current repository remains `jackwillis/globe`; the rename is tracked for
v0.2 in [#124](https://github.com/jackwillis/globe/issues/124).

Licence: GPL-3.0-or-later. Geographic data carry separate notices in
[DataCredit](../data/DATA-CREDITS.txt) and the About window.
