# Standalone memory-accounting primitive (#55/#17)

`include/globe/memory_ledger.hpp` implements the first slice proposed in
`atlas/reports/memory-ledger-design.md`: fixed-storage category counters, simultaneous total peak,
checked byte arithmetic, failure events, snapshots, peak-window reset and move-only accounting
ownership. `MemoryCharge` owns an accounting entry, **not** the allocation itself. The ledger must
outlive every ticket and remain at a stable address. One event-loop owner; no synchronization.

No allocation site, renderer, pack format, cache guard, platform adapter or main build file is
changed. This standalone CMake project gives the primitive a reproducible test without editing
Claude's shared build integration. Root Globe CTest does not yet register this new test.

```sh
cmake -S atlas/research/ledger -B /tmp/globe-ledger-build -G Ninja
cmake --build /tmp/globe-ledger-build
ctest --test-dir /tmp/globe-ledger-build --output-on-failure
```

Use `-DCMAKE_CXX_COMPILER=clang++` in a separate build directory for Clang. Use
`-DLEDGER_SANITIZE=ON` for an isolated ASan/UBSan build. On a sandbox tracing processes,
LeakSanitizer can fail under ptrace; `ASAN_OPTIONS=detect_leaks=0 ctest ...` permits the separate
ASan/UBSan checks, and must be reported as leak detection disabled.

The C++ test verifies overlapping/disjoint category peaks, reset while live, zero-byte events,
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

On the tested Linux x86-64 ABI the ledger is 248 inline bytes and a ticket is 24 inline bytes.
Those are object-size measurements, not added heap allocations or verified Classic sizes.
Both PowerPC and m68k Retro68 compilers accept the final header/test translation unit in offline
compile-only checks. No cross-target test executable or hardware was run; 68k runtime support is
not claimed.

Next scoped step: coordinate root-build registration and instrument framebuffer pixels/coverage
plus one scene constructor, with owner tickets and failure injection. Keep current guards until
all relevant allocations are counted and estimates reconciled. The primitive alone does not
complete #55/#59, and supplies no degradation (#56), eviction (#57) or CI budget (#60).
