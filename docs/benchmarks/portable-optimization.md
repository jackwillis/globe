# Portable optimization measurements — 2026-11-01

Native PowerPC 7400 revision 2.9, 256 MiB RAM, NetBSD 11.0/macppc, GCC 12.5.0.
Flags: `-O2 -std=c99 -Wall -Wextra -Wpedantic`. CPU frequency is 467 MHz per
assignment context, not independently measured. All runs use CLOCK_MONOTONIC.

## Workloads and results

Both paths use the same 120 camera positions (yaw 0 through 357 in 3-degree
steps, pitch -15, zoom 1), Ocean theme, coastlines and borders enabled, grid off.
Order alternates between reference-first and optimized-first to reduce ordering
bias. No warmup; first-frame cache setup is included. Each output is compared
byte-for-byte before accepting its sample. Mismatches abort the benchmark.

| Workload | Resolution | Mean before → after (ms) | Mean speedup | p99 before → after (ms) |
|---|---|---:|---:|---:|
| [Colored render (initial, noisy)](g4-view-640-initial.json) | 640×480 | 88.92 → 36.19 | 2.46× | 379.15 → 339.89 |
| [Colored render (repeat)](g4-view-640-repeat.json) | 640×480 | 42.74 → 15.37 | 2.78× | 47.96 → 29.55 |
| [Colored render](g4-view-320.json) | 320×240 | 25.24 → 10.45 | 2.41× | 33.07 → 20.80 |
| [Modeled RGB565 conversion](g4-packing-640-final.json) | 640×480 | 82.52 → 11.89 | 6.94× | 87.55 → 13.94 |
| [Modeled RGB565 conversion](g4-packing-320-final.json) | 320×240 | 20.26 → 2.67 | 7.59× | 21.00 → 2.79 |

The first 640×480 render run had large scheduling/timing spikes. It is retained
above alongside the repeat rather than discarded. Neither run establishes
sustained screen FPS. Conversion and render runs were separate; their averages
must not be presented as a measured complete frame loop.

Each run took less than 16 seconds. Native benchmark peak RSS reports ranged
from about 4.7 to 9.2 MiB; they cover the benchmark process, including reference
buffers, not a precise measurement of the interactive application.

## Changes measured

- Compute camera trigonometry once and reuse transformed segment endpoints.
- Cache the background disc while dragging; rebuild for theme, zoom or size.
- Reuse framebuffer and XImage allocations in the interactive window.
- Replace per-pixel XPutPixel conversion with mask lookup tables and per-row
  byte-layout selection. Keep XPutPixel as a fallback and correctness reference.

The intermediate generic lookup converter took 24.65 ms on the G4 at 640×480
([intermediate run](g4-packing-640.json)); selecting layout once per row reduced
the final conversion mean to 11.89 ms. Every tested output matched XPutPixel.

## What remains unmeasured

Native X11 presentation is unavailable to the approved SSH account because it
has no authorization to the active desktop. No access controls were changed.
Native conversion tests use a deliberately modeled MSB-first RGB565 XImage;
they do not establish the active display byte order or measure X server work.

The presentation benchmark was exercised with Linux Xvfb at depth 16 and 24.
It separately reports rendering, conversion, submission including XFlush,
XSync server-completion proxy and a continuous frame loop paced toward 30 FPS.
These virtual-display results are functional evidence, not G4 performance or
physical scanout timing. Submission may include transport backpressure; XSync
is an upper-level server-completion proxy, not a vblank or scanout timestamp.

For native presentation evidence, run from an already authorized G4 desktop
session in the project directory:

```
./planet-x11-bench 320 > /tmp/planet-presentation-320.json
./planet-x11-bench 640 > /tmp/planet-presentation-640.json
```

Both new benchmark programs stop starting samples after 45 seconds and have
a 55-second process alarm. Remote commands additionally used timeouts.
No unrelated process was stopped.

## Correctness and next decision

Linux and native G4 tests passed for rendering equality, theme/layer changes,
pole views, wraparound, zoom and resize. Pixel packing matches XPutPixel for
seven layouts in both byte orders, including row padding. Linux ASan/UBSan
passed; leak detection remains unavailable in the local execution environment.
16-bit and 24-bit Xvfb tests passed for mouse input, toolbar switches, resize
and idle stability. The portable reference paths remain available.

AltiVec and MIT-SHM remain deferred. The portable fixes already provide
measured gains without changing pixel output. Native display submission and
pacing should be measured before choosing another optimization. Grid-enabled
views and background-cache rebuilds may have different costs from this workload.
