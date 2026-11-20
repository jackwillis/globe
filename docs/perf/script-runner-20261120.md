# Script runner prototype: host and cross-build evidence, 2026-11-20

**Historical implementation evidence at reviewed head `201d344`.** Current corrections,
size measurements and acceptance details are in
[the independent-review follow-up](script-review-followup-20261120.md). In particular,
these original tests did not expose the idle-wait scheduling defect; the follow-up does.

Issue #221, branch `codex/classic-script-runner`, baseline `5d7dfe2`.
Implementation/contract/owner procedure: [script-runner.md](../script-runner.md).
All evidence here is Linux host or compile/package evidence. No G4, transfer, emulator,
Classic runtime, physical display, native UI acceptance, or release operation was performed.

## Host

Linux x86_64, kernel `7.2.10-200.fc44.x86_64`, GCC 16.2.1, Clang/clang-format/
clang-tidy 22.1.8; Ninja, Python 3.14.8. Worktree `/tmp/poodle-script-runner`.

```sh
cmake -S . -B build -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j4
ctest --test-dir build --output-on-failure
clang-tidy -p build src/platform/common/script.cpp --quiet
clang-format --dry-run --Werror src/platform/common/script.cpp src/platform/common/script.hpp \
  src/platform/os9/script_session.cpp src/platform/os9/script_session.hpp \
  src/platform/os9/main.cpp src/platform/os9/menu_bar.cpp include/globe/menus.hpp tests/test_script.cpp
```

**70/70 tests passed**, sequentially (116.78 s). Existing trace/Model replay, deterministic
render tests, X11 startup, replay, pixel verification at 16/24-bit, toolbar, repaint,
zoom, progressive rendering and idle tests passed. Focused script C++ and independent
Python JSON tests also passed. New source clang-tidy and changed C++ format checks passed.
Toolbox-only code is covered by cross compilation, not host clang-tidy.

Initial sandboxed X11 tests failed opening local X sockets. An unrestricted parallel run
had three Xvfb depth/verification failures; those four smoke/pixel cases passed sequentially,
then the complete sequential suite passed. No unrelated X11 test or renderer changes were made.

```sh
cmake -S . -B build-sanitize -G Ninja -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-sanitize --target test_script test_app test_classic_present test_classic_wait -j4
ctest --test-dir build-sanitize \
  -R '^(script|script_json|app|classic_present|classic_wait)$' --output-on-failure
```

**5/5 passed**, ASan/UBSan with LeakSanitizer, outside the ptrace sandbox. Within the sandbox,
LeakSanitizer itself refused ptrace after otherwise successful tests; it was not suppressed.
Tests count global C++ `new`/`new[]` around the complete four-command runner/report and
individual JSON serialization: **zero new allocations**. This is not an allocation claim
about native disk/dialog internals or rendering/refinement jobs.

## Cross build and packaging

Local image `ghcr.io/autc04/retro68:latest`, PowerPC GCC 16.1.0, normal application `-Os`,
C++20, no exceptions/RTTI, timing title off, embedded floppy data unchanged. These are
local compile candidates, not distributable releases or native acceptance.

```sh
podman run --rm -e SOURCE_DATE_EPOCH=1795152300 \
  -v /tmp/poodle-script-runner:/src:Z -w /src ghcr.io/autc04/retro68 sh -ec '
  cmake -S os9 -B build-os9-script \
    -DCMAKE_TOOLCHAIN_FILE=/Retro68-build/toolchain/powerpc-apple-macos/cmake/retroppc.toolchain.cmake
  cmake --build build-os9-script -j2
  tools/package-floppy.sh build-os9-script'
```

Build, Rez resources, PEF/MacBinary and 1.44-MB HFS packaging passed; application forks
round-tripped byte-identically from the HFS image. Baseline was exported from `origin/main`
with `git archive` into `/tmp/poodle-script-baseline` and built with the same image/options.
No other agent's build/worktree was used. The compiler reports existing low-memory QuickDraw
address/array warnings in `main.cpp` in both candidates; these are not native runtime proof.
The first compile caught a Toolbox `pascal` macro collision; the helper was renamed.

| Component | Baseline | Prototype | Change |
| --- | ---: | ---: | ---: |
| Data fork | 582,433 | 633,397 | +50,964 |
| Resource fork | 1,294 | 1,706 | +412 |
| MacBinary | 584,064 | 635,392 | +51,328 |

Final payload: **677,227 bytes**, **522,773 bytes** of the
1,200,000-byte payload budget remain. HFS image: 1,474,560 bytes.
Build identity is explicitly unavailable in this container,
whose source `.git` points outside the mount. No invented revision appears in JSONL.

`tools/probe-retro68.sh` passed all **68 compile checks** (34 each, PowerPC and m68k,
including the new shared script source). No `FAIL`/compiler error or warning remained.
68k compilation is subset compatibility evidence, not a 68k application/runtime claim.
An earlier probe overlapped a separate `:Z` container mount and hit SELinux access denials;
the successful final probe ran alone. The wrapper exit status alone is insufficient:
its log was also checked for `FAIL` and `error:`.

## Memory and performance limits

On x86_64: `sizeof(ScriptRunner) == 120`, `sizeof(JsonRecord) == 4112` (4096 payload bytes
plus bookkeeping). Native sizes differ with the ABI; these are not measured Classic heap
figures. Session state is small and embedded; no source allocation, file/dialog access or
transcript buffer is created at startup. A script read allocates at most 16 KiB (one byte
for an empty script), released at finish/cancel/error. Writer storage is on the stack;
error finalization can temporarily nest record-writing calls, so 4096 bytes is not a
claim of maximum total stack use. No transcript history grows in RAM.

User-requested state changes may allocate the ordinary refinement job; the serialization
zero-allocation claim excludes that work. Each line/record and the whole source are bounded.
Native file writes are synchronous and can stall; no G4 latency, startup timing, whole-app
heap peak, stack peak, or physical-display correctness measurement is available.
