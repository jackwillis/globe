# PR #222 independent-review corrections — 2026-11-20

Reviewed head `201d344b782cc42eb823cba4bf72a59f4275d2a6`; P0 commit `c779696`;
follow-up remains within #221. Contract and revised owner procedure:
[script-runner.md](../script-runner.md). No native execution, G4 access, transfers,
emulator, merge, release or scope changes. Reused clean `/tmp/poodle-script-runner`;
remote head matched reviewed head; all four reviewed-head CI checks had succeeded.

## P0 falsification and correction

The reviewed-head wake policy selected only rendering's wake or the script deadline.
The runner intentionally required a later observation. With presentation already idle,
`WaitNextEvent` slept until timeout and the next observation failed before checking settle.
This defect was **not** covered by the original host tests; passing those tests was not
end-to-end scheduling evidence.

`ScriptRunner::wake_at(now, application_wake)` now returns the earliest of the existing
application wake, timeout and a 16-ms completion-observation wake. Time arithmetic is
bounded near clock overflow. `classic_wait_ticks` rounds positive intervals up to 60-Hz
native ticks. Only an active pending wait schedules these observations. The ordinary
outer event loop dispatches events, continues native presentation and steps the script;
there is no second loop and waiting alone never requests zero-delay polling.

`test_script` now simulates **no external input**. It runs the real runner/wake helper,
60-Hz sleep conversion, settled predicate, and (for pending work) a real
`ClassicPresentation` queue of 65 rows in three submissions. Idle completion passes on
the next observation; pending submission cannot pass before the queue drains. Short
timeouts and earlier real wakes retain priority; existing timeout, supersession and
cancellation tests remain. The test explicitly demonstrates the original deadline-only
policy yields error against an already-settled view.

A separate mutation replaced only the new helper's observation with `deadline_`, compiled
against the same tests/core, and exited **1 with three failing assertions**: idle completion,
pending-queue continuation and eventual pending-work completion. The production helper
passed. The mutation was in `/tmp`, not the repository:

```sh
c++ -std=c++20 -fno-exceptions -fno-rtti -Iinclude -Isrc/platform/common -Itests \
  tests/test_script.cpp /tmp/poodle-script-wake-mutant.cpp build/libglobe_core.a \
  -o /tmp/poodle-script-wake-mutant
/tmp/poodle-script-wake-mutant
```

## Formatting cause and effect

Local Retro68 image `ghcr.io/autc04/retro68:latest`, PowerPC GCC16.1.0, `-Os`, C++20,
no exceptions/RTTI, timing title off, same embedded data. Collected actual linked XCOFF
symbols with `powerpc-apple-macos-nm -S --size-sort` before and after:

```sh
/Retro68-build/toolchain/bin/powerpc-apple-macos-nm -S --size-sort \
  build-os9-script/format-before.xcoff
/Retro68-build/toolchain/bin/powerpc-apple-macos-nm -S --size-sort \
  build-os9-script/format-after.xcoff
```
 The before binary
includes `.snprintf`, `._svfprintf_r` (0x2480 = 9,344 bytes), `._dtoa_r` (0x1830 =
6,192 bytes), `._Balloc`, integer stdio wrappers and helpers. None of the snprintf/
svfprintf/dtoa symbols appear in the main baseline or corrected binary. Library stack
and all helper contributions cannot be inferred merely by summing those two functions.

A matched intermediate experiment changed **only UI formatting and its source-list entries**
after the P0 fix. Small bounded ASCII string/unsigned integer helpers replaced all four
snprintf uses (filename, BCD OS version, title, summary). No general formatter or dependency.

| Data fork | Bytes | Difference |
| --- | ---: | ---: |
| main `5d7dfe2` | 582,433 | baseline |
| reviewed head / P0 corrected, formatting retained | 633,397 | +50,964 |
| P0 corrected, formatting removed | 595,969 | **−37,428** vs retained |

Formatting therefore accounted for about **73.4%**, not all, of the reviewed feature's
50,964-byte growth. The remaining 13,536-byte growth in the isolated experiment includes
script/UI/JSON code and other dependencies. Final flush/title/menu changes have separate
small effects (final package measurements below).

Host tests check exact decimal/hex strings, OS 9.2.2 and BCD 10.3.0, filename truncation/
sanitation/fallback, maximum unsigned count, summary success/error/no-output, overflow
and NUL termination. These are fixed-buffer functions with no locale, variadic calls,
heap use or floating-point conversion.

## Durability and P2 decisions

Apple's [Inside Macintosh: Files](https://dev.os9.ca/techpubs/mac/pdf/Files/Intro_to_Files.pdf),
pp. 1-46 and 1-55, distinguishes closing file data from updating volume/catalog metadata.
The adapter retains the **Save As volume reference**, attempts close exactly once, then
calls `FlushVol(nullptr, selected_volume)` once. Write/close/flush errors latch failure
and prevent the final UI from claiming saved. The file reference is invalidated before
close; repeated cleanup cannot close it twice. Host fault tests cover all combinations
of close/flush success/failure, callback order, exact file/volume identity and repeated
cleanup. Actual disk behavior remains unverified. A flush can block and affect volume-wide
buffered metadata; it is an explicit finalization of user-requested output, not a new file
or periodic background operation. Successful API return is not a power-loss guarantee.
`end.status` still describes commands, before native close/flush; final UI is also required.

P2: preserve/restore the actual Pascal window title, including a timing title; place Run/
Cancel before Quit; distinguish an existing-file create error; gate **all** script stepping
on foreground state, so queued background events cannot execute commands. Monotonic elapsed
time still includes suspension; expired waits fail on resume. Escape only applies with the
Globe frontmost; File → Cancel Script works otherwise. Documented the pre-existing difference
between script edits (restart an old-layer job) and interactive edits (redraw without that
restart). Repairing interactive behavior is outside this focused assignment.

### Static stack evidence, not native low-water measurements

Used Retro68 `-fstack-usage -Os -std=c++20 -fno-exceptions -fno-rtti` on reviewed and corrected
native sources, plus the new formatter/shared runner. Compiler-local frames:

- `choose`: 1376 bytes before/after formatting; progress-title method: 288 before/after.
- Reviewed `finish`: 352 bytes. Corrected compiler split it into a 32-byte entry and
  a 368-byte body. This is **not** a claimed stack reduction; caller/local frames can grow.
- Bounded text helpers: filename 112, other public formats 96, integer digits 96,
  append 32 bytes; includes compiler spills/ABI, excludes callers.
- At reviewed head `5edde91`, JSON `start` and command `result`: 4208 bytes each;
  `end`: 4192. The final scope correction below ends the initial run-record lifetime
  before source-size error reporting. No whole-stack bound is inferred.

Removing formatting eliminates its libc call-chain dependency, but no runtime stack-peak
saving is claimed. Title preservation adds 256 bytes to application state. The revised
owner procedure prepares read-only `StackSpace()` samples under a native debugger, without
adding a test build, changing partition settings, or claiming samples have been collected.

## Final verification

Linux x86_64 Fedora kernel 7.2.10, GCC16.2.1; Clang/clang-format/clang-tidy22.1.8;
Python3.14.8. Commands from the isolated worktree:

```sh
cmake -S . -B build -G Ninja
cmake --build build -j4
ctest --test-dir build --output-on-failure
cmake -S . -B build-sanitize -G Ninja -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -g' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build build-sanitize --target test_script test_script_text test_script_output \
  test_app test_menus test_classic_present test_classic_wait -j4
ctest --test-dir build-sanitize \
  -R '^(script.*|app|menus|classic_present|classic_wait)$' --output-on-failure
clang-tidy -p build src/platform/common/script.cpp src/platform/common/script_text.cpp --quiet
tools/check-policy.sh
tools/probe-retro68.sh
```

Results: **72/72 full host tests passed** sequentially (118.04 s); **8/8 focused
ASan/UBSan/LeakSanitizer tests passed**. Updated presentation-queue regression and mutation
were rerun separately after strengthening the simulation. Existing JSONL/zero-allocation
tests remain passing. Changed C++ formatting, shared-source clang-tidy and the output
helper's host clang-tidy passed. Native Toolbox source is compile-checked, not host-linted.

Normal Retro68 app, Rez/PEF/MacBinary and 1.44-MB HFS packaging passed; application forks
round-trip identically. Final data fork: **595,991 bytes**
(**+13,558** vs main, **−37,406** vs reviewed head);
resource fork: **1,706**; MacBinary: **598,016**;
payload headroom: **559,877 bytes**. Native cross-build still has
pre-existing low-memory QuickDraw array/address warnings. No new formatting dependency
remains in final linked symbols.

```sh
podman run --rm -e SOURCE_DATE_EPOCH=1795152300 \
  -v /tmp/poodle-script-runner:/src:z -w /src ghcr.io/autc04/retro68 sh -ec '
  cmake -S os9 -B build-os9-script \
    -DCMAKE_TOOLCHAIN_FILE=/Retro68-build/toolchain/powerpc-apple-macos/cmake/retroppc.toolchain.cmake
  cmake --build build-os9-script -j2
  tools/package-floppy.sh build-os9-script'
```

**70/70 portable compile checks passed** (35 each for PowerPC/m68k, including the new
formatter), with no `FAIL` or compiler errors in the log. The m68k result is compile-only
subset evidence, not a 68k app/runtime claim. Earlier private/shared container mounts were
run sequentially to avoid SELinux relabel interference. All claims here distinguish host
simulation and compile/package evidence from native acceptance. CI on the corrected PR
head is separate from these local results.

Remaining P1/P2 evidence: real UI/title/menu behavior, actual file write/close/flush errors,
foreground switching and deadline delivery, ordinary-input responsiveness, physical pixels
and native StackSpace samples all require later owner acceptance. The pre-existing interactive
border/refinement discrepancy is documented, not broadened into this correction. No new
scripting commands or runtime dependencies. Next: independent re-review of the draft;
owner-run native acceptance follows only after that review.


## Final record-lifetime correction

Owner-assigned final P2 correction after independent re-review of `5edde916be1e08be7a7e01e4236187a6b20a954d`.
The live PR head matched, the existing worktree was clean, and all four reviewed-head CI
checks were successful. No other implementation or polish is included.

`ScriptRunner::start()` now puts construction/emission of the initial `run` record in an
explicit inner scope. Its roughly 4-KiB buffer is no longer a live C++ object when the
oversized-source path calls `error()`. Initial run reporting, schema, order, counters,
source ownership and error/write-failure propagation are unchanged. The native adapter's
existing pre-read 16-KiB rejection remains unchanged; this was not a demonstrated native fault.

The focused host regression now checks oversized-source `run` first, the synthetic
sequence/line 0 resource-limit error, final error record/counts, and zero extra C++ allocations.
It injects write failure at each of the three records, checking terminal error, exact error
counts and absence of a valid `end`. Existing JSONL tests remain unchanged.

### Static lifetime/frame comparison

Retro68 PowerPC GCC16.1.0, `-std=c++20 -Os -fno-exceptions -fno-rtti -fstack-usage`, same
include paths, separately compiled the saved `5edde91` source and corrected source:

```sh
/Retro68-build/toolchain/bin/powerpc-apple-macos-g++ -std=c++20 -Os \
  -fno-exceptions -fno-rtti -fstack-usage -Iinclude -Isrc/platform/common \
  -c /before.cpp -o build-os9-script/lifetime-before.o
/Retro68-build/toolchain/bin/powerpc-apple-macos-g++ -std=c++20 -Os \
  -fno-exceptions -fno-rtti -fstack-usage -Iinclude -Isrc/platform/common \
  -c src/platform/common/script.cpp -o build-os9-script/lifetime-after.o
```

`ScriptRunner::start()` reports **4208 bytes static before and after**. Lexical lifetimes
are now disjoint, but the compiler still reserves the same function frame. This does **not**
establish reduced physical nested stack use, native headroom or whole-stack safety. The
previously documented owner-run `StackSpace()` observation remains pending. No architecture
change was introduced to force a different compiler frame layout.

### Final verification and candidate

The normal host/sanitizer/probe/build/package commands above were rerun for this correction.
Focused ASan/UBSan/LeakSanitizer: **8/8 passed**; the existing JSONL and zero-allocation
checks remain passing. Full normal host suite: **72/72 passed** sequentially (117.61 s).
Portable Retro68 probe: **70/70 compile checks passed** (PowerPC and m68k subsets), with
no `FAIL` or compiler errors in its log. Changed C++ format, shared-source clang-tidy and
repository policy checks passed. New-head CI status is recorded at handoff.

Classic compile/Rez/PEF/MacBinary and HFS package checks passed with identical fork
round-trip. Data fork: **595,991 bytes**; MacBinary: **598,016 bytes**.
Specific local candidate (not transferred or executed):
`build-os9-script/PoodleEarth-pr222-stack-lifetime.bin` (Finder-named MacBinary), SHA-256
`cb43f100bcaae4fa8cb6429f35a3b2360f89fdb024ded4268560e33ac7ca7ba6`.
Build identity in this container remains explicitly unavailable because Git metadata lies
outside its mount. The source revision is the resulting commit recorded on the PR/issue;
CI results at that new head are separate from local evidence. No G4/emulator, transfer,
milestone/release operation or merge occurred. Next: Claude's review of the resulting commit,
then only separately owner-authorized limited native acceptance using the documented procedure.
