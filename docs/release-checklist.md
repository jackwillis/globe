# Release checklist

Prospective release checklist for issue #12. **No release is authorized or published by this document.**

## Candidate and version

- [ ] Select immutable revision/tag and one user-visible version. Resolve current CMake `0.0.1`, proposed v0.1 and historical prototype tag `v1` without renaming history.
- [ ] Freeze source/data inputs; record toolchain versions, image digests, pack revisions and accepted geographic-source licenses.
- [ ] Review README, Classic ReadMe, About/credits, changelog and supported-platform claims against actual evidence.

## Verification

- [ ] Host tests, renderer goldens, sanitizers, data QA, format checks.
- [ ] PowerPC Retro68 build, portable 68k compile probe if relevant, application SIZE/resource checks.
- [ ] Build .sit and .bin and appropriate read-only image; independently extract and compare both Mac forks.
- [ ] Check StuffIt extraction and startup. Explicitly distinguish Tiger Classic, emulator, and bare Mac OS 9.
- [ ] Owner signs off native OS 9 menu, input, Quit, rendering, memory partition and acceptable G4 performance.
- [ ] Document any unresolved release defects (#74 limb chunks among them).

## Artifacts

- [ ] Include the complete GPLv3 corresponding source and needed build scripts from **the same revision** as binaries; a GitHub URL alone isn't a substitute for physical-media source compliance.
- [ ] Include GPL, attribution/data credit and third-party notices; no proprietary Apple ROM/OS images.
- [ ] Produce manifest recording commit, compiler/container image, data/source version, file sizes, SHA-256 and build limitations.
- [ ] Validate archive names, checksums, Macintosh metadata and read-only-volume behavior.
- [ ] Rebuild on a clean host before claiming reproducibility; record observed limits.
- [ ] Obtain owner approval before creating tags or publishing downloads.

## After release

- [ ] Dated CHANGELOG and release notes identify supported platforms, known limits, checksums and corresponding source.
- [ ] Update issues #12 and #47 with actual testing and download evidence.
