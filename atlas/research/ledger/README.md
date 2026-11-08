# Standalone memory-accounting primitive (#55/#17)

`include/globe/memory_ledger.hpp` implements the first slice proposed in
`atlas/reports/memory-ledger-design.md`: fixed-storage category counters, simultaneous total peak,
checked byte arithmetic, failure events, snapshots, peak-window reset and move-only accounting
ownership. `MemoryCharge` owns an accounting entry, **not** the allocation itself. The ledger must
outlive every ticket and remain at a stable address. One event-loop owner; no synchronization.

`Framebuffer::create(width, height, ledger, category)` now optionally reports its pixel and coverage
allocations and failures. Existing two-argument callers remain uninstrumented. Charges follow the
framebuffer's moves/lifetime, with no extra heap allocation or drawing-time allocation. No platform
adapter, renderer algorithm, pack format, cache guard or root build file is changed. This standalone
CMake project registers two tests without editing Claude's shared build integration; root Globe
CTest does not yet register these new tests.

```sh
cmake -S atlas/research/ledger -B /tmp/globe-ledger-build -G Ninja
cmake --build /tmp/globe-ledger-build
ctest --test-dir /tmp/globe-ledger-build --output-on-failure
```

Use `-DCMAKE_CXX_COMPILER=clang++` in a separate build directory for Clang. Use
`-DLEDGER_SANITIZE=ON` for an isolated ASan/UBSan build. On a sandbox tracing processes,
LeakSanitizer can fail under ptrace; `ASAN_OPTIONS=detect_leaks=0 ctest ...` permits the separate
ASan/UBSan checks, and must be reported as leak detection disabled.

The primitive C++ test verifies overlapping/disjoint category peaks, reset while live, zero-byte events,
partial construction failure, correct event counts, invalid categories, underflow, byte overflow,
move construction/assignment/self-move, cross-ledger moves and idempotent release. It compares
10,000 interleaved operations with a separate fixed-array reference model. Independent global
new/new[] hooks are exercised first, then observe **zero allocation calls** across all tested ledger
operations/ticket lifetimes/snapshots. No latency or G4 performance claim follows from that test.

Errors are sticky and preserve numeric counters for the failing event; subsequent valid events
may still be counted, but diagnostics must mark the whole measurement invalid. Raw release calls
require matching ownership; aggregate counters cannot detect a wrong individual allocation size
when it fits the category total. Prefer tickets at actual allocation owners. A successful
zero-byte acquisition increments success events; zero-byte release changes no bytes. Counters
count requested payload, excluding allocator metadata, RSS, inline arrays and embedded resources.
Failure event-counter overflow is checked in code; exhausting uint64 event counters is not exercised
by the bounded tests. Checked allocation-size multiplication belongs at future allocation sites.

The framebuffer test independently observes exact requested allocations, injects failure at each
of the two allocations, verifies partial cleanup and old-buffer retention during replacement failure,
checks same-/cross-ledger moves, compares pixels with an uninstrumented buffer, and observes no
allocation during 10,000 drawing/measurement iterations. A failing accounting charge flags the
ledger without changing allocation success. The factory counts both arrays; the framebuffer object
itself and ledger/ticket inline storage are separate metadata costs.

| ABI / evidence | Ledger | Ticket | Framebuffer before → after | Pointer |
| --- | ---: | ---: | ---: | ---: |
| Linux x86-64 / executed sizeof | 248 | 24 | 40 → 88 | 8 |
| Retro68 PowerPC GCC 16.1.0 / assembly constants | 244 | 16 | 28 → 60 | 4 |
| Retro68 m68k GCC 16.1.0 / assembly constants | 242 | 14 | 26 → 54 | 4 |

Values are bytes of inline object storage. `abi_probe.cpp` exposes compile-time constants for
assembly inspection (compile with `-std=c++20 -Iinclude -S`; compare the old framebuffer header at
base `52ccb33` for the baseline). These are not allocator overhead/partition measurements. Both
Retro68 compilers also accept the final framebuffer/ledger test translation units and framebuffer
implementation offline. No target executable or hardware was run; 68k runtime support is not claimed.
The standalone framebuffer target suppresses preexisting signedness warnings in drawing code;
the primitive retains strict conversion warnings. No unrelated drawing-loop cleanup was made.

Next scoped step: coordinate root-build registration and a stable ledger owner in each frontend,
then instrument a scene constructor with owner tickets and failure injection. The framebuffer
capability is present but no frontend passes a ledger yet. Keep current guards until
all relevant allocations are counted and estimates reconciled. The primitive alone does not
complete #55/#59, and supplies no degradation (#56), eviction (#57) or CI budget (#60).
