# Line overhead pass — 2026-11-01

Same 640x480 trace, `--convert --repeat 2`, 750 frames, cross GCC 12.5.0 `-O2 -static`.
Compare `pass-20261101-g4-pack-bytes.json` with `pass-20261101-g4-lines.json`; both use the same
byte-store pixel packer, and the only renderer change is the line optimization.

| G4 renderer | Mean | p50 | p95 |
| --- | --- | --- | --- |
| Before | 20.7951 ms | 21.1496 ms | 24.3357 ms |
| Inline floor and safety-box fast path | 18.6214 ms | 18.9134 ms | 23.5502 ms |

One run each, no real-display submission timing. Host renderer means were 0.3939 and 0.3129 ms
in matching host JSON files. Both host/G4 checksums remain `41c7adb508763323`; golden frames and
app replay hash are unchanged. No re-pin was needed.

The inline floor converts only finite coordinates confined to +/-1e6, then corrects negative
nonintegers after truncation. When both endpoints already lie in that box, Liang-Barsky is skipped;
otherwise the existing safety clip still runs. Work remains bounded by framebuffer dimensions.
Boundary tests ran against the old code before implementation, then passed against the change.
Mutation checks rejected truncation without negative correction (exact-clipping mismatch at
camera line 2, pixel 1,0) and dropping out-of-box lines (four new safety-box assertions failed).
The complete renderer test passes on host and G4; Retro68 PowerPC/68k compile, built not run.
