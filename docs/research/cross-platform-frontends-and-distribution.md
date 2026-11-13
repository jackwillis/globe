# Cross-platform frontends and distribution — exploratory research

**Status:** Nonbinding exploratory planning, 2026-11-12, condensed 2026-11-13. No runtime implementation, support promise, release gate, milestone or approved platform/toolchain selection. The active v0.1 goal remains a stable, usable Classic Mac OS 9 release; nothing here is a v0.1 or v0.2 requirement.

**Related work:** [#83 X11 single-window layout](https://github.com/jackwillis/globe/issues/83), [#31 versioned data packs](https://github.com/jackwillis/globe/issues/31), [#72 optical-media research](https://github.com/jackwillis/globe/issues/72), [#48 optional Classic Mac data files](https://github.com/jackwillis/globe/issues/48), [#49 read-only volume policy](https://github.com/jackwillis/globe/issues/49), [#63 developer console](https://github.com/jackwillis/globe/issues/63), [#114 frontend presentation parity](https://github.com/jackwillis/globe/issues/114), [PR #110 geographic direction](https://github.com/jackwillis/globe/pull/110) and [PR #113 experiment registry](https://github.com/jackwillis/globe/pull/113). See also [layered CD-ROM atlas research](cd-rom-atlas-architecture.md), [toolchain evidence](../toolchain.md), [GATL v1](../atlas-format.md) and [architecture](../architecture.md).

## Thesis and principles

A contemporary free-software geographic atlas that stays usable across generations of hardware: modern, reproducible cross-builds produce native-feeling programs, sharing a portable geographic engine, semantic commands and versioned geographic data. This is *not* a plan to port every platform.

1. **One portable engine, platform-specific presentation.** The core owns geography, rendering, navigation and `Model / Msg / update / Effects`. Frontends adapt OS events, windows, controls, fonts, clocks, paths and pixel presentation. No OS APIs in the core, and no large cross-platform widget framework to unify a few handlers.
2. **Keep working ports while experimenting.** The Xlib frontend and the native OS 9 Toolbox/QuickDraw frontend stay until a replacement is measured and accepted.
3. **FOSS build inputs by default.** Prefer free compilers, linkers, headers, libraries and scripts usable from Linux, pinned with licences, hashes and reproducibility limits. Convenient CI is fine; the scripts must not require a proprietary service.
4. **Cross-building is not compatibility proof** (ladder below).
5. **Shared read-only geography.** The embedded Earth works with no optional pack. Versioned external geography validates before use; one compatible pack is readable on any OS or medium. Preferences and caches go on writable user storage, never beside an application on a disc.

## Compatibility-family inventory

Research targets only. "Unverified" means no import audit or run on the genuine OS has been recorded.

| Family | Candidate toolchain / frontend | Status |
| --- | --- | --- |
| Classic Mac OS 9, PowerPC | Existing Toolbox/QuickDraw frontend, Retro68 | **Real, with hardware evidence.** The v0.1 target |
| Other Classic Mac (System 7/8, 68k) | Retro68 | Unverified. OS 9 source portability implies nothing about other APIs or hardware |
| Mac OS X 10.4 Tiger, PowerPC | Separate Carbon/Cocoa/Unix UI research | Modern GCC/ld64 cross-builds from Linux exist (e.g. [tiger-ppc-builds](https://github.com/danupsher/tiger-ppc-builds), which uses the proprietary 10.4 universal SDK). A strictly FOSS native GUI path is unresolved |
| Broader Mac OS X / macOS ranges (10.1–10.6, Intel 10.7+, arm64) | — | **Unverified research targets.** Do not promise one binary across them: Intel Macs began at Tiger, 10.6 is Intel-only, 10.15 dropped 32-bit apps, and SDK/deployment-target compatibility needs evidence |
| Windows XP / 2000 (NT-era) | Win32/GDI, MinGW-w64 | Plausible: cited MinGW Lite lists `winxp` and `win2000` profiles. Unverified for Globe's C++20 core |
| Windows 98 SE | Same Win32 frontend, Win9x-safe APIs | Plausible: MinGW Lite lists a `win98` profile. CRT, ANSI/Unicode, CPU baseline, fonts and native execution unproven |
| **Windows 95 / Me** | — | **Unverified research target.** Not covered by the cited toolchain's profiles; do not treat as a goal until evidence exists |
| Modern Windows | Reuse a compatible x86 build; native x64 only if justified | Unverified |
| Linux (x86_64, aarch64), NetBSD (macppc, amd64) | Existing Xlib first | NetBSD/macppc is owner-prioritized; keep the baseline narrow and retest each release. Other architectures and BSDs are source-portability only |
| DOS, OS/2, other vintage | None | Aspirational; no commitment |

A shared source tree does not mean one executable spans many OS releases: that holds only if ABI, APIs, C/C++ runtime, graphics and actual tests permit it. Record every claim per *(source revision, executable, ISA, OS version, driver, runtime libraries, test type)*.

## Unix UI: FLTK deferred

A toolkit-based Unix frontend (FLTK 1.4.x) is a possible experiment, but **it is deferred until the existing X11 single-window work under #83 has been assessed**; #83 is the active path and must show what remains unsatisfactory first. If revisited, prototype alongside the Xlib frontend (not replacing it) and compare cold start, incremental and peak memory, framebuffer copies, drag latency, idle CPU and usability on NetBSD/macppc. A 24 MiB budget would be a benchmark hypothesis, not a limit; distinguish application memory, X-server allocations and physical RAM. A second Unix frontend does not by itself reopen the "third platform" threshold in [ADR 0002](../adr/0002-rendering-shared-code-and-usability.md).

## Windows and Tiger notes

**Windows.** A native Win32/GDI frontend would avoid tying Win9x compatibility to a modern toolkit. Use old-enough window, menu, dialog, input and DIB/GDI APIs; keep engine and semantics shared; preserve UTF-8 names internally while handling ANSI interfaces deliberately. One Windows source tree, and one executable only if measurement shows it practical. MinGW Lite profiles are *candidates*, not proof that Globe runs on any of these systems.

**Tiger.** GCC's ability to generate PowerPC Darwin code is separate from having a native GUI SDK. Open-source Darwin lacks Quartz/Carbon/Cocoa. Options to investigate separately: a minimal independently implementable interface, an alternate frontend, or an explicitly non-FOSS SDK-dependent build. Do not claim a strict all-FOSS native Tiger pipeline.

## Evidence ladder

1. **Host checks:** portable C++20 tests, sanitizers, format/parser tests, deterministic atlas output, stable rendering/trace fixtures.
2. **Cross-build checks:** build and link target binaries (Retro68, NetBSD cross tools, MinGW); inspect target architecture, CPU baseline, dependencies, PE imports and Win32 API floor. Label results "build-only".
3. **FOSS runtime smoke tests:** Xvfb on Unix; Wine for fast Windows execution; ReactOS under QEMU for an NT-like guest ([vmactions/reactos-vm](https://github.com/vmactions/reactos-vm) is a candidate; pin a reviewed commit and guest image, keep it informational until reliable). ReactOS and Wine share some lineage, neither proves genuine Windows behavior, and ReactOS says nothing about Win9x.
4. **Authentic acceptance:** actual bare OS 9, NetBSD/macppc, Windows XP SP3, Windows 98 SE or any other claimed target on real hardware or identified genuine guests. Proprietary guest installation media are separate from the FOSS cross-build pipeline.

A future release workflow would be Linux-hosted and runner-independent with pinned inputs. Experiment methodology (predictions, controls, A/B/B/A) lives in the registry proposed in [PR #113](https://github.com/jackwillis/globe/pull/113), not here.

## Hybrid disc idea (not a release requirement)

A future disc might hold platform-native executables and one shared set of geographic packs, using **classic HFS + ISO 9660 hybrid** mastering with Rock Ridge and Joliet views that reference the *same physical pack sectors* (see the [genisoimage manual](https://manpages.debian.org/bookworm/genisoimage/genisoimage.1.en.html)). The Mac resource fork, Finder type/creator and HFS view need special preservation and verification; HFS, HFS+ and plain ISO are not equivalent. Illustrative logical layout: `MAC/`, `UNIX/`, `WINDOWS/`, `DATA/`, `README.TXT`; names stay conservative, host paths are never baked into packs, and each frontend locates its root, validates optional packs and falls back to the embedded Earth.

[GATL v1](../atlas-format.md) already fixes little-endian fields, byte-by-byte decoding, strict validation, CRC and deterministic output; extend these invariants. Future containers (#31) need stable IDs, explicit byte order and widths, versioning, bounds, checksums and tests on little-endian x86 and big-endian PowerPC. Removal, seek/throughput, read-only startup and corruption belong to #48/#49/#72.

**First proof, if ever pursued:** a *small* test image with a resource-fork-correct Classic application, one Unix executable and one shared pack; verify each OS's directory view and the pack checksum. No disc burning, no full data edition, no milestone or physical-media target follows from this note.

## Small experiments before any decision

| Gate | Narrow proof | Evidence needed |
| --- | --- | --- |
| Native Win32 | Linux-cross-built core plus a minimal GDI viewport | Import audits, Wine smoke, then authentic XP/Win98 runs |
| ReactOS automation | Reproducible QEMU smoke run | Boot reliability, exit status, repaint/input, logs and screenshots |
| Hybrid disc | Small HFS/ISO image with one pack | Resource-fork integrity, Unix/Windows views, identical pack checksum, read-only behavior (#31/#48/#49/#72) |
| Tiger FOSS feasibility | Assess GUI headers and legal provenance | Documented SDK/API dependency graph; no proprietary files in a FOSS build |
| Unix toolkit | After #83 is assessed | See "FLTK deferred" |

Pursue these only after v0.1 obligations, unless a limited experiment gives direct value without changing gates. No source, CI, interface, pack-format, roadmap or release change is authorized by this note.

## Starting points (not endorsements)

[FLTK](https://www.fltk.org/) · [MinGW-w64](https://www.mingw-w64.org/) and [MinGW Lite](https://github.com/redpanda-cpp/mingw-lite) · [NetBSD build system](https://www.netbsd.org/docs/guide/en/chap-build.html) · [Retro68](https://github.com/autc04/Retro68) · [ReactOS](https://reactos.org/) · [Wine](https://www.winehq.org/) · [Apple's Darwin architecture notes](https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/Architecture/Architecture.html).
