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

## Not measured

- Nothing has been linked or run on a G4, under Retro68, or on NetBSD/macppc. Compiling is
  not evidence of speed, memory use or correct floating-point behavior there.
- The NetBSD library check used the sysroot headers only; no libstdc++ link was attempted.
- clang-format and clang-tidy are pinned to version 18 in CI. They were not installed on the
  development machine when M0 was written, so the format and lint steps have not been run
  locally and the CMake build has not been run locally (the tests were built with g++ directly).

## Lessons carried over from the weather project

Retro68 printf has no `%z` or `%ll`; an OS 9 event pump must bound its work per call and call
`BeginUpdate`/`EndUpdate` for update events; Retro68 ships no Open Transport headers.
