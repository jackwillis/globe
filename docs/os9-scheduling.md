# OS 9 cooperative scheduling audit

Audit of baseline 1363c43, performed on 2026-11-04–05 (America/Chicago). Scope: scheduling and presentation foundation before progressive terrain resolution. Rendering remains CPU-only. This is a static audit plus Tiger Classic testing, not a claim of native OS 9 hardware acceptance.

## Findings and changes

| Finding | Change | Remaining limit |
| --- | --- | --- |
| SIZE accepts suspend/resume and allows background execution, but run() never interpreted suspend/resume osEvt messages. Refinement and deadlines could continue while another app was active. | Interpret suspend/resume; pause redraw, refinement and stage deadlines while suspended. Request a long interruptible wait without the mouse-motion region. Resume existing work; end an interrupted drag through update(). | Background update events copy the cached GWorld without converting the framebuffer. Verify alongside another Classic app, not only the native Tiger Finder. |
| The 24 ms slice began after any synchronous coarse redraw and ended only after a full continuation and presentation. | Budget the foreground turn from return of WaitNextEvent, including event handling and redraw; check elapsed time between continuations. Check queued events every 16 continuations to dispatch input sooner. | Individual geometry calls, Toolbox tracking and screen copies remain synchronous. The slice is a target, not a hard latency guarantee. |
| Full framebuffer conversion occurred even when CopyBits requested only a row band. RGB555 used generic color packing. | Convert only requested rows; add an exact RGB565-to-RGB555 bit mapping. | Circle passes report the full frame, so row restriction alone does not reduce their conversion area. CopyBits can still dominate the longest turn. |
| Existing title measured total settling and aggregate copying, hiding pauses between yields. | Also report maximum foreground turn and maximum measured continuation. Reuse the loop clock reading rather than add two Microseconds calls to every tiny continuation. | Wall-clock measurements include Tiger scheduling/preemption. Maximum turn excludes WaitNextEvent itself and tiny bookkeeping after its measurement. |

The original long idle wait, one-shot deadlines, paused-drag presentation gating, copy throttling and final-frame presentation remain. Mutable framebuffer, GWorld and scheduling state stay in the platform/renderer. The core receives timestamped messages and has no Toolbox calls or clock access. No worker threads, periodic idle timers or per-frame allocations were introduced.

## Evidence and tuning

Fresh baseline screenshot on shared G4: 2,303 ms total, 501 ms copying, default home view at 640 x 480. An 8 ms / 16-work-unit candidate with additional per-continuation timing reads took 7,066 ms (copy 1,012, max turn 117, max step 109); reset repeat 6,822 ms (copy 1,014, max turn 61, max step 8). Rejected: too much completion overhead, with copies still extending turns.

A 16 ms / 64-work-unit candidate before reducing clock instrumentation took 3,562 ms (copy 546, max turn 78, max step 14). These candidates change several factors, so they do not isolate the cost of yield frequency, work size, clock reads or pixel conversion. Do not treat a single launch or an incomparable maximum as a causal benchmark. The tuned 16 ms / 64-work-unit candidate, reusing one timing read per continuation and checking queued input every 16 continuations, took 2,783 ms (copy 480, max turn 64, max step 8); reset repeat 2,690 ms (copy 480, max turn 67, max step 14). Retained as a modest throughput tradeoff for a shorter foreground budget and explicit background policy. This does not establish improved perceived responsiveness: the baseline lacks comparable turn instrumentation, and screen copying still overruns the target.

Classic SimpleText opened during a Reset run, and Globe completed after returning. That run reported 9,172 ms (copy 791, max turn 227, max step 225), including time in the other application and launch/preemption effects. It is evidence of switching/resumption, not a background-CPU or latency benchmark. The final package additionally avoids framebuffer conversion on background update events and avoids labeling a canceled refinement as complete. Its launch smoke check reported 2,849 ms (copy 494, max turn 79, max step 3); its SimpleText switch/return check completed at 8,808 ms (copy 800, max turn 217, max step 8), including the interruption. A final Reset took 2,622 ms (copy 481, max turn 67, max step 8). The launched application data fork matches the locally built application.

[Machine-readable trials](perf/os9-scheduling.json). Screenshots and the baseline image are retained locally in the ignored build-scheduling-evidence directory. Remaining measurement work includes input-to-pixel latency, first useful image, idle/background CPU, drag/resize lifecycle and native OS 9 cooperation. No user documents were edited in the SimpleText check.

Validation: 40/40 host tests pass (display tests run sequentially because their existing wrappers share :99); focused AddressSanitizer/UndefinedBehaviorSanitizer tests pass with LeakSanitizer outside the tracing sandbox. PowerPC and 68k compile probes pass without warnings/errors. Retro68 OS 9 build and floppy fork-roundtrip/size checks pass. Policy and formatting checks pass. The ordinary refined replay checksum remains e86d02e2424d2a47. The 640 x 480 client pixels in the baseline and tuned Tiger screenshots are identical. Final package: 249,625-byte data fork, 432-byte resource fork, 287,021-byte floppy payload; fork roundtrip identical. A copy of the final image and MacBinary is retained in build-scheduling-evidence.

## Repeatable measurement procedure

1. Build locally with tools/build-os9.sh --floppy. Never compile on shared G4. Preserve the baseline image and each candidate under distinct filenames; record their hashes and configuration. Transfer to /tmp, mount as a raw HFS disk with hdiutil, and launch the exact mounted application. Do not reuse a still-running older instance accidentally.
2. At 640 x 480 in the default Map view, wait for completion and record the title. Repeat Reset at least three times with the pointer stationary. Record total, aggregate copying, maximum foreground turn and maximum continuation separately. Use screenshots as evidence. For a fair comparison, instrument the baseline equivalently before comparing turn maxima.
3. During refinement, use a Classic application such as SimpleText. Confirm that it accepts input promptly, Globe stops advancing while suspended, no stuck drag remains, and Globe finishes after returning. Repeat during paused drag and paced line reveal. Also try a native Tiger app, while keeping that result distinct from Classic-to-Classic cooperation.
4. Exercise rotation, zoom, style changes, expose and resize during refinement. Assess first useful image, interruption responsiveness and visual continuity separately from total completion. A shorter settle that stalls input is not automatically preferable.
5. After completion, leave the pointer still and sample idle CPU. Repeat while Globe is suspended. Measure the Classic environment with care: its process CPU includes other Classic apps, and cumulative ps %CPU is not an instantaneous Globe measurement. Real OS 9 testing is still needed for scheduler/latency claims.

## Next boundary

Progressive resolution remains a separate implementation described in progressive-resolution-plan.md. Do not add more work to the OS 9 floppy edition merely because a finer source exists. First account for the still-synchronous screen copy and validate cooperation. A future presentation continuation should copy bounded row ranges, unlock PixMaps before yielding, and track snapshot ownership so a new camera generation cannot finish presenting an old one. Startup grid/cache construction and synchronous moving-frame rendering are other unbounded regions to profile if they cause observed delays.

Event decoding follows Apple's [Inside Macintosh: Handling Operating-System Events](https://leopard-adc.pepas.com/documentation/mac/Toolbox/Toolbox-41.html): suspend/resume message high byte 1 and resume flag bit 0. The installed Multiversal headers omit those two symbolic constants, so the platform adapter supplies named local constants.

## Follow-up: bounded snapshot submission

Refinement presentation now coalesces requests into an immutable GWorld snapshot and submits at most
32 rows per continuation, before geometry consumes the foreground budget. Renderer writes continue
into the native framebuffer without changing an in-flight snapshot. New camera generations, resize
and synchronous moving frames cancel old submission. PixMaps are unlocked before every yield.
Pending presentation keeps the loop runnable; completion drains the final settled-overlay snapshot
and returns to the existing long idle wait. Suspension pauses both geometry and presentation.
The existing GWorld is reused: no additional bitmap is retained per resolution. A pure queue helper
tests row bounds, coalescing, stale generations and completion. The window title reports submitted
band count to identify this build in Tiger trials.

Moving frames, expose repaint, snapshot conversion and overlay composition remain synchronous;
32-row CopyBits is a work bound, not a hard wall-time guarantee. The new resolution sequence is
shared with X11, but the OS 9 floppy still uses one embedded grid. Final combined package includes
the newer main-branch overlays, info cards and Page Up/Down controls. Earlier numeric trials above
remain historical and must not be compared as equivalent combined-build benchmarks.

The identified `GlobeVerify` build (same committed runtime sources; creator GlP3 instead of Glb2)
ran alongside the other session's Classic apps from `<temporary-test-binary>`. Its title reported
180 submitted 32-row bands, 2,750 ms total, 734 ms copying, maximum measured turn 20 ms and
maximum continuation 7 ms. This confirms bounded presentation actually executed; the older
Glb2 instance remained active in earlier ambiguous launch attempts, so their screenshots are
excluded from the new result. See [identified Tiger evidence](perf/progressive-resolution-tiger.json).
The final production package additionally guards failed PixMap locks against busy retries; the
normal presentation path is unchanged and the rebuilt package passes compile/fork/size checks.
Do not quit shared Classic to switch builds; use a unique test application name/creator.

The identified Reset repeat reported 180 bands, 2,757 ms total, 732 ms copying, maximum turn 17 ms and maximum continuation 16 ms. Both identified trials retain the existing one-degree OS 9 data and the new main-branch markers/overlays.
