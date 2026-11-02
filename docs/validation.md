# Phase 0 reconnaissance and Phase 1 validation

Date: 2026-11-01. All target evidence below came from real hardware over the
approved maulctl-style SSH jump route, using the supplied IP 10.42.0.30.
The maul.local hostname did not resolve. No access controls were changed.

## Reconnaissance

- Linux x86-64 host: GCC 16.2.1, GNU Make 4.4.1, Xlib 1.8.13 available.
- Target: NetBSD 11.0 GENERIC macppc; powerpc; 7400 revision 2.9; 256 MiB RAM.
- Target GCC 12.5.0 and native NetBSD make available.
- About 34 GB target disk free; approximately 53 MiB RAM free at inspection;
  no swap pages in use.
- Xlib/Xext headers and libraries under /usr/X11R7.
- X :0 and Window Maker running. Xorg log reports Rage 128 Pro GL PF,
  16 MiB VRAM, 16-bit TrueColor, RGB565, and MIT-SHM initialization.
- SSH DISPLAY empty; explicit :0 queries denied authorization. Current display
  dimensions, exact masks, byte order and usable SHM remain unverified.
- clock_gettime and CLOCK_MONOTONIC are declared in target headers; runtime
  timing behavior has not yet been tested.
- CPU clock speed is from assignment context, not independently measured.

## Validation

- Linux optimized build and unit tests: passed, with warning-free final build.
- Linux AddressSanitizer + UndefinedBehaviorSanitizer: passed with
  ASAN_OPTIONS=detect_leaks=0. LeakSanitizer itself could not run because of
  the execution environment's tracing restriction; no leak-check claim.
- Native G4 build with identical Makefile and tests: passed.
- Tests cover equator, north pole, longitude wrapping, orientation,
  horizon crossing/rejection/tangent endpoints, line clipping, invalid frame
  sizes, NaN line input, repeated output and changed camera output.
- Frame at yaw 31, pitch 22: both platforms yielded POSIX cksum
  2126918428, size 230415 bytes. Linux SHA-256:
  ae6ee1def6137610ab701adb7d32ac78ee4016691ec1532407eeb519d161955c.
- Rendered synthetic grid visually inspected on Linux.

No coastline data, X11 frontend or performance benchmark is implemented yet.
No FPS claim is made. Remote build/tests were bounded by a 55-second command
limit; no unrelated processes were terminated.

## Coastline and X11 increment

- Natural Earth source downloaded and converted into 134 lines / 5389 points.
- Coastline view visually inspected; Africa, Europe and South America visible.
- Optimized Linux geometry tests and map parser adversarial tests passed.
- Actual parser exercised under ASan/UBSan with leak detection disabled: passed.
- X11 backend compiled warning-free on Linux.
- Isolated 16-bit Xvfb startup smoke test passed outside the sandbox. The first
  attempt inside the sandbox failed because display sockets could not be bound;
  all test-owned processes were cleaned up. This is a startup test, not a full
  keyboard/resize/visual correctness test or native G4 presentation evidence.
- Native G4 coastline build/tests and X11 compilation passed. Native coastline
  PPM POSIX cksum matched Linux: 2561484380, 230415 bytes, yaw 0 / pitch -15.
- Staged raster output matched the reference renderer in the unit suite; Linux
  ASan/UBSan passed again after introducing the benchmark raster entry point.
- Native monotonic timing worked in the bounded headless benchmark. See
  benchmarks/README.md and machine-readable results for timing evidence.
- Native G4 X11 execution, input/resize testing, and presentation measurements
  remain outstanding. No display permissions were changed.

## Mouse interaction increment

Automatic rotation was replaced with left-button drag rotation and wheel zoom.
Keyboard controls remain. The event loop blocks while idle. Linux build and
geometry/map tests passed. A 16-bit Xvfb integration test sends drag and wheel
events, verifies that each changes the window image, and verifies idle image
stability. This test passed. Native desktop interaction remains unverified due
to the existing SSH display authorization limitation.

Reproduce the Linux interaction check:

```
make test_x11_input
X11_INPUT_TEST=./test_x11_input python3 scripts/smoke_x11.py
```

## Themes and vector layers

Added three palettes and independently selectable grid, country land boundaries,
and coastlines. X11 toolbar is clickable; keyboard alternatives are T/G/B/C.
The renderer paints a flat ocean disc; no terrain/elevation is claimed.
Original reference rendering and benchmark paths are preserved.

Linux: warning-free build, geometry/map validation tests and ASan/UBSan passed
(leak detection disabled for the previously documented environment limitation).
Focused renderer tests check repeatability, distinct layer/palette outputs,
background/disc distinction, and large zoom. The isolated 16-bit Xvfb test now
checks each toolbar button changes the actual window, plus mouse drag, wheel
zoom and idle stability. All passed. Native interactive behavior remains subject
to the existing display-authorization limitation. Prior headless measurements
exclude this new colored background and optional layers.

## Portable optimization increment

The interactive backend now reuses framebuffer and XImage storage. Colored
rendering caches the disc background and camera trigonometry, and transforms
shared endpoints once. The original map_view remains the reference. Optimized
frames matched the reference on Linux and native G4 across 120 benchmark camera
positions; tests also cover themes, layer combinations, poles, zoom and resize.

Pixel conversion uses detected masks, stride, pixel size and byte order, with
XPutPixel fallback for unsupported layouts. Both hosts passed seven synthetic
XImage layouts in both byte orders, including padded rows, against XPutPixel.
Linux ASan/UBSan passed for renderer, cache, packer and XImage comparisons.
Mouse, toolbar, resizing and idle behavior passed with 16-bit and 24-bit Xvfb.

Native G4 before/after results, raw JSON, variability and instrumentation limits
are documented in benchmarks/portable-optimization.md. These measurements do not
include native display presentation. Benchmark programs have a 55-second alarm;
normal runs completed well within the assignment's 60-second limit.

## Separate toolbar increment

Moved controls into a separate child window above a 640×480 globe viewport;
initial main-window size is 640×508. Toolbar updates are composed offscreen.
16-bit and 24-bit Xvfb interaction checks passed: non-overlapping geometry,
unchanged toolbar pixels during drag/zoom/viewport exposure, layer switches,
ignored toolbar wheel/blank-area input, normal resize, a programmatic 1×20 outer
window, and idle stability. The first virtual-display attempt was too short for
the enlarged window and XGetImage rejected the offscreen capture; enlarging the
test display to 800×600 resolved the test setup issue. The final actual UI capture
was visually inspected. Native interactive flicker remains to be assessed from
an authorized G4 desktop session; no display permissions were changed.


## Wikidata place popovers

See places.md for the offline snapshot format, record provenance, regression tests
and bounded native marker/picking timings. The prepared 1664-byte asset matches
between Linux and the G4. Native place tests and X11 build passed. Linux parser
and UI sanitizer checks passed with leak detection disabled; 16-bit and 24-bit
Xvfb popover/clipboard tests passed. Native desktop popover behavior remains
unverified because existing display authorization is unavailable to SSH.
