# Measured renderer integration and startup vectors — 2026-11-10

This checkpoint integrates the measured candidates without discarding the owner's typed-phase work,
corrects the independent OS 9 sizing review, and measures a separate startup improvement. Raw source
recipes, compiler flags, data/binary hashes and every new measured sample are in
[g4-integration-20261110.json](g4-integration-20261110.json). The earlier baseline and independent
candidate experiment remain in [the preceding report](g4-validation-20261110.md).

## Integration and review decisions

- **#101 review:** already merged as `64a0edb` when inspected. The 11 B/pixel expression charged the
  fixed sidebar, so a 628x428 window with an exactly fitting 408x428 globe shrank to 480x327. Filed
  #106 and coordinated the narrow correction with Claude via #101/#106; #107 fixes it, adds portable
  budget/minimum/odd/max/monotonic tests, and corrects the memory-model table. Installed Retro68
  malloc references NewPtr. FreeMem is a conservative current-zone total-free-byte hint, not a
  contiguous allocation guarantee or held reserve. The model includes all four pixel framebuffers,
  GWorld and line list, but not every old+new resize peak. No overflow/nontermination was found in
  the original bounded dimensions. The 7/8 shrink policy remains intentionally coarse. Bare OS 9
  partition/fragmentation/resize acceptance stays in #10/#95.
- **#100:** Claude independently reviewed and merged it as `beb12cb`; this session did not self-merge
  it. The presentation-slice regression remains part of the suite; bare OS 9 #74 confirmation is open.
- **#69:** refreshed normally against main, reviewed and merged with history preserved as `068e351`.
  Fresh PR CI passed all 3 jobs; local suite 59/59 on its refreshed head. Current main including #100
  was independently green before merge; post-merge main run 42938211291 passed.
- **#70/#104:** original #70 commit retained by ancestry, then the owner's typed-phase patch was
  applied in an isolated branch. One BoundCells/list-setup conflict was resolved by retaining both
  behaviors and naming the added phase `BuildPassLists` (value 13). No other phase order/value or
  four-fill/16-row bound changed. #104 merged as `8528abd`; #70 was marked merged by ancestry.
  Full suite 60/60, fresh 3-job CI and a new G4 A/B passed; main run 42939097605 passed.
- **#107:** merged as `f2baa56`, following 61/61 local tests, mutation detection, Retro68 build and
  fresh 3-job CI. Its main run was cancelled by Claude's unrelated #108 key fix; inclusive main
  `96f3222` run 42940183942 then passed. Cancelled runs are not described as passing tests.
- **#71:** retargeted to main and refreshed normally, first to `8ab1b74` and then `df57dc2` including
  #107. Full final host suite 61/61 and all 3 fresh CI jobs passed. G4 renderer-source revalidation is
  below. Merged as `1f4a818`; post-merge main run 42940777120 passed all 3 jobs.

The original owner diff in `/home/jack/code/globe` remains unmodified and unstaged, SHA256
`3dec60be5c86a925ee26fcae85b2ee989f95df2df07d768e5b4434fc4ad38bfc`. Its contents are now represented
in merged code, with the new list phase reconciled. Do not blindly update that dirty local main.
No force push, release, destructive cleanup or other-session process termination occurred.

## New G4 stage comparisons

Target verified before timing: NetBSD 11.1/macppc, one 7400 rev 2.9, 256 MiB; 467 MHz is the owner's
hardware specification. Existing GCC 12.5 cross-toolchain/sysroot, `-O2 -DNDEBUG -static`, same
embedded atlas/elevation/land classes, 640x480, lat 30/lon 10, radii 220/440. Development-host builds,
isolated target directories, no concurrent benchmark/build found in process inventories. Desktop
and htop remained running. No display configuration, installed application or boot change.

Each stage is A/B/B/A, one discarded warmup and 9 fresh-process measurements per block, 18 measured
processes per variant. Numbers below are lines-first settle medians; p95 for n=18 is the maximum,
and every outlier is retained in raw JSON. The stage baselines are separate observations, not pooled.

| Stage / exact renderer sources | r220 baseline → candidate | r440 baseline → candidate |
| --- | ---: | ---: |
| #69 main `068e351` → typed lists `a6cc799` | 505.27 → 396.44 ms (−21.5%) | 487.55 → 371.88 ms (−23.7%) |
| typed lists `a6cc799` → ocean skip `8ab1b74` | 391.51 → 259.70 ms (−33.7%) | 377.17 → 261.66 ms (−30.6%) |

First-line medians stay about 55–56 ms. All 288 measured case hashes in these two stages match
`9c714394f3f2bd41` / `e1e923186a7e528d`. The combined result is about 53% below the earlier
same-hardware baseline 556/553 ms, but that historical comparison is not a new paired baseline run.
The individual matched stage comparisons above are the direct integration evidence.

#71 adds no second cell workspace; #70's 259200-byte 1° list allocation remains. Python-parent wait4
RSS has a pre-exec floor and is not used to infer small capacity differences. Prior native-launcher
RSS evidence remains distinct from payload accounting and from Classic partition requirements.

#71's intermediate work/presentation cadence can differ: fresh Map ocean already exists before
land fills; clipped repair remains ordered and preserved/flat/Relief paths do not skip. An added
per-presentation line invariant checks that every coast/graticule pixel shown in the first normal
lines-first frame remains at every later presented pass. It passes alongside 1500 skip-equivalence
views and 72 preserved/interrupted compositor comparisons. Final-pixel equivalence is not a claim
that every intermediate frame or work count is identical. The equivalence/guard decision is now
accepted by integration; real X11/OS 9 visual latency remains separate.

One new #71 r440 sample took 1019 ms wall time despite a 262 ms median. Bounded work units are not a
hard wall-time guarantee under preemption/load. No attribution to a particular background process
is proven, and no noisy sample was deleted.

## Separate startup improvement: reuse row/longitude factors

`TerrainScene::create` previously called `to_unit_vector` for every grid corner, repeating latitude
and longitude trigonometry. The new private initializer calls the same conversion once per column
at latitude 0 and once per row at longitude 0, then uses the same final products. The unpublished
first world-vector row temporarily stores longitude factors and is overwritten **last**. There is
no new allocation, persistent table, public API, clock read, runtime data format or frame-loop work.
It replaces O(width*height) transcendental calls with O(width+height); vector writes remain linear.

The scalar old loop is the independent oracle. Tests compare all three component bits for 360025
corners across six grids (1x1, small global, odd regional, poles/dateline, 1° and 0.5°), including signed
zero. Host and actual PPC benchmark checks pass. Deliberately corrupting a factor is detected at
1x1. Host/G4 component hashes can differ with libm implementations; candidate/reference match
exactly **within each platform**, preserving the pre-existing behavior. Existing image pins are
untouched, and the new line invariant also guards intermediate composition.

The reproducible `globe-grid-vector-bench` validates those grids before timing, uses two allocated
output arrays, reserves sample storage outside timing, and emits JSON after timed work. Nine
samples plus one warmup, CLOCK_MONOTONIC, old scalar loop against the production helper:

| Physical G4 kernel | Old scalar median | Cached factors median |
| --- | ---: | ---: |
| 360x180 cells | 97.92 ms | 3.41 ms |
| 720x360 cells | 395.51 ms | 14.77 ms |

These are vector construction, **not full startup**. End-to-end confirmation used exact post-#71
main `1f4a818aa898744a1981e86dd65173a9229581d7` versus implementation commit
`74cca1d8ed4bf345d08a2df70eb5bb33cdbbb745`, same flags/data and a separate A/B/B/A. Each block runs
`globe-bench --repeat 1 --size 640x480 --refine settle.txt` (the actual command array is in raw JSON),
with 9 fresh processes after a warmup. Pooled medians over 18 samples per build:

| CPU-side metric | Baseline | Cached initializer |
| --- | ---: | ---: |
| First frame since main | 327.20 ms | 181.00 ms |
| Whole process | 597.45 ms | 451.39 ms |
| Atlas Scene construction | 15.24 ms | 15.20 ms |
| Final refinement | 240.72 ms | 244.82 ms |

First CPU frame improves about 44.7% in this workload. No settle/drag throughput gain is claimed;
refinement variation is small relative to the noisy tails. All 36 final-frame tags match
`ca536f8b7e6ec99b`. Cache state is uncontrolled/warmed as recorded, not cold disk loading. No X11
submission, OS 9 launch/display time or 0.5° whole-application startup was measured.

## Validation / reproduction

On Fedora Linux x86-64, GCC 16.2.1:

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS_RELEASE='-O2 -DNDEBUG'
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/globe-grid-vector-bench
```

Full suite 61/61; focused ASan/UBSan/LeakSanitizer terrain+line_layer 2/2 (144.05 s), and clang-format
22.1.8, targeted clang-tidy, policy and diff checks pass. The cached Retro68 PowerPC application
build links/generates outputs; this is not OS 9 execution. NetBSD GCC 12.5 cross-builds and actual G4
production-helper bit checks pass. No golden or published geographic bytes changed.

Use [the bundle procedure](../g4-benchmark.md) to rebuild/run matched baselines. The additional
kernel target is independent of embedded data:

```sh
cmake --build build-g4 --target globe-grid-vector-bench
# On the authorized target, from the distinctive transferred directory:
./globe-grid-vector-bench > grid-vectors-new-run.json
```

All timed target processes finished; scratch artifacts remain isolated. Next acceptance is still
bare OS 9 #74 and #10/#95 (all rows copied, memory partition, growth/failure behavior and actual
presentation). #68's compiler flags remain unmeasured on bare OS 9. Do not infer those from these
NetBSD CPU-side results or from the OS 9 cross-build.
