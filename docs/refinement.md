# Bounded refinement

The pure quality profile derives from stage: Moving/Settle use 3 px arc pieces; Refine/Decorate
use 1.5 px and permit terrain when data exists. Time remains in Tick messages, not quality payloads.
Accepted input and resize increment Model.generation. A job snapshots the camera, layers, quality
and generation; a stale generation cancels before touching the framebuffer. Hover/repaint do not
invalidate geometry. No allocation occurs in the job or its step.

Refinement is drawn in 16-row bands. Each continuation handles at most 64 units by default:
one background row, geographic arc, or table transition. Long arcs retain the existing subdivision
cap; raster work is clipped to the band while preserving Bresenham phase. Partial bands remain
unpresented until complete. The platform pumps input between continuations. A completed job leaves
no periodic wake. Frontends must cancel old jobs and redraw coarse pixels when input returns.

The benchmark accepts --refine and reports full refinement render time, maximum continuation time
and step count separately from moving-frame timing. At 640x480 on the G4, one run after the drag
trace took 157.4420 ms across 1,830 steps; the slowest step was 0.5975 ms. This excludes packing and
submission of completed bands. Raw host/G4 results are in docs/perf/refinement-*.json. Optional
--refine checksums differ from moving-frame checksums by design; original pins remain unchanged.

X11 uses the same continuation and pumps events between steps. Its idle measurement now waits
for settling and refinement to complete before measuring two seconds of genuinely quiet idle.
Refinement bands are reported separately; zero idle frames means no redraw after completion, not
that the newly enabled detail was suppressed. Tests require actual refinement bands and two
one-shot stage wakes, then no more work. Real-display behavior remains owner acceptance.

Tests compare banded and full rendering, enforce work/row limits, check clip restoration and stale
job nonmutation. Generation, work-limit, clip restoration and out-of-band-write mutants all fail.
