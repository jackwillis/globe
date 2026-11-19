# Platform qualification and release editions — policy proposal

**Status:** Proposed for owner review, 2026-11-18. Not an adopted release policy or a new release gate.  
**Related:** [Roadmap](backlog.md), [proposed ADR 0006](https://github.com/jackwillis/globe/pull/193), [platform research](research/cross-platform-frontends-and-distribution.md), [portable architecture #3](https://github.com/jackwillis/globe/issues/3).

## The three questions

These are **views of one project**, not three competing roadmaps:

- **Atlas / product:** *What can Poodle Earth do?* Existing `v0.1.x`, `v0.2`, `v0.3`, `v0.4` milestones describe approved user outcomes and the release gates for them.
- **Platforms / qualification:** *Where does it work, and what is the evidence?* A target is an explicitly bounded **OS/version floor + CPU architecture/ISA + ABI/runtime + frontend** combination. A toolkit that compiles is not proof of OS support.
- **Editions / releases:** *What can somebody obtain and run?* A published edition adds a **product version + target tuple + distribution artifact** with declared features, dependencies and acceptance evidence.

Product and platform progress are independent until the owner commits a specific edition to a release. A successful port experiment does not by itself create a milestone obligation or a supported-platform claim.

## Qualification ladder — evidence, not priorities

| State | Evidence required to advance |
| --- | --- |
| **Candidate** | Purpose, plausible approach, largest unknowns and a bounded first experiment; no executable claimed |
| **Buildable** | Reproducible target build/link, toolchain provenance, architecture/ABI/dependency inspection |
| **Runnable** | Execution on the *named target OS* with working frame, input and clean exit; record guest vs physical evidence |
| **Distributable** | Useful declared feature subset, package/media test, data integrity, install/run instructions and known limitations |
| **Supported** | Owner-approved publication and ongoing regression/maintenance policy, target acceptance and a named supported OS/ISA/runtime floor |

These states are **not** the existing GitHub `P0`–`P3` issue priorities. Cross-compilation, Xvfb, Wine, ReactOS, emulation and physical hardware provide different evidence. Label results precisely; an emulator is valid target-OS evidence when identified, not proof of every real machine. "Supported" is a bounded compatibility commitment, not permanent support.

Keep qualification evidence next to an existing platform issue whenever possible. For active release candidates, use one compact register entry: target tuple; frontend; state; source revision/artifact; tested OS/VM or device and date; tests; blockers; maintenance/release owner. Do **not** create a speculative row, issue, or CI job for every historical computer.

## Shared versus edition-specific guarantees

- **Shared:** validated atlas/pack formats and stable geographic identity semantics; portable core behavior and deterministic/regression contracts for supported operations.
- **Frontend-specific:** native windows, menus, layout, fonts, controls, controller/touch support, optional productivity tools. Document capabilities without automatic UI-feature parity debt (e.g. FLTK gamepad controls are not an Xlib requirement).
- **Release-specific:** only the **editions explicitly included in that release** must meet their declared capabilities, packaging, relevant tests, and authorized target acceptance. A release milestone is *not* a promise for every buildable frontend or OS family.
- **Regression guard:** continue the relevant shared-core, Classic, Xlib and other maintained build/test checks even when those targets are not distributed in a release. Do not confuse a green cross-build with native acceptance.

The owner approves changes to supported-platform declarations, release scope, milestone readiness, hardware access, merges and publication under [AGENTS.md](../AGENTS.md). An edition may lag the latest product version without becoming an automatic blocker; document its **actual** product version instead of inventing a parallel port version.

## GitHub planning rules

1. **One set of product/release milestones.** Put only approved, version-specific deliverables under a milestone. Keep open-ended port investigations unmilestoned, linked to #3 or existing platform issues.
2. **Promote by evidence and decision.** A `runnable` experiment is not automatically assigned packaging work; a `distributable` artifact is not automatically a supported release.
3. **Prefer existing issues and links.** No new platform epics, labels, dashboards or mandatory parity checklist until real coordination pain justifies them.
4. **Keep port work bounded.** Record the question, target, acceptance/stop criterion, dependency and maintenance cost. Prefer one focused new-port investigation at a time when capacity is constrained; do not prohibit independent parallel work.

## Three decisions this must handle

- **Country labels on Classic Mac:** ship within an approved Classic release gate; retain shared-core and Xlib regressions, but do not demand identical Xlib widgets or a Windows port.
- **First Windows 98 SE execution:** record `runnable` *only after an actual Win98 guest/device smoke test*; no supported release or product milestone is implied.
- **NetBSD/macppc package:** only if the owner selects it for a release, qualify the precise binary/pkgsrc artifact, target environment, atlas behavior and installation before publishing.

This proposal is prospective: it must not reinterpret v0.1.0, silently override accepted ADR 0002 or existing owner commitments, or change a live milestone. Adoption requires an explicit owner review of the 2026-11-13 frontend-parity wording in the roadmap and proposed ADR 0006.
