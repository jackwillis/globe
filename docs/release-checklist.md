# Release checklist

**Living, reusable checklist.** Apply the relevant items to each Poodle Earth release, recording evidence and justified exceptions on the release/hardware issue. This document is a review aid, **not authority to change release scope, access hardware, tag or publish**. Only the owner authorizes releases. See [agent contract](../AGENTS.md), [release engineering epic](https://github.com/jackwillis/globe/issues/6) and [recurring audit issue](https://github.com/jackwillis/globe/issues/127).

The public preview `v0.1.0` was published on **2026-11-13**; the historical prototype tag `v1` is not a semantic 1.0 release. Use the *actual* candidate version in CMake, application resources, archives and release metadata. Do not treat old planning documents or a previously closed defect as current release evidence.

## 1. Candidate, provenance and release scope

- [ ] Confirm the owner-approved release scope, target platforms, version, immutable candidate commit and applicable release/hardware acceptance issue. Do not silently add blockers or alter milestones.
- [ ] Verify source revision, clean tree, pinned compiler/container inputs, data editions, dependency/licence review and reproducible source acquisition.
- [ ] Compare version strings in CMake, native application resources/About, package metadata, filenames, manifest and proposed GitHub release. Record any deliberate difference (e.g. packaging revision).
- [ ] Review open defects and risk waivers against the candidate; distinguish reported symptoms, user-observed improvements, regression tests and genuine hardware acceptance.

## 2. Verification by platform

- [ ] Run applicable host unit/integration tests, sanitizer builds, deterministic renderer/pixel goldens, format/parser tests and geographic data QA. Record **exact commands, commit, OS/compiler, results and omissions**.
- [ ] Run every configured cross-build and inspect binary architecture, minimum ISA, dynamic dependencies/imports, startup/ABI constraints and packaging output. A successful cross-build does not certify native execution.
- [ ] For **Classic Mac OS**, confirm Retro68 compilation, SIZE/resources, data/resource-fork round trip, StuffIt expansion, installation, startup, menu/input/quit behavior, memory/resize and performance on specifically identified genuine hardware; distinguish Tiger Classic from bare OS 9 and emulators.
- [ ] For any **Unix/Linux/Windows** release, test the published binary/package in the claimed operating-system and architecture environment. Record libc/runtime/library baseline; test installation, launch, input, repaint, data lookup, clean exit and removal as relevant.
- [ ] For **pkgsrc NetBSD packages**, when introduced, validate source package metadata/build and resulting `.tgz` binary package with `pkg_add`, including required dependencies, installation paths, checksums and declared NetBSD/architecture compatibility. A generic `.tar.gz` is not a pkgsrc binary package.
- [ ] For **read-only or hybrid-media** editions, when introduced, verify the same byte-identical data packs on each supported view/platform, Macintosh forks/Finder metadata, missing/removable media, permissions and fallback to the embedded Earth. Do not claim physical-media support from an image-only check.

## 3. Documentation and repository hygiene

- [ ] Verify README, download/installation instructions, Classic ReadMe, user guide, contributor setup and `docs/README.md` reflect *shipped* behavior; try the documented build/test commands on a clean suitable host.
- [ ] Audit status and version claims across CMake, native metadata, README, documentation index, issue links, package manifests and release notes. Remove stale **current-state** assertions or explicitly mark them historical/proposed; do not rewrite the meaning of old results.
- [ ] Check relative Markdown links, examples, commands, filenames, screenshots, alt text, font/platform requirements and the distinction between headless renders and authentic screenshots.
- [ ] Review docs for duplicated or contradictory instructions. Archive valuable experiments, historical handoffs and benchmark evidence under a navigable history/research index; do not delete simply because old.
- [ ] Inspect the Git-tracked tree and release staging area for accidental binaries, generated outputs, caches, source archives, machine-specific paths, private host instructions and unnecessary files; investigate before removing.
- [ ] Run an appropriate **working-tree and Git-history secrets audit**. Record triage privately. Actual credential exposure requires rotation/incident handling and owner-approved remediation; deleting current text alone does not erase Git history. See [public-repository hygiene](public-repo-hygiene.md).
- [ ] Recheck first-party and third-party code/data licenses, provenance, attributions, source-offer language, notices and redistributed dependencies according to [licensing policy](licensing-policy.md). Do not silently approve new data sources.
- [ ] Record exceptions, documentation debts and security follow-ups in existing issue owners rather than leaving misleading claims or creating duplicate trackers.

## 4. Release artifacts and publication

- [ ] Produce complete corresponding GPL-3.0-or-later source and build scripts from the **same revision** as each executable; a GitHub URL by itself is not an adequate substitute for an appropriate physical-media source-compliance plan.
- [ ] Include license and data credits and all required third-party notices; do not include proprietary OS/ROM images or credentials.
- [ ] Verify package/archive format for each target (for example, Mac `.sit`; NetBSD pkgsrc `.tgz` when available), names, sizes, checksums, source revision, build inputs and installation instructions in the release manifest.
- [ ] Extract/inspect staged artifacts independently; verify Classic Mac forks where applicable and rebuild on a clean host before making any reproducibility claim. Report limitations instead of assuming bit-for-bit reproducibility.
- [ ] Obtain **explicit owner sign-off** for release scope, native acceptance, tags and publication. Keep build/test evidence separate from approval.

## 5. Changelog, release notes and follow-up

- [ ] Update **`CHANGELOG.md`** with a curated, accurate user-visible summary for the actual version, not a dump of commits; preserve the historical behavior/known limits of older releases ([#125](https://github.com/jackwillis/globe/issues/125)).
- [ ] Prepare **version-specific release notes** with supported platforms, real tests, known limits, artifact links, checksums and corresponding source. The published GitHub release/tag is authoritative for what was actually shipped; do not list proposed features as released.
- [ ] Check that documentation and issue/milestone state agree with the release, but do not override owner decisions or `status: ready`. Link real verification records and any remaining tasks to the relevant issues.
- [ ] After publication, verify downloads, installation instructions, release links and checksums as seen by a newcomer. Keep a dated audit record with passed checks, exceptions, follow-ups and sign-off.

## Scope reminders

This checklist supplements the single release-specific hardware acceptance issue and existing [README/onboarding work](https://github.com/jackwillis/globe/issues/118). It does not create new milestones or change a patch release's agreed scope. Apply platform-specific items **only when that platform/artifact is actually shipping**. Do not execute checks on the owner's G4 without separately granted hardware approval.
