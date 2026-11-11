# Scene payload accounting checkpoint — 2026-11-09

Issues: #55, #59 (under #54). Implementation: `4a30d84`.

## Delivered boundary

`Scene::create(atlas, ledger)` optionally counts the two allocations for cached world/graticule
vectors and polyline caps. Tickets follow ownership through construction, optional return,
move construction, same-/cross-ledger assignment and destruction. A failed second allocation
records the first allocation's temporary peak, releases it and leaves an existing scene usable.
Move assignment frees replaced payload before releasing its tickets. No renderer algorithm,
production data, cache guard, budget policy or platform frontend was changed.

The borrowed ledger must remain at a stable address and outlive its scenes. Source atlas bytes
still must outlive their views. Existing callers can omit the ledger. The extra two inline tickets
make `sizeof(Scene)` 96 bytes on this Linux x86-64 build, including when accounting is omitted;
that is metadata, not counted dynamic payload. Target inline size was not measured this session.

`globe-bench` now owns one ledger for its scene and primary framebuffer. JSON `memory_payload`
reports byte units, current/peak and success/failure counts per category, simultaneous total peak,
and a sticky accounting-error flag. Its explicit scope is `scene_and_primary_framebuffer`.
Terrain/class caches, owned packs, jobs (including line buffers), host containers, embedded/static
and inline storage, allocator overhead and RSS are excluded. Existing `scene_bytes`, terrain,
framebuffer and RSS fields retain their previous meanings. The reference renderer still constructs
a scene in this tool; its counted residency remains real even when that scene is not used to draw.
No app-wide memory or OS 9 partition-headroom claim follows from this report.

## Reproduction and evidence

Host: Fedora Linux x86-64, kernel `7.2.10-200.fc44.x86_64`; GCC 16.2.1, Clang/clang-format/
clang-tidy 22.1.8. Default CMake build (no optimization requested). No hardware accessed.

```sh
cmake -S . -B build -G Ninja
cmake --build build -j 4
ctest --test-dir build --output-on-failure -j 4
ctest --test-dir build --rerun-failed --output-on-failure
build/globe-bench --size 80x60 --repeat 1 tests/data/trace1.txt

cmake -S . -B build-sanitize -G Ninja -DGLOBE_SANITIZE=ON
cmake --build build-sanitize -j 4
ctest --test-dir build-sanitize --output-on-failure -j 4
ctest --test-dir build-sanitize -R 'scene_ledger|bench_memory' --output-on-failure

cmake -S . -B build-clang -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang -j 4
ctest --test-dir build-clang -R 'scene_ledger|render|line_layer|terrain|memory_ledger|framebuffer_ledger|bench_memory' --output-on-failure
ctest --test-dir build-clang -R 'scene_ledger|bench_memory' --output-on-failure

clang-format --dry-run --Werror include/globe/render.hpp src/render/render.cpp src/platform/bench/bench.cpp tests/test_scene_ledger.cpp
clang-tidy -p build --quiet src/render/render.cpp src/platform/bench/bench.cpp
sh tools/check-policy.sh
git diff --check
tools/probe-retro68.sh
```

- Normal build: 49/55 passed in the parallel run, then all six failed X11 checks passed serially
  (16/24-bit pixel readback, toolbar, repaint, click, zoom). Earlier sandbox-only runs could not
  access private X displays. Tests with display access ran outside that restriction. This is
  recorded as a passing combined result, not a clean single-run 55/55 claim.
- ASan/UBSan: all 55/55 passed with leak detection enabled outside sandbox tracing. An earlier
  sandbox run hit LeakSanitizer's ptrace restriction. Final affected tests passed again after
  the final fixture/report edits. No leak suppression was needed in the successful run.
- Clang: focused 7/7 passed; final affected 2/2 passed after rebuilding the final edits.
- Formatting, changed-source lint, repository policy and whitespace checks passed.
- Retro68: all portable probe translation units compiled for PowerPC and m68k, with no `FAIL`
  entries. This is compile evidence, not target execution or 68k runtime support.
- Existing render goldens, continuation-budget and terrain/line tests passed without pin edits.
  The new scene test also compares complete pixels with an uninstrumented scene and observes
  zero allocation calls across repeated cached rendering, budget-7 continuation steps and
  cancellation. This new allocation test covers the line-only job; terrain job allocation
  behavior remains outside this slice.
- Independent allocation hooks verify both failure points, cleanup, retained-scene behavior,
  overlapping replacement peaks, self-/cross-ledger moves and transfer of the source atlas view.

For the bundled atlas at 80×60, the report counted 241,912 scene bytes and 10,200 framebuffer
bytes (9,600 pixels + 600 coverage), total current/peak 252,112 bytes, two successful allocations
per category and zero failures. These are host requested allocation payloads, not process memory.
The small fixture independently observes 32,160 vector bytes + 32 cap bytes = 32,192 bytes.

## Integration and next handoff

Work is isolated on `codex/scene-memory-ledger` in `/tmp/globe-scene-memory`, based on remote main
`96bf8c0`. Owner-authorized scoped branch publication is permitted. Local main was at `06f0482`
with pre-existing render header/implementation changes and an untracked `AGENTS.md`; it was not
checked out, staged or merged. Other existing worktrees and performance branches remain intact.
No remote main push, release, hardware activity, production-pack change or Claude PR merge.

#55/#59 remain open: this is one allocation category plus a scoped host report. Next bounded tasks:

1. Add TerrainScene allocation tickets and failure-at-each-allocation tests, preserving its guard;
   coordinate with performance pass-list branches because they change terrain storage.
2. Establish stable frontend ledger owners and propagate accounting to line/job allocations,
   including cancellation and resource replacement; keep excluded categories explicit.
3. Have Claude measure target payload/inline cost and partition headroom after integration;
   do not infer target responsiveness or memory headroom from this host run.
