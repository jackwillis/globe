# Overnight progress — 2026-11-05

Worktree: codex/os9-cooperative-scheduling. User authorized unattended work and merging to main after validation. Main has advanced independently (day/night overlays and OS 9 keys); preserve those changes when integrating.

Completed in the previous commits: OS 9 suspension policy, 16 ms foreground target, exact RGB555 row conversion, audit and Tiger measurements; half-degree data rebuilt twice identically.

Current implementation under validation: immutable GWorld snapshot submitted in 32-row continuations; pure presentation ownership/coalescing helper with tests; explicit Refine effects and RefinementFinished messages; sorted coarse-to-target resolution selection; terrain-only intermediate levels; one-bit framebuffer coverage plus bounded repair removing coarse remnants; benchmark and frontend integration. Final pixels equal fresh finest-only output in synthetic adversarial land/ocean tests for both styles, limb/polar views and two zoom levels. Existing one-level pins pass so far.

Remaining: complete trace roundtrip support, integrate latest main preserving overlays, full host and sanitizer/display suites, Retro68 probes/build, real half-degree/X11 run and finest-only comparison, Tiger presentation measurements against the latest combined build, documentation/provenance, commit and merge only after checks. The half-degree artifact is at build-scheduling-evidence/elevation-0.5deg.gelv. OS 9 floppy remains one-degree.

Do not assume a numbered /Volumes/Globe mount: Tiger's mount inventory changed during testing. Mount subsequent images at an explicit private /tmp path and verify the launched APPL data-fork digest. Never compile on maul. Normal screen-copy calls for moving/expose remain synchronous; only refinement presentation is being split. Actual input-to-pixel latency and native OS 9 acceptance must not be claimed from screenshots alone.
