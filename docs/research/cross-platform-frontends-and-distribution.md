# Cross-platform frontends and distribution — exploratory research

**Status:** Nonbinding exploratory planning, originally 2026-11-12; owner preferences clarified 2026-11-13. No runtime implementation, compatibility certification, approved toolkit, release gate, new milestone or platform support commitment follows. The Classic Mac OS 9 public preview 0.1.0 has shipped; v0.1.1 onboarding/release work and v0.2 atlas usability retain their existing scope.

**Related work:** [#83 X11 single-window layout](https://github.com/jackwillis/globe/issues/83), [#31 versioned data packs](https://github.com/jackwillis/globe/issues/31), [#72 optical-media research](https://github.com/jackwillis/globe/issues/72), [#48 optional Classic Mac data files](https://github.com/jackwillis/globe/issues/48), [#49 read-only volume policy](https://github.com/jackwillis/globe/issues/49), [#63 developer console](https://github.com/jackwillis/globe/issues/63), [#114 frontend presentation parity](https://github.com/jackwillis/globe/issues/114), [#126 repository-modernization planning](https://github.com/jackwillis/globe/issues/126), [#127 release hygiene](https://github.com/jackwillis/globe/issues/127), [PR #110 geographic direction](https://github.com/jackwillis/globe/pull/110) and [PR #113 experiment registry](https://github.com/jackwillis/globe/pull/113). See also [layered CD-ROM atlas research](cd-rom-atlas-architecture.md), [toolchain evidence](../toolchain.md), [GATL v1](../atlas-format.md) and [architecture](../architecture.md).

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
| Windows XP (first Windows prototype); Windows 2000 optional research | Win32/GDI, MinGW-w64 or tested FLTK option | Owner prefers XP first. C++20 runtime, imports and native behavior unverified; no Windows binary exists |
| Windows 98 SE (later experiment) | Older-toolchain Win32/GDI; avoid making current FLTK a prerequisite | Explore only after XP proof; CRT, ANSI/Unicode, CPU baseline, fonts and native behavior unverified |
| **Windows 95 / Me** | — | **Unverified research target.** Not covered by the cited toolchain's profiles; do not treat as a goal until evidence exists |
| Modern Windows | Reuse a compatible x86 build; native x64 only if justified | Unverified |
| Linux (x86_64, candidate i686/aarch64); NetBSD (macppc, amd64, candidate i386) | Existing Xlib reference; proposed FLTK Unix UI | NetBSD/macppc has real headless evidence. New platform builds, ABI floors and frontend acceptance remain unverified |
| DOS, OS/2, other vintage | None | Aspirational; no commitment |

A shared source tree does not mean one executable spans many OS releases: that holds only if ABI, APIs, C/C++ runtime, graphics and actual tests permit it. Record every claim per *(source revision, executable, ISA, OS version, driver, runtime libraries, test type)*.

## Unix packaging direction (owner preference, unverified)

- **NetBSD:** use a **pkgsrc package definition** and produce genuine installable **pkgsrc binary packages** (normally `.tgz`), rather than offering an ad hoc executable `.tar.gz` as the NetBSD release. Test `make package` / `pkg_add`, `PLIST`, dependencies, `LOCALBASE`, manifests and correct per-OS/per-architecture binaries. Candidate targets: macppc (PowerPC), amd64 and i386; only supported after matching target builds and runtime acceptance. See [official pkgsrc guide, binary packages](https://www.netbsd.org/docs/pkgsrc/). Publishing in the pkgsrc collection is a later, separate maintainership decision; a source tarball may still serve the pkgsrc distfile.
- **Linux:** initially consider one deliberately chosen ABI/libc and x86 ISA baseline, a portable application tarball and optional native Debian/Fedora packages. Do not equate a distro label with an independent executable requirement; x86_64, i686 and aarch64 are possible build/acceptance tracks, not five promised editions. Inspect ELF loader, glibc/libstdc++ symbols, X11/FLTK dependencies and kernel/syscall floor. Containers test userspace but do **not** prove old-kernel compatibility; use VMs for that.
- **Cross-platform release process:** normal source builds should remain simple CMake/CTest, with optional toolchain/preset files and separate package staging. Pinned CI images should make binaries reproducible where tested, without making GitHub Actions, proprietary services, or containers mandatory to build from source. Track broader [repository/build simplification in #126](https://github.com/jackwillis/globe/issues/126).
- **Support policy:** each artifact documents its target ISA, OS version, ABI/library requirements, build revision and native-runtime evidence; otherwise label it experimental or source-build only. The goal is *easy source portability*, not a support promise for Solaris, other Unix variants or untested computers.

## Unix UI: reconsider Xlib and evaluate FLTK

**Owner preference, 2026-11-13:** the current Xlib application feels awkward and is not the desired long-term Unix user experience. A new frontend may be preferable to continuing a large UI polish effort. The existing #83 single-window work and its tests are still evidence and must not be silently discarded. **Assessing or completing #83 is no longer proposed as a prerequisite for an isolated FLTK prototype**; the owner should decide whether to re-scope #83 once an alternative is evaluated.

Try one small FLTK 1.4.x native-window prototype using the unchanged framebuffer, core messages and render lifecycle, then compare it with the Xlib reference on NetBSD/macppc and at least one contemporary Unix host. Measure cold start, resident/peak application and toolkit memory, X-server resources, framebuffer conversion/copy, input-to-visible drag latency, idle CPU, window resize, native text/widgets, build/dependency complexity and usability. Keep the OS 9 Toolbox frontend separate. A 24 MiB target is a **hypothesis**, not an imposed limit. Do not remove `globe-x11` or its smoke/pixel tests before a verified replacement and an explicit owner decision. A second Unix frontend alone does not overturn [ADR 0002](../adr/0002-rendering-shared-code-and-usability.md).

FLTK officially spans Unix/X11/Wayland, Windows and macOS, but availability in a particular NetBSD package, static/dynamic dependency choice, or particular old Windows binary requires build/runtime verification. See [FLTK releases](https://www.fltk.org/software.php) and the [Windows compatibility notes](https://github.com/fltk/fltk/blob/master/README.Windows.txt).

## Windows and Tiger notes

**Windows.** Prototype **32-bit XP first**, then examine a modern Windows build and only later explore Windows 98 SE. A narrow Win32/GDI frontend avoids making Win9x compatibility depend on a modern toolkit. FLTK is an alternative for newer Windows, but its [1.4 introduction](https://www.fltk.org/doc-1.4/intro.html) mentions XP support while [current Windows README](https://github.com/fltk/fltk/blob/master/README.Windows.txt) cautions that 1.4+ may require Windows 10; resolve that conflict with pinned source and actual XP test, not a support assertion. Preserve UTF-8 names internally and test older ANSI/Unicode APIs explicitly. Check imported DLL functions, CRT, target ISA and C++20 runtime before Wine/ReactOS smoke and genuine XP acceptance. A Win98 edition may require a separate toolchain/build, even with the same UI source. MinGW Lite profiles remain candidates, not proof.

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

**First proof, if ever pursued:** a *small* test image with a resource-fork-correct Classic application, one Unix executable and one shared pack; verify each OS's directory view and the pack checksum. A pkgsrc `.tgz` on the disc would be **installed with pkg_add**, not launched in place; running directly from the CD would instead need a tested unpacked NetBSD executable and its compatible dependencies. The Mac app's forks remain in the HFS view, while shared pack bytes use ordinary file streams. A combined disc can differ from individual GitHub release downloads. No disc burning, full edition, milestone or physical-media target follows from this note.

## Small experiments before any decision

| Gate | Narrow proof | Evidence needed |
| --- | --- | --- |
| Native Win32 | Linux-cross-built core plus a minimal GDI viewport | Import audits, Wine smoke, then authentic XP/Win98 runs |
| ReactOS automation | Reproducible QEMU smoke run | Boot reliability, exit status, repaint/input, logs and screenshots |
| Hybrid disc | Small HFS/ISO image with one pack | Resource-fork integrity, Unix/Windows views, identical pack checksum, read-only behavior (#31/#48/#49/#72) |
| Tiger FOSS feasibility | Assess GUI headers and legal provenance | Documented SDK/API dependency graph; no proprietary files in a FOSS build |
| Unix toolkit | Independent minimal FLTK prototype, owner scheduling | Compare to Xlib reference and document #83 decision; see "reconsider Xlib and evaluate FLTK" |

These are optional experiments, not v0.1.1 or v0.2 release gates. No source, CI, interface, pack-format, roadmap or release change is authorized by this note.

## Starting points (not endorsements)

[FLTK](https://www.fltk.org/) · [MinGW-w64](https://www.mingw-w64.org/) and [MinGW Lite](https://github.com/redpanda-cpp/mingw-lite) · [NetBSD build system](https://www.netbsd.org/docs/guide/en/chap-build.html) · [Retro68](https://github.com/autc04/Retro68) · [ReactOS](https://reactos.org/) · [Wine](https://www.winehq.org/) · [Apple's Darwin architecture notes](https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/Architecture/Architecture.html).
