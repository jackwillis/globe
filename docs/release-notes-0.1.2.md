# Poodle Earth v0.1.2 — release notes

**Candidate prepared November 21, 2026. Not published.** The most recent
published release is [v0.1.1](https://github.com/jackwillis/poodle-earth/releases/tag/v0.1.1).
Update the date, candidate revision and artifact evidence only after they are
verified and the owner authorizes publication.

## Changes since v0.1.1

- **Country names** appear after the globe settles. Classic Mac OS users can
  switch them on or off with **View → Country Names**; X11 users can use the
  F2 tools window. Names are not shown during drag or zoom.
- The native overlays use installed fonts. On Classic, Helvetica is preferred
  and Palatino is the fallback. The country anchors and priorities come from
  the pinned Natural Earth country data, and mismatched country editions
  disable the labels.

## Verification and limits

The feature was merged in [PR #185](https://github.com/jackwillis/poodle-earth/pull/185)
(commit `f4f4cf8ba6eddb1b3deb37019e44f91e3678e6cb`).
On the integrated PR head `9c8c3bea445d9d404f766aed4a13822779901a3d`,
[Checks](https://github.com/jackwillis/poodle-earth/actions/runs/44873279919)
and [Release build](https://github.com/jackwillis/poodle-earth/actions/runs/44873279766)
passed. Host GCC/Clang suites, X11 label smoke tests and Retro68
cross-build/package checks passed.

The owner visually accepted an earlier country-label build (`b7aa82a`) on
a Power Mac G4 running bare Mac OS 9 and reported no perceptible slowdown.
This observation does not certify the final 0.1.2 release artifact. Numeric
G4 timing and font-removal tests were waived for this patch, and actual Classic
glyph/shadow ink bounds remain unverified by owner decision.

Country labels can extend past borders or be suppressed in crowded views.
Each country has one fixed anchor; the label may disappear near the limb.
The compact atlas, ASCII names and existing geographic coverage are unchanged.
City-name labels and new platform packages are not part of v0.1.2.

## Distribution

The intended public binary is the PowerPC Classic Mac OS application, with a
StuffIt archive and hybrid HFS/ISO image. Matching source, `RELEASE.json`
and `SHA256SUMS` must be generated and verified from one final clean revision.
The Unix/X11 frontend remains available as a source build, not a new binary
download. See the [release checklist](release-checklist.md).

**Pending:** final candidate build and fork/archive checks; immutable source
revision, final checksums and artifact links; owner publication approval.
No final v0.1.2 download or binary verification is claimed here.

License: GPL-3.0-or-later. Geographic data notices are in
[DataCredit](../data/DATA-CREDITS.txt).
