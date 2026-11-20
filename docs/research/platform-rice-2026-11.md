# Platform RICE and package-channel research — November 2026

**Status:** Dated, exploratory quantitative planning snapshot. **Not a supported-platform inventory, commitment, approved milestone, revenue/adoption forecast, or permission to begin a port.** Research archive issue: [#213](https://github.com/jackwillis/poodle-earth/issues/213). Official release planning: [#212](https://github.com/jackwillis/poodle-earth/issues/212).

## Provenance and preserved inputs

This records the user's expanded Poodle Earth RICE workbook dated **2026-11-19**. That workbook retained the earlier **2026-11-16 baseline of 49 scenarios** and added **22**, giving **71 distinct OS/version/ISA/ABI scenarios**. It also compared **45 package/distribution channels** (29 earlier plus 16 additions). The original editable workbook has 11 worksheets and remained in the user's ChatGPT file library; it was **not** committed because [AGENTS.md](../../AGENTS.md) prohibits committing binary files. Its locally observed SHA-256 was:

    8631ceac20707c16b4d3abe8e5b84fc434b2ced7ee3b23172c44cd6f12f2431f

Two small, textual **input snapshots** preserve every scenario's key ranking inputs and rounded outputs:

- [71 OS/architecture/ABI RICE rows](platform-rice-scenario-inputs-2026-11.csv)
- [45 package-channel value/day rows](platform-package-channel-inputs-2026-11.csv)

These CSVs are **not a complete serialization** of the original workbook: separate CD-reach inputs, detailed blocker/evidence text, references for each row, original 49-platform formulas, chart formatting, and the sensitivity simulation remain in the owner-held workbook. Do not mistake a CSV snapshot for verified programmatic reproducibility of the entire workbook.

## Model

The workbook defines digital:

    RICE = estimated distinct adopters per future quarter
           × per-user impact (0.5 to 3)
           × subjective confidence (0 to 1)
           ÷ estimated engineering effort in person-months.

This is a *relative planning heuristic*. In particular:

- **Reach is an invented prior**, not observed users, survey evidence, traffic, or a demand forecast.
- Effort estimates are unvalidated and often mix first-port work and incremental release work.
- Confidence bundles market, technical and acceptance uncertainty; separating these would improve the model.
- Platform scenarios **overlap** (e.g. Linux x86_64 across distros, modern Windows and Windows ISA variants); neither reach nor RICE can be summed.
- A score cannot override the project's source, licence, build, runtime, ABI, memory, installation or hardware-acceptance gates.
- A published PPC Classic application, a headless NetBSD/macppc benchmark and an X11 host build represent **different levels of evidence**.

Packaging is separately scored:

    package incremental value/day = subjective benefit (1–5)
                                     × subjective confidence
                                     ÷ incremental engineering days

It is **not** additive to platform RICE: .deb, RPM, AppImage and a portable archive may package the same Linux executable. Some workbook channel labels (e.g. source tarball "future"; Classic Mac "installer") are preliminary and should be reconciled to actual release evidence before operational use.

## Illustrative ranks

| Rank | Scenario | Workbook digital RICE | Evidence caveat |
| ---: | --- | ---: | --- |
| 1 | Classic Mac System 7.1.2–9, PPC | 762.6 | Released Classic PPC app; physical acceptance is OS 9, not proof of every listed OS version |
| 2 | Linux x86_64 | 246.0 | Host-built/tested X11; package and baseline ABI unqualified |
| 3 | NetBSD 11.1/macppc PPC | 118.3 | G4 headless native tests; X11 GUI/pkgsrc install and release acceptance outstanding |
| 4 | Modern Windows x86_64 | 85.8 | A native frontend and target runtime evidence still needed |
| 5 | NetBSD amd64 | 61.0 | Reasonable pkgsrc candidate; target acceptance unrecorded |
| 6 | Linux aarch64 | 50.7 | Different architecture and binary; untested |
| 7 | FreeBSD amd64 | 47.6 | OS-native build/pkg/smoke still required |
| 8 | Windows 2000/XP i686 | 42.5 | Historical Win32 runtime and ABI feasibility unknown |
| 9 | Modern Windows i686 | 37.9 | Potentially overlaps other Windows choices |
| 10 | Browser/WebAssembly wasm32 Reader | 33.5 | New frontend/execution work, not a native OS release |

All numbers are from the spreadsheet's **November 19 scenario inputs**, not a prospective commitment. Rounded ties retain the same rank in the CSV.

### Long shots specifically raised

| Candidate | Workbook score | Cheapest useful research gate |
| --- | ---: | --- |
| Redox OS x86_64 | 1.6 | Confirm C++20/relibc/toolchain floor and actual X11 versus Orbital pathway, then tiny native executable |
| MorphOS PPC | 1.5 | Historical PowerPC compiler/ABI and graphics feasibility |
| Vinix x86_64 | 1.1 | Current V-based OS native toolchain, C++ compatibility, graphics/process capability |
| SerenityOS x86_64 | 0.7 | Native libc/C++ ABI and graphics presentation |
| Vinix ARM64 | 0.4 | Actual architecture/runtime evidence, not merely project plans |
| Genode / Sculpt x86_64 | 0.4 | Native depot/component feasibility and display architecture |
| MINIX 3 i386 | 0.2 | Historical pkgsrc/X11 status versus a working modern C++20 toolchain |
| 9front / Plan 9 amd64 | 0.2 | Language, graphics, runtime and portability boundary assessment |

These are **research leads**, not verified Poodle Earth ports. A [draft PR #208](https://github.com/jackwillis/poodle-earth/pull/208) documents Redox and MINIX as long shots; as of this archive, it was **not merged**. Vinix and additional niche-OS details are preserved as model scenarios here, not silently inserted into the accepted platform inventory.

## Packaging analysis and release sequence

Selected channel scores: source distribution plus reproducible scripts **2.43** (already partly published in releases, so the status label needs adjustment); Linux portable .tar.gz **1.19**; Flatpak **0.50**; Debian .deb **0.38**; NetBSD pkgsrc binary .tgz **0.37**; AppImage **0.35**; RPM **0.34**.

RICE efficiency is not the sole priority: a verified NetBSD/macppc pkgsrc package is strategically important to Poodle Earth's **"NetBSD of globes"** portability aim even if its channel score is lower than Linux tarball's.

The owner-discussed **candidate** sequence in [#212](https://github.com/jackwillis/poodle-earth/issues/212):

1. **v0.1.2 (official current)** — country names, font behavior and Classic hardware acceptance; no Java or new OS requirement.
2. **v0.1.3 (candidate, not committed)** — tested pkgsrc for a particular NetBSD target and a Linux x86_64 portable archive; AppImage only if qualified.
3. **v0.1.4 (candidate)** — native Debian/RPM packages, optionally one further Unix OS if target-native evidence exists.
4. **Independent research** — Windows XP, WebAssembly, old Mac OS X, Redox, Vinix, MINIX, etc., with cheap falsifying probes first.

Priorities should be retested against measured effort, native executable acceptance, and observed user demand. A historical **5,000-run sensitivity study applies only to the original 49-platform workbook**, not all 71 scenarios; its percentages are sensitivity to specified hypothetical perturbations, not probabilities of success.

## Limitations and related records

The spreadsheet's original reference URLs are not included in every CSV row; consult the owner-held workbook and these live records before citing specific target feasibility:

- [Cross-platform frontends and distribution](cross-platform-frontends-and-distribution.md)
- [Experimental methodology/registry](rendering-experiment-program.md)
- [Release candidates and unresolved v0.1.3 alternatives](https://github.com/jackwillis/poodle-earth/issues/212)
- [Unix package versus regional-data-pack separation](https://github.com/jackwillis/poodle-earth/issues/122)
- [Repository/build audit](repository-cmake-audit-2026-11.md)
- [Redox/MINIX draft research PR #208](https://github.com/jackwillis/poodle-earth/pull/208)
- [Architecture proposals #193 and #194](https://github.com/jackwillis/poodle-earth/issues/193)

**Decision not made:** No ranking here authorizes a port, platform support declaration, G4 operation, new release milestone or packaging submission.