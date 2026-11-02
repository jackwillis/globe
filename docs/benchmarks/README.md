# Initial headless baseline — 2026-11-01

Native PowerPC 7400 / NetBSD 11.0; GCC 12.5.0;
`-O2 -std=c99 -Wall -Wextra -Wpedantic`. Assignment CPU clock: 467 MHz,
not independently verified. 256 MiB physical RAM.

320×240 RGB framebuffer, Natural Earth coastline (5389 points), 120 frames per
measurement. Camera yaw advances by 3 degrees per frame, pitch -15, zoom 1.
Three measurements run consecutively for each camera; no warmup. The transform
pass transforms each vertex once. The raster pass clears the framebuffer, clips
and projects transformed segments, then draws lines. Total headless measures the
reference renderer separately, including clearing and repeated endpoint transforms.
Therefore the isolated stage times must not be summed to infer reference totals.
A unit test confirms that staged and reference rendering match for a fixed view.

| Measurement | Mean ms | Median ms | p95 ms | p99 ms |
|---|---:|---:|---:|---:|
| Transform | 6.771 | 7.133 | 7.339 | 8.157 |
| Raster | 6.279 | 5.743 | 6.602 | 11.094 |
| Total headless | 12.948 | 12.668 | 13.926 | 14.472 |

Total render-only throughput: 77.23 frames/s. Entire benchmark: 3.12 seconds.
Reported peak RSS: 9344 KiB, including all benchmark stages and buffers.
The renderer has not been optimized beyond the compiler's -O2 setting.
This is a single short baseline, not evidence of sustained presentation at 30 FPS.

Reproduce:

```
make planet-bench
./planet-bench data/coastline.plnt > result.json
```

The program stops starting samples after 45 seconds and caps samples at 120.
Remote experiments additionally use a 55-second outer SSH timeout. No output
image writing occurs inside timed sections. Percentiles use nearest ranks;
median uses the lower middle observation for even counts.

X11 conversion, image submission, synchronization proxy and end-to-end pacing
remain unmeasured because native display authorization is unavailable. Physical
scanout timing is not measured. No performance improvement claim is made.
