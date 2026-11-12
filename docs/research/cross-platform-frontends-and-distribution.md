# Cross-platform frontends and distribution — exploratory research

**Status:** Exploratory planning, 2026-11-12. No runtime implementation, support promise, release gate, milestone, or approved platform/toolchain selection.

**Related work:** [#83 X11 single-window layout](https://github.com/jackwillis/globe/issues/83), [#31 versioned data packs](https://github.com/jackwillis/globe/issues/31), [#72 optical-media research](https://github.com/jackwillis/globe/issues/72), [#48 optional Classic Mac data files](https://github.com/jackwillis/globe/issues/48), [#49 read-only volume policy](https://github.com/jackwillis/globe/issues/49), [#63 developer console](https://github.com/jackwillis/globe/issues/63), and [PR #110 geographic workstation direction](https://github.com/jackwillis/globe/pull/110). See also [layered CD-ROM atlas research](cd-rom-atlas-architecture.md), [toolchain evidence](../toolchain.md), [GATL v1](../atlas-format.md), and [architecture](../architecture.md).

## Product thesis

**A contemporary free-software geographic atlas that remains usable across generations of hardware:** modern, reproducible cross-builds produce native-feeling programs for different desktop systems, while sharing a portable geographic engine, semantic commands, and versioned geographic data.

This is *not* a plan to port every platform immediately. The active v0.1 goal remains a stable, usable Classic Mac OS 9 release. The existing NetBSD/X11 application and native Mac frontend are real baselines; proposed Windows, FLTK, Tiger, and hybrid-disc paths are experiments.

## Working principles

1. **One portable engine, platform-specific presentation.** The portable C++20 core owns geography, rendering, navigation, commands, and `Model / Msg / update / Effects`. Frontends adapt OS events, native windows, controls, fonts, clocks, file paths, and software framebuffer presentation. Do not put OS APIs in the core or create a large cross-platform widget framework merely to unify a few handlers.
2. **Keep existing working ports while experimenting.** Continue shipping/testing the current Xlib frontend until a replacement is measured and accepted. The native OS 9 Toolbox/QuickDraw frontend remains independently valuable.
3. **FOSS build inputs by default.** Prefer modern free-software compilers, linkers, headers/import libraries, libraries, packaging tools, and scripts usable from Linux. Pin revisions and record licenses, provenance, hashes, configurations, runtime assumptions, and reproducibility limitations. Building in GitHub Actions is convenient; the actual scripts should not require a proprietary CI service.
4. **Cross-building is not compatibility proof.** Distinguish compile/link success, binary/API inspection, emulated or compatible-system execution, and authentic OS/hardware acceptance. Do not label an untested target “supported.”
5. **Shared read-only geography.** Existing embedded Earth remains usable when no optional pack exists. Versioned external geography must validate before publication; one compatible pack should be readable regardless of operating system or storage medium. Preferences and optional caches belong on writable user storage, not next to an app on CD.

## Candidate UI and platform strategy

| Target | Proposed frontend / toolchain | Current status / question |
| --- | --- | --- |
| Classic Mac OS 9, PowerPC | Existing native Toolbox/QuickDraw, Retro68 | Existing build and hardware evidence; preserve v0.1 release gate |
| Linux / NetBSD / FreeBSD, X11 | FLTK 1.4.x as an experiment; retain current Xlib | Does FLTK improve usability while meeting G4 CPU/RAM and input latency constraints? |
| Modern Unix with Wayland | Modern FLTK where supported | Evaluate only after X11/macppc feasibility |
| Windows XP, i386 | Native Win32 controls/GDI; GCC/MinGW-w64 targeting an XP-compatible runtime | Cross-build the actual C++20 core and verify PE imports and execution |
| Windows 98 SE, i386 | Same Win32 frontend, avoiding APIs absent from Win9x; investigate MinGW Lite | Modern compilation is plausible, but CRT, ANSI/Unicode, CPU baseline, fonts, and native execution are unproven |
| Mac OS X 10.4 Tiger, PowerPC | Separate Carbon/Cocoa/Unix UI research | Cross-compilation exists, but a strictly FOSS native GUI SDK/interface story is unresolved |
| DOS, OS/2, other vintage targets | No selected frontend or toolchain | Aspirational; no release commitment |

**Unix UI experiment.** Prototype `globe-fltk` alongside `globe-x11` with a globe viewport, menu, sidebar, info card, and basic input. Keep software rasterization and existing renderer scheduling. Profile a minimal X11-only toolkit build on NetBSD/macppc. Compare cold start, incremental heap, peak/private memory, framebuffer copies, image presentation, drag latency, idle CPU, and UI usability against Xlib. **24 MiB is a benchmark hypothesis for a compact app configuration, not a promised total-system or process limit**; distinguish app memory, X-server allocations, and physical RAM. Modern hardware may use larger automatic budgets.

**Windows UI experiment.** A native Win32/GDI UI avoids tying Windows 98 compatibility to FLTK's modern Windows implementation. Use old-enough Win32 window, menu, dialog, input, and DIB/GDI presentation APIs; keep the engine and semantics shared. Preserve UTF-8 geographic names inside Globe; handle Win9x ANSI interfaces and lossless/lossy text presentation deliberately. Prefer a *single* Windows UI source tree and one executable across compatible releases if measurements prove practical, not by decree. Modern MinGW Lite Win98-target profiles are a promising **candidate**, not proof that Globe runs on Win98.

**Mac OS X Tiger exception.** Linux-hosted PowerPC Darwin 8 builds using modern GCC/ld64 have been demonstrated, but documented toolchains commonly rely on Apple's proprietary 10.4u SDK. Open-source Darwin alone does not include native Quartz/Carbon/Cocoa interfaces. Investigate a minimal independently implementable interface, alternate frontend, or an explicitly non-FOSS SDK-dependent build as separate options; do not claim a strict all-FOSS native Tiger pipeline exists. This is a distinct research blocker from GCC's ability to generate PowerPC code.

## CI and compatibility evidence ladder

A desirable long-term release workflow would be Linux-hosted and runner-independent, with pinned source/toolchain inputs and build artifacts for each target.

1. **Host checks:** portable C++20 tests, sanitizers, format/parser tests, deterministic atlas output, and stable rendering/trace fixtures where applicable.
2. **Cross-build checks:** build/link target binaries (Retro68, NetBSD cross tools, MinGW) and inspect target architecture, CPU baseline, static/shared dependencies, PE imports and Win32 API versions. Build-only evidence must say “build-only.”
3. **FOSS runtime smoke tests:** Xvfb/native Unix; Wine for fast Windows execution; ReactOS in QEMU for an NT-like guest with real Win32 GUI/GDI and filesystem interactions. ReactOS and Wine share some implementation lineage and neither proves authentic Windows compatibility.
4. **Authentic acceptance:** test actual bare OS 9, NetBSD/macppc, Windows XP SP3, Windows 98 SE, and other claimed targets on real hardware or carefully identified genuine OS guests. Proprietary guest OS installation media are separate from the fully FOSS cross-build pipeline. Record configurations and reproducible results.

Investigate the open-source [ReactOS QEMU GitHub Action](https://github.com/vmactions/reactos-vm), but pin a reviewed action commit and guest image; isolate boot/guest instability from Globe failures. Initially keep ReactOS tests informational or on a separate schedule until reliable. A successful ReactOS test does **not** prove Windows 98 behavior (Win9x is not NT).

## Shared hybrid CD-ROM hypothesis

A future disc could contain platform-native executables (Classic Mac, Unix, Windows) and common indexed geographic packs. Investigate **classic HFS + ISO 9660** hybrid mastering with **Rock Ridge** (Unix) and **Joliet** (Windows) extensions. Multiple filesystem directory views can reference the *same physical pack data sectors*; the Mac application's **resource fork, Finder type/creator metadata, and HFS view** still need special preservation and verification. Avoid claiming that HFS+, plain ISO, and classic HFS are equivalent.

Possible *logical* contents:

```text
MAC/          Classic app and resource-fork-correct metadata
UNIX/         architecture-specific executables
WINDOWS/      Win32 executables
DATA/         versioned shared geography packs
README.TXT    startup paths, requirements, source/license information
```

The precise per-platform directory views and ISO-level naming constraints remain to be tested. Consider conservative names for shared files; never bake host-specific paths into atlas packs. Each frontend should locate its app or disc root, validate optional compatible packs, and fall back to embedded Earth if unavailable.

The [GATL v1 format](../atlas-format.md) **already** defines fixed little-endian fields, byte-by-byte decoding, strict validation, CRC, and deterministic output: extend these invariants rather than redesigning them. Future resource containers in #31 need stable IDs, explicit byte order/widths, versioning, bounds, checksums, and tests on both little-endian x86 and big-endian PowerPC. Disc removal, optical seek/throughput, read-only startup, and media corruption belong to #48/#49/#72 and existing resource-budget work.

First disc proof: a *small* test image with a resource-fork-correct Classic Mac application, one Unix executable, and one shared pack; verify each OS's directory view and the pack's bytes/checksum. **No actual CD burning or full data edition until measured.**

## Small experiments before decisions

| Gate | Narrow proof | Evidence needed | Existing owner |
| --- | --- | --- | --- |
| FLTK on vintage Unix | New prototype frontend, no Xlib removal | NetBSD/macppc performance/RAM/UI comparison; native G4 confirmation | #83 and usability work |
| Native Win32 | Linux-cross-built C++20 core plus a minimal GDI globe viewport | XP/Win98 target import audits, Wine smoke, authentic target results | Research note only; implementation issue later |
| ReactOS automation | Reproducible QEMU guest smoke run | Boot reliability, command/exit status, GUI repaint/input, logs and screenshots | CI research only |
| Hybrid CD | Small HFS/ISO image with common pack | Mac resource-fork integrity, Unix/Windows-visible files, identical pack checksum, read-only behavior | #31/#48/#49/#72 |
| Tiger FOSS feasibility | Assess GUI headers/import interfaces and legal provenance | Fully documented SDK/API dependency graph; no proprietary files in FOSS build | Research only |

Follow these only after v0.1 release obligations, unless a limited experiment provides direct value without changing gates. No new issues or milestones are implied by this note. It should be reconciled with broader geographic-workstation direction in PR #110 without changing that PR's identity/navigation contracts.

## Research starting points (not endorsements)

- [FLTK project](https://www.fltk.org/) — toolkit source, versions, X11/Wayland behavior.
- [MinGW-w64](https://www.mingw-w64.org/) and [MinGW Lite](https://github.com/redpanda-cpp/mingw-lite) — Linux-to-Win32 builds and historical Windows target profiles.
- [NetBSD build system](https://www.netbsd.org/docs/guide/en/chap-build.html) — `build.sh` and cross tools.
- [Retro68](https://github.com/autc04/Retro68) — classic Macintosh GCC cross builds and open-source Multiversal Interfaces.
- [ReactOS](https://reactos.org/), [Wine](https://www.winehq.org/), [vmactions/reactos-vm](https://github.com/vmactions/reactos-vm) — Windows-style execution/test candidates.
- [genisoimage manual](https://manpages.debian.org/bookworm/genisoimage/genisoimage.1.en.html) — HFS, ISO, Joliet, Rock Ridge, Macintosh file metadata and hybrid mastering.
- [tiger-ppc-builds](https://github.com/danupsher/tiger-ppc-builds) — modern Tiger cross-toolchain with 10.4u SDK dependency.
- [Apple's Darwin/open-source component distinction](https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/Architecture/Architecture.html) — missing proprietary GUI layers.

**Planning boundary:** no source, CI, interface, pack-format, roadmap, or release change is authorized by this exploratory document.