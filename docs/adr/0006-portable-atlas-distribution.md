# ADR 0006 (proposed) — Portable geographic artifacts, native distribution

**Status:** Proposed, not accepted (2026-11-19). No production or release authorization.

## Context

Poodle Earth embeds enough global geography to run offline, while optional geographic detail is being investigated in #31, #48 and #50. Future platforms may distribute the same geography through pkgsrc, FreeBSD pkg, DEB/RPM, a Windows ZIP or NSIS installer, Classic Mac Data folders and an optical disc. Package-manager semantics must not become geographic file-format requirements. The existing GATL v1 reader is strict and reproducible; later indexed/resource formats are not fixed.

See [the detailed proposal with diagram and experimental staging tool](../research/portable-atlas-distribution.md), [CD research](../research/cd-rom-atlas-architecture.md) and [cross-platform research](../research/cross-platform-frontends-and-distribution.md).

## Decision proposed for owner review

1. Keep **data packs**, **editions** and **distribution packages** as separate concepts. Geography, stable identities, versioning, provenance and compatibility belong to the pack/edition layer; installation paths, package dependencies, shortcuts and ownership belong to native packaging.
2. Publish identical validated pack bytes into different platform distribution wrappers where licences and runtime compatibility permit. Package metadata can be generated from edition metadata but cannot change the on-disk geographic representation.
3. Preserve the useful embedded base Earth with no mandatory optional packages. Discover external packs from platform-provided paths without contacting the package manager; select detail and memory residency independently of installed file count.
4. Preserve read-only operation, bounded validation, safe resource replacement, version rejection, pack-removal fallback and explicit attribution.
5. Keep native C++20 and WebAssembly sharing the engine. A possible historical Java 1.1 Reader is a future consumer of stable format/semantic contracts, not a justification to prematurely freeze them.

## Consequences if accepted

- Smaller optional data packages may be reused across different architectures; each package manager still needs its native metadata and tests.
- Global and regional editions can be installed independently without forcing RAM use, but overlap resolution and file discovery need deterministic contracts.
- A Windows portable ZIP and NSIS installer can be built from the same staged files. Windows 9x installer compatibility and the native application are separate test gates.
- Native packaging and attribution checks become release responsibilities, while existing runtime/optical-drive tests remain separately required.

## Explicitly unresolved

- Final pack and manifest formats, version/compatibility fields and data dependencies.
- Geographic pack granularity, compressed chunk size, index or one-file/multiple-file decision.
- Identity overlap precedence and path search-order rules across user, system and removable media.
- Which regional data licences permit which distributions.
- Actual package names, prefixes, package repositories, installer scripts and CD layout.
- Any implementation or release milestone beyond explicitly assigned existing issues.

## Evidence required before acceptance

A byte-preserving staging proof (already proposed) is evidence for distribution portability only, not a production pack loader. Next obtain one external-pack runtime roundtrip, then pkgsrc install/remove against a real reader, native Win32/NSIS testing once a Windows executable exists, and optical-media measurements before finalizing a multi-platform disc. A failed experiment is a valid result.
