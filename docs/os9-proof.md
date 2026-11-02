# OS 9 proof of life — built, not run

Build locally with Retro68's PowerPC toolchain in the container:

```
podman run --rm -v "$PWD":/src:Z -w /src ghcr.io/autc04/retro68 sh -ec '
  cmake -S os9 -B build-os9 -DCMAKE_TOOLCHAIN_FILE=/Retro68-build/toolchain/powerpc-apple-macos/cmake/retroppc.toolchain.cmake
  cmake --build build-os9 -j2'
```

Transfer `build-os9/Globe.bin` as MacBinary and decode on the Mac; preserve both forks. The generated
`.dsk` is Retro68's default 800 KB transport image, not the designated 1.44 MB floppy experiment.
No Apple headers, implementations or binaries are copied into this repository. Compilation uses
Retro68's installed Multiversal interfaces and import libraries.

The classic, non-Carbon front end supports dragging in both modes, double-click zoom, arrows,
+/- zoom, R reset, G graticule, B borders, T drag mode, N north up, Q/Escape quit. Close, grow and
window dragging use Toolbox tracking. It embeds the atlas and one-degree elevation, renders moving
outlines then the same bounded settled terrain bands. Optional toolbar, country/city windows and
acquisition are outside this D7 proof-of-life application.

An offscreen 16-bit GWorld receives big-endian RGB555 via `pixel_format`; locked PixMap rowBytes
are masked to their actual stride, then CopyBits submits complete bands. Microseconds supplies
message times through a minimal ABI declaration (the installed interfaces omit it; InterfaceLib
exports it). AppearanceLib exports SetThemeCursor; the same pure intent maps to arrow 0, closed
hand 8 and open hand 9. The API values are documented in the archived
[Appearance Manager reference](https://leopard-adc.pepas.com/documentation/Carbon/Reference/Appearance_Manager/index.html).
A failed native cursor call falls back to the system arrow. This requires OS 9 / Appearance Manager.

WaitNextEvent sleeps until an event, a replacement stage deadline, or available refinement work.
With neither deadline nor work it requests a long interruptible sleep; a one-pixel mouse region
wakes for motion without a repeating timer. Future deadlines round up to 60-Hz event ticks.
The pure sleep calculation tests overdue, one-ms, 150-ms, rounding and saturated waits; zero-idle
and rounding-down mutants fail. A SIZE resource requests a 24 MiB partition, minimum 12 MiB.

Actual build: Retro68 GNU 16.1.0, `-Os`, no exceptions/RTTI; linked and packaged successfully with
no final compiler warnings. Data fork 233,020 bytes, resource fork 424 bytes, MacBinary 233,728
bytes. Initial compile failures exposed an absent QDOffscreen.h and Carbon-only port accessors;
the final code uses the available classic QuickDraw interfaces instead.

Not executed in an emulator or on OS 9. The owner must verify launch, drag/cursor behavior, input,
resize/expose, colors and quiet idle on hardware. The G4 headless NetBSD tests are not OS 9 tests.
