# Overnight work complete — 2026-11-05

Merged into local main at afa7946 after integrating main through 7c245e9. No GitHub push.
The following documentation-only completion record follows that source merge.

Implemented: generation/level refinement effects and validated completions; coarse-to-target
terrain sequence; final-only line decoration; coverage repair giving exact finest-only final
pixels; bundled half-degree terrain in X11; 32-row immutable GWorld presentation with cancellation,
coalescing, background suspension, and quiet lock-failure handling. Newer main-branch overlays,
info windows, picking, markers, controls and overlay tests are preserved. OS 9 keeps one degree.

Validation: 47/47 host tests pass, including actual two-job X11 quiet idle and real-data full-pixel
comparisons in both styles at two zooms. Eight focused sanitizer tests pass. PowerPC/68k probes,
OS 9 build/packaging/fork roundtrip/size, formatting and policy checks pass. The final production
package was rebuilt after the low-memory retry guard.

Host high-zoom compute, nine repeats: complete coarse terrain 9.55 ms, final progressive image
42.22 ms; finest-only final 31.64 ms. Final checksums match. No observed ordinary one-degree
compute regression in the recorded sample. These are host compute timings, not G4 scanout.

Tiger is shared. GlobeVerify (creator GlP3) at /tmp/globe-verify-volume was verified with its new
band-count title, alongside the other session's Classic apps. Identified launch: 180 bands,
2,750 ms settle, 734 ms copy, 20 ms maximum turn, 7 ms maximum continuation. Identified Reset:
180 bands, 2,757 ms settle, 732 ms copy, 17 ms maximum turn, 16 ms maximum continuation.
The verification source is 36608c4 with application name/creator changed only; a later production
lock-failure guard changes the failure path only. The final production package is compile/size
checked. Earlier ambiguous ordinary-app launches are excluded from the new Tiger evidence.
Do not quit shared Classic or assume numbered volume names identify a build.

Review: docs/progressive-resolution-plan.md, docs/os9-scheduling.md, data/SOURCES.md,
docs/perf/progressive-resolution-host.json, docs/perf/progressive-resolution-tiger.json.
Installable production image/MacBinary and screenshots are retained in the Codex worktree's
ignored build-scheduling-evidence directory, named GlobeProgressive-1440.dsk and GlobeProgressive.bin.
The worktree is /home/jack/.codex/worktrees/progressive-resolution/globe.

Remaining limits, not claimed complete: native OS 9 hardware acceptance, G4 half-degree runtime
performance, measured input-to-pixel latency, and bounded moving-frame/snapshot-conversion/full
settled-overlay work. These do not block the completed coarse-to-fine and row-submission changes.
