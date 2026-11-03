# OS 9 proof of life — launched once in Tiger Classic

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

## First run, 2026-11-02 (Tiger Classic on maul)

Mac OS X 10.4.11 on maul, reached with `maulctl osx`. `GlobeFloppy-1440.dsk` was pushed to `/tmp`,
attached with `hdiutil attach -imagekey diskimage-class=CRawDiskImage` (the plain `.dsk` is not
recognized; the raw-image key is), and `open /Volumes/Globe/GlobeFloppy` started Classic
(`Classic Startup`, `TruBlueEnvironment`) and the app. A `screencapture` of the Tiger desktop shows the Globe
window open and the **Map look rendered correctly** (land classes, sea, graticule): see
`docs/images/os9-tiger-classic.png`. The app volume holds `GlobeFloppy` (245,370-byte data fork),
`ReadMe`, `License` and `DataCredit`.

Not yet checked there: drag and cursors, double-click zoom, keys (`S`, `R`, ...), resize and expose,
refinement timing and quiet idle. Tiger Classic is an emulated environment, so this is evidence the
build launches and draws on OS 9 APIs, not real OS 9 hardware acceptance (GL-1 stays open).

## Settle time in Tiger Classic, and presentation throttling (2026-11-02)

The owner found refinement "super slow" in Classic. The window title now reports the last settle
(`Globe N ms, copy M`: first refine step to last, and total time inside `copy`). First measurement,
read from a screenshot of the Tiger desktop: **4,267 ms, copy 2,101 ms**. About half the time was
copying about 33 full frames (16 terrain rings and 16 line-reveal steps, roughly 64 ms each: convert
the whole framebuffer, then CopyBits). Two changes in `src/platform/os9/main.cpp`:

- refine steps now run in 24 ms slices before each `WaitNextEvent` (a yield costs a tick or more);
- a refinement frame is shown only when the last copies took under a quarter of the time since they
  started (`present`); skipped frames are caught up by the next shown one and the final frame is
  always shown.

Result: **2,314 ms, copy 458 ms** (this includes the 480 ms of paced line-reveal pauses). Settled
pixels are unchanged (pins and host tests unchanged). The X11 front end got the same throttle
(`present_refine`), a no-op when uploads are fast. Still untested: interaction, real OS 9 hardware.
