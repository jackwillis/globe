# OS 9 proof of life — built, not run

Build the OS 9 floppy edition locally with `tools/build-edition.sh floppy` (Retro68 container).
Transfer `build-os9-floppy/GlobeFloppy.bin` as MacBinary and decode on the Mac; preserve both forks.
Alternatively use `build-os9-floppy/GlobeFloppy-1440.dsk`, the designated 1.44 MB HFS image. The
framework also generates an 800 KB transport image; that is not the designated floppy report.

No Apple headers, implementations or binaries are copied into this repository. Compilation uses
Retro68's installed Multiversal interfaces and import libraries.

The classic, non-Carbon front end supports dragging in both modes, double-click zoom, arrows,
+/- zoom, R reset, G graticule, B borders, T drag mode, S terrain style, N north up, Q/Escape quit. Close, grow and
window dragging use Toolbox tracking. It embeds the atlas and one-degree elevation, renders a moving
gray land/ocean base and outlines then the same bounded settled terrain bands. Optional toolbar, country/city windows and
acquisition are outside this D7 proof-of-life application.

An offscreen 16-bit GWorld receives big-endian RGB555 via `pixel_format`; locked PixMap rowBytes
are masked to their actual stride, then CopyBits submits complete bands. Microseconds supplies
message times through a minimal ABI declaration (the installed interfaces omit it; InterfaceLib
exports it). AppearanceLib exports SetThemeCursor; the same pure intent maps to arrow 0, closed
hand 8 and open hand 9. The API values are documented in the archived
[Appearance Manager reference](https://leopard-adc.pepas.com/documentation/Carbon/Reference/Appearance_Manager/index.html).
The return type is the documented 32-bit OSStatus (checked against the archived reference).
A failed native cursor call falls back to the system arrow. This requires OS 9 / Appearance Manager.

WaitNextEvent sleeps until an event, a replacement stage deadline, or available refinement work.
With neither deadline nor work it requests a long interruptible sleep; a one-pixel mouse region
wakes for motion without a repeating timer. Future deadlines round up to 60-Hz event ticks.
The pure sleep calculation tests overdue, one-ms, 150-ms, rounding and saturated waits; zero-idle
and rounding-down mutants fail. A SIZE resource requests a 24 MiB partition, minimum 12 MiB.

Actual build: Retro68 GNU 16.1.0, `-Os`, no exceptions/RTTI; linked and packaged successfully with
no final compiler warnings. Designated data fork 237,116 bytes, resource fork 432 bytes, MacBinary 237,824
bytes. Initial compile failures exposed an absent QDOffscreen.h and Carbon-only port accessors;
the final code uses the available classic QuickDraw interfaces instead.

Not executed in an emulator or on OS 9. The owner must verify launch, drag/cursor behavior, input,
resize/expose, colors and quiet idle on hardware. The G4 headless NetBSD tests are not OS 9 tests.
