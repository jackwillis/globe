# Toolchains and measured limits

Everything here was measured on 2026-11-01. Nothing is claimed beyond what was run.

| Target | Compiler | Standard | Flags | Result |
| --- | --- | --- | --- | --- |
| Host (Fedora) | g++ 16.2.1 | C++20 | `-fno-exceptions -fno-rtti -Wall -Wextra -Wpedantic -Wconversion -Wshadow`, ASan+UBSan | core and all three tests build and pass |
| Mac OS 9 PowerPC | Retro68 GCC 16.1.0 (`ghcr.io/autc04/retro68`, image 7522dbcdad17) | C++20 | `-fno-exceptions -fno-rtti` | **compile only:** core and probe build |
| System 7 / 68k | Retro68 GCC 16.1.0, same image | C++20 | same | **compile only:** core and probe build |
| NetBSD/macppc | `powerpc--netbsd-g++` 12.5.0 (cross, `~/scratch/netbsd-xc`) | C++20 | same, `--sysroot` | **compile only:** core and probe build |

Probe (`tools/retro68-probe/probe.cpp`) confirmed on all three cross targets: `<optional>`,
`<variant>`, `<span>`, `<array>`, `constexpr` table generation, `<type_traits>`, with
exceptions and RTTI disabled.

## Host checks (2026-11-01)

CMake 4.3.0 + Ninja 1.13.2, GCC 16.2.1 with ASan/UBSan and clang 22.1.8: configure, build and
`ctest` pass (3 of 3). `clang-tidy` 22.1.8 reports nothing on `src/`. The tree is formatted with
`clang-format` 22.1.8, which CI pins via PyPI (`pipx`) rather than the distro package.

## Hardware run (2026-11-01)

The three unit tests were cross-built on the laptop (`powerpc--netbsd-g++` 12.5.0, `-std=c++20 -O2
-static -fno-exceptions -fno-rtti`, stripped, about 0.9 MB each since libstdc++ is linked
statically), pushed to `/tmp` on maul with `maulctl push nb`, and run there: NetBSD 11.0 macppc
(G4, big-endian). `bytes`, `framebuffer` and `projection` all printed `ok`, exit 0. `trackball` was added the same
way later that day (`ok`, exit 0, 1.94 s user for the whole test including a 200,000-drag drift check). `atlas`
followed (`ok`, exit 0, 1.63 s user, 3.2 s real): the little-endian decoder read the real
`data/atlas.gatl` on big-endian PowerPC and reproduced the checksums computed by the Python
decoder, so byte-order handling is confirmed on hardware, not only by construction.

Later the same day: `render` (golden frames identical on x86 and PPC; 16.8 s user, dominated by two
absurd-zoom frames, not representative of normal frames) and `app` (a 124-message recorded session
replayed on the G4 produced a final frame with the same FNV-1a hash as the host,
`b3519ed59541d2b0`, so trackball and rendering arithmetic agree across the two architectures for
that session; this is evidence, not a guarantee for every input). Nothing was
compiled on maul. No timing was taken; these are correctness runs only.

## Not measured

- Nothing has been built or run under Retro68 (Mac OS 9, 68k). The NetBSD/macppc run above
  checks correctness only, not speed or memory use.
- The GitHub Actions workflow has never run (no remote yet).

## Lessons carried over from the weather project

Retro68 printf has no `%z` or `%ll`; an OS 9 event pump must bound its work per call and call
`BeginUpdate`/`EndUpdate` for update events; Retro68 ships no Open Transport headers.
