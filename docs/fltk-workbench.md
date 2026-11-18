# FLTK Unix workbench experiment

[Issue #183](https://github.com/jackwillis/globe/issues/183) tests whether native
widgets make a small Unix development workbench easier to maintain. Use the Xlib
application as the comparison baseline.

## Build and run

Use a C++20 toolchain, CMake, Ninja, Python 3, X11 development libraries for the
reference build, and FLTK 1.4 with its CMake package. Fedora 44 packages FLTK
1.4 as [fltk-devel](https://packages.fedoraproject.org/pkgs/fltk/fltk-devel/).
For a user-installed build, pass `-DFLTK_DIR=/path/to/fltk/cmake/config/directory`.
The workbench links the base toolkit target `fltk::fltk`.

```sh
cmake -S . -B build-fltk -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLOBE_BUILD_FLTK=ON
cmake --build build-fltk
ctest --test-dir build-fltk --output-on-failure
./build-fltk/globe-fltk --size 420x440 --stats
./build-fltk/globe-x11 --size 420x440 --stats
```

The viewport size excludes window chrome in both commands. On a Wayland desktop,
set `FLTK_BACKEND=x11` for the X11 comparison. Run with 100% display scaling
when comparing captured pixel values. Both programs accept an external
`--atlas FILE` and up to three `--elevation FILE` arguments. For the baseline
leave them unset so both use the embedded world atlas and terrain. The
Info button explicitly opens the auxiliary window. Subsequent clicks on the
globe update the read-only card in place, even while the window is hidden.

## Linux verification and comparison

Run both builds on the same local X11 session or start a private 24-bit Xvfb
with `xvfb-run -a -s "-screen 0 1280x1024x24" sh`. Execute the following steps
at equal viewport size, default Map look, and the same display scaling.

1. Open each program separately. Capture the settled globe rectangle with an X11
   window capture tool. Crop Xlib at client origin and FLTK at client y=32, using
   the same 420×440 rectangle. Compare pixel values at the same settled camera
   and check the limb, coastlines, markers and overlays. Record any differences.
2. Drag, wheel zoom, and resize during refinement. Verify the latest camera and
   viewport settle without stale pixels. Cover and uncover the globe and verify
   the recovered image. Repeat with a held drag and check refinement is published
   after release.
3. Open Info, select a recognizable country, and verify that the card text updates
   while focus stays on the globe. Close and reopen Info to verify the latest card
   persists. Toggle Relief and return to Map. Close Globe with Info open and
   verify that both windows disappear and the process exits successfully.
4. Sample Linux RSS and CPU ticks from /proc/PID/status and /proc/PID/stat at first
   visible frame, then twice five seconds apart after settling. Repeat five
   fresh launches per frontend and record launch-to-visible-frame latency,
   settled RSS and the deltas in idle CPU ticks. Use `stat -c %s` and `ldd`
   for binary size and dynamic dependencies, recording toolkit linkage.
5. Repeat the same interaction sequence five times and save each application's
   `--stats` output on exit. FLTK reports call counts and wall-time totals for
   moving render, bounded refinement steps, ShownFrame plus RGB conversion, and
   draw submission. Divide totals by counts where appropriate and report event
   and frame counts with the timings. FLTK draw submission omits server completion.
   Xlib's presentation timing includes XSync, so compare timings only within
   their stated boundaries. Neither timing measures compositing or scanout.
   For repeatable core work and Xlib protocol checks, use the existing
   `globe-bench` trace and CTest X11 suite.

| Item | FLTK prototype | Xlib reference |
| --- | --- | --- |
| Frontend-specific physical lines | 374 in one file | 1,526 across main and auxiliary-window source/header files |
| Build dependency | FLTK 1.4 CMake package and its configured backend dependencies | X11 development libraries |
| Release executable | Pending FLTK compile | 709,552 bytes on Fedora 44 x86_64, GCC 16.2.1 |
| Application presentation buffer at 420×440 | 554,400 bytes packed RGB24 | 739,200 bytes for a 32-bpp XImage with this width and no row padding |
| Copy and conversion | Full RGB565-to-RGB24 conversion per publication, cached across exposes. FLTK may convert or copy again for its backend. | Full conversion into XImage per publication, then XPutImage, potentially limited to refinement-band rows. Exposes reuse the XImage. |
| Startup, RSS, timing and interaction | Pending paired live tests | Pending paired live tests |

Shared renderer workspace and ShownFrame overlay copies are outside the
presentation-buffer totals. The FLTK adapter caps intermediate refinement
publications at roughly 30 Hz, forces the final publication when input permits
it, and blocks on toolkit events when idle. The line counts compare different
feature scopes. Xlib includes replay, more extensive instrumentation, keyboard
commands, and additional workspace controls. The FLTK widgets do remove the
need to draw and hit-test custom Look and Information chrome.

### Recorded host results

Fedora 44 x86_64, GCC 16.2.1, CMake 4.3.0, base `0b5b16d`:

- `cmake -S . -B /tmp/globe-fltk-build -G Ninja -DCMAKE_BUILD_TYPE=Release`
  and `cmake --build /tmp/globe-fltk-build -j 6` passed with FLTK disabled.
- `ctest --test-dir /tmp/globe-fltk-build -E '^x11_' -j 4 --output-on-failure`
  passed all 58 tests. Full CTest ran 68 tests, with all 10 X11 tests blocked by
  Xvfb's inability to bind Unix or TCP listeners in the sandbox.
- `tools/check-policy.sh` passed. `tools/probe-retro68.sh` stopped at Podman
  setup because its runtime directory is read-only. No cross-compile result.
- `cmake -S . -B /tmp/globe-fltk-enabled -G Ninja -DGLOBE_BUILD_FLTK=ON`
  was blocked by missing FLTK development files. Shell DNS restrictions
  prevented obtaining the dependency. No FLTK compile, live test or screenshot.
- `/tmp/globe-fltk-build/globe-bench --size 420x440 --convert tests/data/trace1.txt`
  produced checksum `08233f33bd52312f` over 339 rendered frames. Headless
  render and conversion medians were 0.2922 ms and 0.1260 ms respectively,
  first frame was 4.4773 ms from main, and process peak RSS was 7,528 KiB.
  The benchmark used one elevation level, so these figures are a renderer
  reference and cannot fill the two-frontend measurements above.

## NetBSD 11.1/macppc acceptance checklist

Owner-run after authorization. Use a host build or an equivalent NetBSD/macppc
build environment for day-to-day builds, then test the resulting executable
with the G4's local X server. A native build probe, if wanted, is an owner
decision. Obtain FLTK from pkgsrc `x11/fltk`. Check the actual installed
package and its CMake config. The presence of
[PowerPC packages for NetBSD 10](https://mirror.nju.edu.cn/pkgsrc/current/pkgsrc/x11/fltk/index.html)
does not establish availability or ABI compatibility for 11.1.

- Record `uname -a`, `uname -p`, `c++ --version`, `cmake --version`, and
  `pkg_info -e 'fltk>=1.4'`. Verify a C++20 compiler and record the package
  repository, version, and X11 backend. Confirm executable and toolkit library
  architecture, endianness and bitness with `file`, and linked libraries
  with `ldd`. Keep compiler and runtime ABI consistent.
- Use the build commands above with the target compiler and pkgsrc prefix.
  If necessary pass `-DCMAKE_PREFIX_PATH=/usr/pkg` and use the applicable
  pkgsrc CMake package path for `FLTK_DIR`. Record the exact compiler,
  CMake options, binary size and dynamic dependencies.
- From the local session record `DISPLAY` and `xdpyinfo` server vendor, depth,
  and visual. Start the binaries at 420×440 from that session. Use SSH for
  artifact transfer and log retrieval. An SSH launch for measurement is valid
  only with the existing local X display and its authorized credentials.
- Repeat the Linux pixel, drag, wheel, resize, Look, selection, Info, expose and
  quit checks, looking explicitly for byte order and color errors on PowerPC.
  Record launch-to-first-frame, responsiveness, `ps -o pid,rss,vsz -p PID`
  after settling and during refinement, and peak RSS via `/usr/bin/time -l`
  when available. Attach viewport, visual, atlas, compiler and host details.
  Keep SSH-forwarded X11, emulation, cross-build and native timing results
  distinct.

Continue as a narrow developer workbench if the paired Linux run confirms
frame/input correctness and acceptable memory and presentation costs. Prioritize
that run before adding workbench features or drawing conclusions about macppc.
