# Opt-in NetBSD/macppc benchmark workflow

This workflow builds on the development host and runs offline on the owner's existing NetBSD
installation. The harness never connects to hardware, installs packages, changes configuration,
flushes caches, or builds on the G4. Coordinate timing runs with other authorized hardware sessions.
The owner authorized direct NetBSD SSH/transfer/measurement during this session; the default here
continues to cross-build on the development host to minimize target disturbance. The runner needs an **existing Python 3.9+** with its standard library. If unavailable, use
the direct-binary fallback below; do not install it as part of a benchmark.

## Build on the development host

Use a clean committed checkout and a fresh build directory for each candidate. Preserve other
worktrees. The supplied toolchain file expects an existing NetBSD cross GCC and matching sysroot:

```sh
export GLOBE_NETBSD_TOOLS=/absolute/path/to/existing/tools/bin
export GLOBE_NETBSD_SYSROOT=/absolute/path/to/existing/sysroot
cmake -S . -B build-g4-baseline -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=tools/netbsd-g4-toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE='-O2 -DNDEBUG' \
  -DGLOBE_BUILD_TESTS=OFF -DCMAKE_DISABLE_FIND_PACKAGE_X11=ON
python3 tools/g4_bench.py prepare --build-dir build-g4-baseline \
  --output build-g4-baseline-bundle
```

The toolchain selects PPC32 NetBSD and static linking; it assumes the sysroot's C++ headers are in
`usr/include/g++`. Adapt a **local copy** if an existing toolchain uses another layout. CMake, Ninja,
Python, compiler, headers and libraries must already be available on the development host. No
`-ffast-math`, unity build, LTO or CPU-specific tuning is introduced. For a host-only smoke test,
omit `CMAKE_TOOLCHAIN_FILE`; label its output as host evidence.

`prepare` rebuilds three explicit targets, then copies them, two traces, the runner and the compile
commands into a new directory. `manifest.json` records the full Git SHA, dirty status and tracked
diff hash, compiler version/configuration, CMake flags, target-system description, embedded data
hashes, and hashes of every bundle file. It does not certify a dirty source tree as reproducible.
Use clean commits and inspect the manifest before transferring. Output paths must be new.

The bundle is self-contained: there is no runtime dependency on Git, CMake, the repository, X11 or
geographic data files. Transfer it under a distinctive SHA-bearing name through the authorized
hardware session. `file build-g4-baseline-bundle/globe-bench` should identify a **32-bit big-endian
PowerPC NetBSD** executable, not the host architecture. Building/linking is not target execution.

## Commands for the authorized owner or hardware session

On the inspected NetBSD 11.1 installation, the existing interpreter is `/usr/pkg/bin/python3.13`.
From the transferred bundle directory (replace the SHA label and choose unused output names):

```sh
/usr/pkg/bin/python3.13 g4_bench.py run --bundle . --label g4-netbsd-baseline-SHA \
  --repeat 9 --warmup 1 --output g4-netbsd-baseline-SHA-run1.json
/usr/pkg/bin/python3.13 g4_bench.py run --bundle . --label g4-netbsd-baseline-SHA-raster \
  --repeat 9 --warmup 1 --workload shade --output g4-netbsd-baseline-SHA-raster1.json
```

Allow several minutes on the G4. No output is printed inside the renderer's timed sections; one
result path is printed on completion. Normal interruption stops a run; incomplete results are not
published as successful JSON. Keep the machine's OS, foreground load, power state, viewport,
compiler flags and data identical for comparisons. Save the complete output with its manifest.

Default workloads (each measured in a fresh process, round-robin, following one discarded warmup
per workload):

| Workload | What it measures | Parameters and limits |
| --- | --- | --- |
| `line-layer` | Fixed-view complete refinement, first CPU-ready band, worst step | Map, 640x480, lat 30/lon 10, radii 220 and 440, lines-at-end and lines-first; existing default work budget 256 |
| `drag` | Atlas parsing, scene construction, first frame since main, repeated input-style camera updates | `bench_drag.txt`, 640x480, 374 frames per process at 640x480 on the inspected main; embedded 1° data |
| `settle` | Model-driven final refinement, first band/level, largest continuation | `expose; tick 1000`, 640x480, embedded 1° data; no external fine packs |
| `shade` (optional) | Selected flat-cell and shaded-triangle microbenchmarks | Existing benchmark, 3 internal repetitions per process, two radii; not a complete-job or coverage-equivalence comparison |

The JSON preserves every binary's original output and raw process samples. Summary percentiles
are across processes (nearest-rank p95; with 9 samples p95 is the maximum). A median of per-process
frame p95s is **not** a pooled frame p95. Renderer timing uses `CLOCK_MONOTONIC`; process wall time
uses Python's monotonic performance clock and includes launch/loader/exit. First band means ready
in CPU memory, **not first visible response to a real input event**.

Cache state is explicitly uncontrolled: fresh process startup is measured, but embedded data and
filesystem pages may already be cached. Warmups are recorded, never described as cold disk loading.
There is no privileged cache flush. True cold-start/import tests need a separate owner-controlled
protocol. Full current-job phase timings remain future work; the old `globe-refine-profile` is an
algorithm experiment and does not profile today's lines-first phases directly.

`wait4` records each child's `ru_maxrss` in KiB on Linux/NetBSD, plus user/system CPU time. RSS includes
static data, libraries, allocator and OS accounting; it is neither live heap payload nor the Classic
Memory Manager partition requirement. Compiler memory is excluded. The measured NetBSD `wait4` high-water mark showed an inherited
pre-exec Python memory floor (~20 MiB), which can mask smaller binary allocation differences.
Treat this field as an OS process-accounting observation, not reliable evidence of a 259 KB list
allocation delta. A direct native `/usr/bin/time -l ./globe-line-layer-bench --repeat 9` run
from the SSH shell provides a separate smaller-launcher check; retain its stderr alongside JSON. The existing payload
ledger remains separately labeled inside `globe-bench` output.

## Matched comparisons

Do not combine #69, #70 and #71 into one new integration branch. Measure current baseline A and
one candidate B with the same harness, flags and data; #71 necessarily includes its declared #70
base. To isolate #71's incremental benefit compare #70 against #70+#71. Use separate clean review
branches/snapshots; carry the harness separately if needed. Run A/B/B/A blocks, each with 9 samples,
and repeat if drift or outliers dominate. Keep all raw samples and compare checksums before timing.
A changed hash is a correctness investigation, not a speed result. The runner checks repeat stability
within a workload; the engineer must also compare **matched baseline/candidate** hashes.

The first acceptance request is to recheck #74 on bare OS 9 after the presentation-slice fix. NetBSD
headless timing cannot accept OS 9, X11 submission/scanout, real event latency, memory ceilings or
#95. Compiler flag experiments in #68 require separate bare-OS-9 matched builds and authorization;
the NetBSD `-O2` run says nothing decisive about Retro68 `-Os` versus `-O2`.

## Fallback when Python is not already present on NetBSD

The static binaries themselves require no Python. Run from the bundle with new output filenames:

```sh
uname -srm > g4-SHA-system.txt
./globe-line-layer-bench --repeat 9 > g4-SHA-lines.json
./globe-bench --repeat 9 --size 640x480 bench_drag.txt > g4-SHA-drag.json
./globe-bench --repeat 9 --size 640x480 --refine settle.txt > g4-SHA-settle.json
./globe-shade-bench 9 > g4-SHA-shade.json
```

Save `manifest.json` alongside these results. These direct runs lack the wrapper's per-process
samples, warmup exclusion, percentile aggregation and per-child RSS for line-layer/shade. Do not
substitute their internal medians for the wrapper's distributions. Commands only run executables
in the current directory; no owner's installed application is replaced.
