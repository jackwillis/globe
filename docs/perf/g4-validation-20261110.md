# Renderer correctness and G4 measurement review — 2026-11-10

Scope: #74 correctness first, individual review of #69/#70/#71, portable measurement infrastructure
for #64/#97, and an owner-executable NetBSD workflow. No optimization merged. Direct NetBSD hardware measurements were subsequently authorized; results below distinguish them from OS9 acceptance.
This report distinguishes inspected code, new host tests, historical hardware measurements and
unexecuted target work. The [benchmark procedure](../g4-benchmark.md) is the reproducible handoff.

## Repository and CI checkpoint

Initial fetched main: `a0e01f3be96f6556502d62d51320a0e9c1ccc75d` (also the owner's local main at inspection).
During this session main advanced to `e5c2af53b0b8cf245fcf3a9d7eeb1ecb4594093e` (#98, OS9 grow/startup handling),
then `84b74347fd73166548bb87c7c4f57c92a4b2f2f5` (#93, English-first UI/strings/overlays).
The measured rasterizer, terrain and refinement sources are unchanged by those main updates.
Measurements and host suite results below remain explicitly tied to the initial snapshot, not a claim
that every later UI revision was rebuilt/tested in this session. Before any
checkout mutation, all nine listed worktrees were inspected. Main contains pre-existing uncommitted
changes to `include/globe/render.hpp` and `src/render/render.cpp` (typed phases), plus untracked
`AGENTS.md`; these were preserved. Other accessible worktrees were clean. Country-selection has two
unique research commits; the other worktree heads have no commits outside fetched main's ancestry.
Work here uses the isolated `codex/g4-render-validation` worktree, not the owner's main/UI trees.

[Main CI run 42759651091](https://github.com/jackwillis/globe/actions/runs/42759651091) is successful:
**three jobs actually executed** (GCC sanitizer/lint/tests, clang build/tests, Retro68 probes and OS 9
build/package). All steps succeeded. The preceding `6215ecb` run is also successful. The inspected
older `13d27cc` run had three jobs, with tests passing and clang-tidy failing; that is a lint failure,
not a zero-job workflow startup failure. A historical zero-job failure is not evidence that today's
tests failed. Target build/package success does not mean the application ran on hardware.
Later main `e5c2af5` CI [42933961193](https://github.com/jackwillis/globe/actions/runs/42933961193)
also completed successfully; `84b7434` run 42934737682 was still running at the subsequent check. Main advanced again to
`8144472f48ab9e3cbf0c1117cb5f365dc291800d` while this report was being prepared; its run 42935775113
was in progress at publication preparation. The initial green baseline is independently verified;
these later running workflows are not reported as passed.

Open PRs at initial inspection: #69 raster, #70 pass lists, #71 ocean skip (base #70), #76 hygiene/release tooling,
#93 English-first UI, #94 atlas research, #96 roadmap/typography. #93 subsequently merged during the session. #76's head is already an ancestor
of this main; its still-open PR is stale relative to that fact. No unrelated PR was altered by this session.

Builds inspected: root CMake C++20, no exceptions/RTTI, host/platform split, optional ASan/UBSan;
OS 9 CMake uses Retro68 `-Os`; historical NetBSD headless uses GCC 12.5 `-O2 -static`. This session's
host uses Fedora Linux x86-64, GCC 16.2.1, `-O2 -DNDEBUG`, clang-format 22.1.8. Actual compile commands
are preserved by the harness; no fast math, LTO, renderer order change or new pack is introduced.

## Verified correctness defect: tuned presentation skips rows

The current OS 9 `present_step()` reads `[first,end)`, copies those rows, calls `tune()` from elapsed
copy time, then calls `submitted()`. Previously `submitted()` recomputed `end()` using the **new**
row count. Growing a slice acknowledges rows that were never copied. Starting at 32 rows in a
480-row frame with elapsed time 0 ms repeatedly copies `[0,32)`, `[64,128)`, `[192,320)`,
`[448,480)`: **224 rows are never copied**, although the queue reports complete. Shrinkage can
recopy rows instead. The effect depends on copy time rather than camera geometry; omitted rows can
retain an earlier refinement snapshot and look like unfinished limb chunks.

The focused test mirrors the real copy/tune/acknowledge call order. It failed on original main
(1,164 failed assertions across 10 cases) and passes after pinning the current slice size until
acknowledgment. `rows_` continues adapting for the next slice; a separate `slice_rows_` preserves
the in-flight range. No change to renderer pixels, order, work budget, UI behavior or copy thresholds.
The helper gains one `int` field (4 bytes on PPC32; enclosing-object padding may absorb it).

This is a demonstrated bug and a **plausible explanation**, not hardware proof that every #74
symptom is fixed. [Latest owner report](https://github.com/jackwillis/globe/issues/74#issuecomment-6759514047)
is bare OS 9 `globe-tip13`, main `c89f20a`, a glitch in the final batch. Keep #74 open for that
platform's verification. #95 remains the P0 hardware acceptance gate.

Additional permanent coverage in `test_line_layer`: 12 deterministic varied views (poles, roll,
odd viewport dimensions, radii from small globes to zoomed clipping, off-screen focus, both looks),
3 budgets (1/7/256), both forced compositor fallback and ample line list: **72 preserved/interrupted
final-frame comparisons** against fresh lines-at-end jobs. Stale colored pixels must disappear,
invalidated jobs must not write, and coverage mode must be off after every continuation. All match.
This checks job orchestration and preservation; it shares the terrain rasterizer and is not an
independent geometric oracle or a test of QuickDraw. Existing canonical terrain tests remain intact.

## Individual optimization review

All three have merge base `59a732eaadd40efe98c73a555397514fd6f138c5` against inspected main. #69/#70
are one unique commit each, 18 main-side commits behind the branch point; #71 contains three unique
commits including #70. Read-only merge-tree inspection and independent patch application found **no
text conflicts** with committed main. This does not cover the owner's uncommitted typed-phase edits:
#70 and #71 overlap those exact renderer files and need deliberate reconciliation with their owner.

Each candidate was applied separately to a fresh snapshot of current main; #71 retained only its
existing #70 stack. The new preserved-frame test was copied into each snapshot. `line_layer`,
`terrain`, `shaded`, `elevation_packs` pass **4/4 per candidate**, with unchanged benchmark checksums
`9c714394f3f2bd41` (r220) / `e1e923186a7e528d` (r440). These are focused host checks, not full suites
or G4 acceptance for the candidates.

| PR | Diff / dependencies | Evidence and risk | Recommendation |
| --- | --- | --- | --- |
| [#69](https://github.com/jackwillis/globe/pull/69), `9b7ae8d53056fa95c631921cce58ecf78a3b5724` | `src/render/raster.cpp`, `tests/test_shaded.cpp`; independent of #70 | Finite validation chain preserves coordinate/value domains. Truncate/adjust helper bounds conversion before casting. Existing reference-function test covers 6,000 triangles, clips, invalid inputs and edge coordinates; direct helper exhaustive-bit coverage is absent. No persistent memory increase. Actual PPC GCC removes shaded-triangle ceil calls (below). | Favor integration first after normal review/current-main checks. Real G4 lines-first median improves 9–11%; no persistent heap cost. Do not merge automatically. |
| [#70](https://github.com/jackwillis/globe/pull/70), `59c058046736b445130ca4514e621fe79201760e` | `render.hpp`, `terrain.hpp`, both implementations, terrain tests, refinement/terrain docs; independent of #69 | Stable per-ring order and bands retained; new setup phase and cached limb flag affect traversal. Tests exercise list partition, scan fallback, progress, limb scanning budgets and final pixels. Extra list allocation can fail scene creation. Above 100,000 cells keeps old ring scans. | Revise/revalidate against current typed-phase WIP and target memory before integration. Real 1° G4 lines-first gain is 20–21%; fine-grid fallback benefit remains unmeasured. |
| [#71](https://github.com/jackwillis/globe/pull/71), `99edbae79082e211aa792455ee5b87776d9ff0a2` | Stacked on #70; adds Map skip guards, proposed ADR 0004, tests and host evidence | Skips only negative-height cells with four corners clear of horizon, matching cleared ocean color, smooth Map and fresh bitmap. Preserve/flat/Relief paths stay enabled. Existing 1,500-view equivalence/mutation tests plus new preservation tests pass. Intermediate frames/progress work differ. | Defer behind #70, #74 hardware check and owner's acceptance of the proposed ADR/intermediate behavior. Compare #70 vs #70+#71 to isolate its gain. |

Memory: #70 adds exactly `64,800 * 4 = 259,200` heap bytes per 1° grid (253.125 KiB), plus 132 bytes
of inline offset/fill arrays in `TerrainScene` and a list cursor in each job (host job 616→624 bytes; measured PPC job 544→552 bytes).
Existing `bytes()/required_bytes()` add the heap list; inline storage is outside that payload count.
No lists on 0.5° grids; this is not a universal fine-resolution speedup. #71 adds no second cell
list; it inherits #70's cost. Neither target RSS nor Classic partition headroom has been measured.

Prior host evidence (author's GCC 13 `-O2 -g`, whole-process callgrind, not reproduced here): baseline
607.6M instructions, #69 552.3M, #70 474.8M, #70+#71 349.9M. Those are instruction counts, not G4 time.
This session's exploratory 3-repeat host medians are noisy and were run with concurrent build load;
checksums/testing are useful evidence, timing is **not an A/B performance claim**. No combined
#69+#70+#71 integration branch was created.

## Actual PPC32 code generation (#97)

Cross compiler: `powerpc--netbsd-g++ (NetBSD nb3 20260326) 12.5.0`, actual translation unit
`src/render/raster.cpp`, `-std=c++20 -O2 -fno-exceptions -fno-rtti`, existing NetBSD 11 sysroot and
`usr/include/g++` headers. `powerpc--netbsd-objdump -dr` on main versus #69 objects shows:

- `fill_triangle_shaded`: **4 `R_PPC_REL24 ceil` relocations → 0**.
- Whole raster TU: 4 `__fixdfdi` and 2 `__divdi3` relocations remain in both versions; these are
  independent of the shaded ceil change. Do not silently optimize the line path from this count.
- Floating divides remain; no claim about cycles, cache effects or real wall time follows.

Unlike #97's earlier clang snippets, this is the real NetBSD toolchain and complete raster source.
Reproduce on the development host (set `XC` and `SYSROOT` to existing directories):

```sh
"$XC/powerpc--netbsd-g++" --sysroot="$SYSROOT" -isystem "$SYSROOT/usr/include/g++" \
  -std=c++20 -O2 -fno-exceptions -fno-rtti -Iinclude -c src/render/raster.cpp -o /tmp/raster-main.o
"$XC/powerpc--netbsd-objdump" -dr /tmp/raster-main.o > /tmp/raster-main.dis
```

Repeat against the actual #69 source under the same flags, with a distinct object name. Static
PowerPC/NetBSD binaries for all three harness targets also cross-link successfully. Their direct target execution is recorded in the hardware results below. Retro68 GCC's specific code generation remains a distinct follow-up. #97's speculative
line fast path, division dose-response and RGB555 changes were not implemented; the new correctness
finding has higher priority. Adaptive copy slices invalidate a simple fixed-32-row interpretation
of historical `88 bands`; the old copy-band count alone cannot establish memory bandwidth.

## Hardware baseline and unresolved boundaries

| Provenance | Verified historical result | Boundary |
| --- | --- | --- |
| NetBSD/macppc G4, 2026-11-07 update, headless Release, 640x480, median of 5, Map layer | r220 settle 539.0 ms, first lines 52.7 ms, worst step 1.1 ms; r440 536.9 / 53.7 / 1.9 ms; matching checksums | `docs/perf/g4-2026-11-06.txt`; historical scratch build, not today's SHA or real X11 presentation |
| Bare OS 9 G4, 2026-11-07 compact edition | Map ~991 ms settle, ~150 ms copying, max turn 20 ms, max step 9 ms; Relief ~992 / 150 / 19 / 8 ms | `docs/os9-proof.md`; presentation-slice bug makes title completion insufficient to prove every row was copied |
| Tiger Classic, earlier recorded run | ~2,750 ms settle, ~734 ms copying | Different environment/workload; never substitute for bare OS 9 acceptance |

Existing `globe-refine-profile` decomposes an older fill algorithm, not today's phase sequence.
Existing line-layer benchmark measures first CPU-ready band/settled frame with monotonic clocks,
but previously only emitted medians and worst step. The new wrapper preserves per-process raw
outputs, summarizes distributions, records compiler/SHA/data/binary hashes and per-child RSS.
Initialization is embedded parsing/scene construction, **not external-pack cold loading**. Full X11
presentation, OS 9 conversion/CopyBits split, current-job phase times and real input latency remain
unmeasured. Direct current-renderer G4 results are reported in the added hardware section below.

## Direct G4 measurements — NetBSD 11.1/macppc

After the owner explicitly authorized direct SSH/transfer/benchmarking, the existing SSH jump-host
helper was used. Verified: NetBSD **11.1**, one 7400 rev 2.9, 268,435,456 bytes RAM; 467 MHz is the
owner's hardware specification. Existing target compiler is GCC 12.5; existing Python is 3.13.
No competing build or benchmark appeared in before/after process inventories. X11, xterm, conky,
window manager and htop remained running; nothing was killed or reconfigured. All source builds
were performed on the development host, then transferred to a unique temporary directory.

Raw machine-readable evidence: [g4-20261110-optimization-ab.json](g4-20261110-optimization-ab.json).
It records full source recipes, flags, binary/data hashes, each process's original benchmark JSON,
monotonic wall/CPU timings, and RSS limitations. All builds use identical NetBSD GCC 12.5,
`-O2 -DNDEBUG -static`, embedded atlas/1° elevation/land classes, 640x480, fixed lat 30/lon 10,
radii 220/440, default 256-work-unit continuation. No CPU tuning, LTO or combined #69+#70 build.

Run order: baseline / #69 / #70 / #70+#71 / #70+#71 / #70 / #69 / baseline. Each block discards one
warmup process and measures nine fresh processes. Each process renders all four cases once. The
following is **pooled median / nearest-rank p95 in milliseconds, 18 processes per cell** (with n=18,
p95 is the maximum). No outlier is discarded:

| Build | r220 unpaced | r220 lines first | r440 unpaced | r440 lines first |
| --- | ---: | ---: | ---: | ---: |
| baseline | 538.3 / 753.3 | 556.0 / 653.7 | 524.6 / 652.4 | 553.3 / 741.0 |
| pr69 | 488.6 / 760.3 | 503.7 / 647.9 | 457.7 / 641.4 | 490.2 / 628.6 |
| pr70 | 425.3 / 657.4 | 445.0 / 496.5 | 407.5 / 536.4 | 435.7 / 721.9 |
| pr71 | 290.3 / 464.4 | 282.2 / 551.4 | 266.3 / 360.6 | 291.5 / 299.5 |

For the normal lines-first path:

- #69 reduces median settle by **9.4% at r220, 11.4% at r440**. Bookend block medians are stable
  (r220 503.7/503.4 ms versus baseline 552.9/559.7 ms). Actual hardware supports the ceil-helper win;
  it is smaller than some host wall-time projections and does not improve first-line latency.
- #70 alone reduces median by **20.0% / 21.2%**, saving about **111 / 118 ms**. This exceeds #66's
  earlier 60–100 ms projection for this workload. It still adds list memory and does not accelerate
  the 0.5° ring scan fallback in the same way. First-line medians stay ~56 / 55 ms because lines
  are drawn before terrain setup/traversal.
- #70+#71 reduces pooled median by **49.3% / 47.3%** versus baseline; its incremental reduction versus
  #70 is **36.6% / 33.1%**. **The r220 block is noisy:** lines-first block medians are 342.0 and
  281.6 ms (roughly 38–49% below baseline), and unpaced medians 263.8 and 391.5 ms. Do not treat the
  pooled r220 point estimate as a precision forecast. The r440 layer block medians agree at
  291.6/291.2 ms. Whole-process user CPU medians agree at ~1,374.7/1,374.9 ms for the two #71 blocks,
  supporting lower compute work despite wall-time outliers.

Every measured case on every candidate has the matched final hash `9c714394f3f2bd41` /
`e1e923186a7e528d`. This is evidence for these views, complemented by the host regression tests,
not an exhaustive visual proof. The large p95 tails and active desktop rule out a hard event-loop
latency guarantee. The helper regression also cross-compiles and **runs successfully on this G4**
(`ok`, exit 0), proving portable PPC logic; it does not execute Toolbox copies on OS9.

Python-parent `wait4` RSS is ~20 MiB for all variants and slowly rises across runs. It includes an
inherited pre-exec interpreter high-water floor on this environment, so it **cannot validate** the
259,200-byte list delta. Keep it as recorded OS accounting, not a target memory saving claim.
A separate matched native launch (`/usr/bin/time -l ... --repeat 9`, one process per variant) avoids
the Python-parent floor. It reports peak RSS **6,396 KiB baseline, 6,392 #69, 6,552 #70, 6,516 #70+#71**,
zero swaps and zero hard page faults. This supports modest resident growth, **not** a smaller heap
allocation: the 259,200-byte list capacity includes pages that need not all be touched (only visible
cells are placed). OS RSS/allocator/page granularity cannot replace the payload ledger or the OS9
partition measurement. Native lines-first medians corroborate the speed result:

| Native repeated-process check | r220 layer (ms) | r440 layer (ms) |
| --- | ---: | ---: |
| baseline | 555.2 | 547.3 |
| #69 | 505.3 | 486.3 |
| #70 | 439.0 | 433.1 |
| #70+#71 | 278.1 | 290.8 |

These nine-internal-repeat medians include the first iteration; they are **not pooled** into the
fresh-process table above. They independently support the large #71 reduction despite the earlier
r220 outliers. Raw native output and resource counters are in the same JSON artifact.

Baseline nine-process drag/initialization/settle workload (one discarded warmup each):

- Embedded atlas parse median **0.756 ms**, Scene construction **15.29 ms** in drag runs. These
  exclude external-pack I/O and do not measure cold disk startup.
- First frame since main in drag runs: median **335.74 ms**, maximum **744.10 ms**, including
  initialization and the first CPU render, but no window presentation. The constructor shares are
  not separately instrumented; profile terrain-cache initialization before proposing a startup fix.
- Drag trace (374 frames/process): median of per-process frame p50s **18.64 ms**; median of frame p95s **21.48 ms**,
  worst process's p95 **25.51 ms**. No packing/submission or real event-dispatch latency included.
- Model-driven final settle: median **551.50 ms**, nearest-rank p95 **727.84 ms**; first CPU-ready
  band median **50.60 ms**; median worst continuation **1.10 ms**, but worst observed continuation
  **170.91 ms wall time**. That outlier is not diagnosed; bounded work units do not guarantee
  wall-time deadlines on a loaded/preempted system. Do not promote the median to a responsiveness
  upper bound. Full raw per-frame summaries and process samples are preserved.

All measurement processes completed, and final process inventories contain no remaining Globe
benchmark/build process. Scratch artifacts remain in the distinct temporary experiment directory;
no installed application, display permission, system setting or other session's work was changed.

## Validation and handoff

Commands actually run on Fedora Linux x86-64, GCC 16.2.1:

- `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE='-O2 -DNDEBUG'`, `cmake --build build -j4`, `ctest --test-dir build --output-on-failure`: **60/60 pass** (serial final run, private Xvfb outside sandbox, 45.53 s). Initial sandbox display failures were environmental; one concurrent Xvfb pixel-readback failure passed alone and in the serial full run.
- `cmake -S . -B build-sanitize -G Ninja -DGLOBE_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_DISABLE_FIND_PACKAGE_X11=ON`; build `test_classic_present test_line_layer test_terrain test_zoom`; `ctest --test-dir build-sanitize -R '^(classic_present|line_layer|terrain|zoom)$' --output-on-failure`: **4/4 pass**, ASan/UBSan + LeakSanitizer outside tracing sandbox (40.06 s).
- `clang-format --dry-run --Werror src/platform/common/classic_present.hpp tests/test_classic_present.cpp tests/test_line_layer.cpp` (22.1.8), `tools/check-policy.sh`, `git diff --check`: pass.
- `python3 tests/test_g4_bench.py`: offline provenance/sample/overwrite/corruption/unstable-checksum contracts pass. Actual host bundle prepare/run smoke covered all four workloads (two measured processes each plus warmups).
- Actual NetBSD GCC 12.5 cross compile/link of all three benchmark targets with the supplied toolchain and static flags: pass. Independent candidate tests and PPC disassembly are detailed above.
- Development-host cached Retro68 container: `cmake -S os9 -B build-os9-validation -DCMAKE_TOOLCHAIN_FILE=/Retro68-build/toolchain/powerpc-apple-macos/cmake/retroppc.toolchain.cmake`, `cmake --build build-os9-validation -j2`: application links and MacBinary/application/disk outputs generated. **No OS9 execution** or archive/installer acceptance claimed.

The regression was demonstrated before the fix. No golden was edited. The owner's renderer changes and all other branches remain
untouched.

Recommended order:

1. Review the presentation fix and new tests; Claude/owner verifies the exact #74 final-batch view
   on bare OS 9, with unique build name/SHA, display depth and copy timing. Record whether a redraw
   repairs old patches, whether all rows arrive, and whether completion/title/progress agree.
2. Preserve the completed NetBSD raw baseline/candidate evidence. Repeat quieter matched runs if
   tighter tail estimates or the noisy r220 #71 case matter; coordinate with other sessions.
3. Use the measured #69 A/B to support normal integration review; follow with #70 memory
   ledger/partition review, then #71 intermediate visual acceptance. Reconcile typed phases before touching the same renderer files.
4. Keep #95 open until the full owner checklist passes or has explicit waivers. #68 flags and #97
   additional optimizations remain separate measured follow-ups. No UI/data/release scope expansion.
