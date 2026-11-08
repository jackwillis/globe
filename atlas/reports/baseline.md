# Checkpoint A: verified baseline

Audit base: fetched `origin/main` **886d126**, November 7, 2026 (America/Chicago).
This is distinct from the owner's local main and concurrent Claude work. No production pack,
decoder, renderer, UI or build policy changed in the atlas experiment.

## Tests

Fresh x86-64 Linux configure (`cmake -S . -B build -G Ninja`) found GCC 16.2.1,
Python 3.14.8, X11 and Xvfb. `cmake --build build -j 4` succeeded.
`ctest --test-dir build -N` registered **49 tests**. A serial execution,
`ctest --test-dir build --output-on-failure -j 1`, executed **49** and passed **49**
in 62.08 seconds. These are CTest cases, not individual assertions.

The sandboxed parallel attempt executed 49, passed 39, and failed 10 because test displays could
not be opened. An unrestricted parallel run passed 44 and failed 5 with display shutdown/pixel
interference. Serial private-display execution passed all 49. Do not use the parallel failures
as evidence of an atlas regression or a newly diagnosed rendering defect. No tests were excluded.
Logs are retained alongside the checkout in `/tmp/globe-atlas-tests*.log`.

Availability depends on CMake configuration: `GLOBE_BUILD_TESTS`, Python, X11 and Xvfb conditionally
control registration. The README's hard-coded 47 was stale; the separate housekeeping commit
replaces it with configured discovery instructions. No universal current test total is promised.

## Measured production bytes

`python3 atlas/research/baseline.py --output atlas/reports/baseline.json` validates production
pack CRCs/lengths where implemented and inventories the actual committed bytes. The existing
CTest suite remains the authoritative strict C++ parser check, including the typed GGRD decoder.

| Pack | File bytes | Records / dimensions | Additional geometry or decoded payload |
| --- | ---: | --- | ---: |
| GATL | 35,068 | 467 parts, 8,120 points | Scene heap measured 249,697 |
| GPOL | 91,928 | 177 countries, 289 rings, 10,360 vertices | Unit vectors measured 248,640 |
| GPLC | 9,641 | 304 places | Unit vectors measured 7,296 |
| GELV 1° | 65,352 | 360 × 180 cells | Terrain heap measured 3,590,509 |
| GELV 0.5° | 259,752 | 720 × 360 cells | Terrain heap measured 14,308,969 |
| GGRD land classes | 3,607 | 360 × 180 classes | Decoded heap measured 64,800 |

Sources, licenses, builder commands, selection and decoder behavior accompany the records in
`baseline.json`; exact original upstream hashes and retained responses remain in
[`data/SOURCES.md`](../../data/SOURCES.md) and builder constants. Pack hashes match that ledger.
NOAA and RESOLVE archives were **not reacquired or reconverted** during this audit. Existing
source reproducibility evidence must not be represented as a new conversion measurement.

GATL uses Natural Earth 110m coastline 4.1.0 and land boundaries 5.1.0. GPOL uses 110m countries
5.1.1. Those data are public domain. GPLC uses the retained November 2 Wikidata response (CC0
structured data). GELV uses ETOPO 2022 v1 60s ice surface (CC0) plus 50m lakes 5.0.0 (public domain).
GGRD derives from RESOLVE Ecoregions 2017 (CC BY 4.0); retain `LANDCLASS-NOTICE.txt`, license link,
attribution and adaptation notice. RESOLVE describes ecological regions, not current land cover.

## Storage and host memory

All six packs are embedded in the full X11 build. Explicit X11 elevation imports retain the coarse
base and use supplied levels instead of the default embedded half-degree level. Inspection of
`os9/CMakeLists.txt` confirms that the Classic compact build also embeds **country and city packs**,
as well as atlas, one-degree terrain and land classes; it omits half-degree terrain. The earlier
architecture sentence describing country/city data as full-only is misleading. External Classic
pack loading remains future work (#48).

`memory_probe.cpp` is a standalone tool, not a runtime change. It overrides ordinary host C++
allocation to count requested payload bytes around individual scene construction, including
nothrow allocations and vectors. It excludes allocation headers, allocator rounding, input file
vectors allocated before tracking, stack/inline object storage and OS/display memory. Each scene
is destroyed before the next measurement. Construction peaks equal live payload in these runs.
No over-aligned allocations were observed or instrumented; it is not a general memory ledger.

```
c++ -std=c++20 -O2 -fno-exceptions -fno-rtti -Iinclude \
  atlas/research/memory_probe.cpp build/libglobe_core.a -o /tmp/globe-atlas-memory-probe
/tmp/globe-atlas-memory-probe
```

Host ABI: `Vec3=24`, `CellBands=4`, `PolylineCap=32`, `TerrainScene=3152`, pointer=8 bytes.
`TerrainScene::bytes()` includes 3,072 inline palette bytes in addition to the heap above:
**3,593,581** and **14,312,041** bytes respectively. Those values match the analytical cache
formula. Neither figure is process RSS. PowerPC pointer/object/allocator sizes require separate
measurement; no G4 run was performed.

## Verified documentation discrepancies

* `docs/place-format.md`'s historical 28/28 and the README's 47 are not the configured count.
* `place_card()` still describes the November 1 starter and a small starter set, although the
  production pack is the November 2 304-city selection. Recorded here; runtime text untouched.
* `docs/architecture.md`'s full-only country/city implication conflicts with the OS 9 CMake target.
* `docs/landclasses-format.md` says five 256-color tables add 5,120 bytes. With current two-byte
  `Rgb565`, `class_colors_` is **2,560** bytes; the separate relief palette adds 512. The source
  budget formula includes the correct combined 3,072. Documentation correction is a follow-up.
* Historical sections in `data/SOURCES.md` describe work that was not implemented at that earlier
  time; later adoption entries and current builders supersede those statements.

## Integration verification

After merging local main through `ce54d39` and recording the already-equivalent historical Codex CI
branches, a fresh configure again registered 49 tests; the complete serial run passed 49/49 in
60.24 seconds. No-X11 configure (`-DCMAKE_DISABLE_FIND_PACKAGE_X11=TRUE`) registered **39**;
tests-disabled configure (`-DGLOBE_BUILD_TESTS=OFF`) registered **0**. Those two configurations
were registration checks only, not built or executed suites.

The repeated memory probe on the integrated code records atlas scene heap **241,912** bytes in
two allocations rather than baseline 249,697 in four; Claude's committed rendering simplification
removed the old line-chunk workspace. Other measured layer figures are unchanged. Both snapshots
are retained with their distinct bases; atlas research did not make that runtime change.

The final integration through local main **db07dab** includes Claude's new looks-palette test:
**50 registered, 50 executed, 50 passed**, serial host run 60.54 seconds. The final no-X11
configuration registers **40**, and tests-disabled remains **0**; configuration-only cases were
not executed. `verification.json` records this final result rather than overwriting the initial
49-test baseline evidence. The eight isolated research tests, 75 full-source semantic diagnostics
(69 pass, six geometry-ID-reuse warnings, no failures), policy check and two byte-identical complete
edition builds also pass. No test was disabled to obtain the result.
