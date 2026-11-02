# Pixel packing pass — 2026-11-01

Measured with `globe-bench --convert --repeat 2 tests/data/bench_drag.txt`, 640x480,
750 frames, current 376-message trace. G4: NetBSD/macppc, cross GCC 12.5.0, `-O2 -static`.
All runs produced checksum `41c7adb508763323`. Each row is one run, not an averaged series.

| G4 conversion | Mean | p50 | p95 |
| --- | --- | --- | --- |
| Original generic byte loop | 17.0486 ms | 16.7399 ms | 17.1175 ms |
| Fixed four-byte stores, reuse adjacent color | 10.9548 ms | 10.7777 ms | 10.9815 ms |
| Aligned native word stores, byte fallback | 6.9144 ms | 6.8419 ms | 6.9437 ms |

The first attempt missed the under-10-ms target; the aligned path meets it. Aligned stores use
`memcpy` with `std::assume_aligned`, preserving aliasing rules; other destinations use explicit
byte stores. The endian conversion is cached with the pixel's packed value. RGB565 layouts copy
whole rows when native, or swap bytes when necessary. Other formats retain the generic path.
No allocation is added. Row padding remains untouched, including odd strides and unaligned rows.

Raw results: `pass-20261101-g4-before.json`, `pass-20261101-g4-pack-bytes.json`,
`pass-20261101-g4-pack-words.json`. The word-store run also includes the separate line optimization;
only its conversion timings are used here. No X submission or real-display frame rate was measured.
Host generic/byte-path conversion means were 0.7447/0.3396 ms in the corresponding host JSON files;
these are host wall times, not G4 estimates.

Verification: all 65,536 RGB565 values against independent rounded eight-bit channel formulas;
fast-vs-generic comparison including 32-bit unusual masks, both byte orders, flat runs, odd width,
unaligned destinations, padded rows and guards. Existing rejection tests retained. Word endian,
byte fallback endian, stale cached color and RGB565 endian mutants were rejected. Tests also pass
on the G4 (big-endian); Retro68 PowerPC and 68k compile the shared converter, built not run.
