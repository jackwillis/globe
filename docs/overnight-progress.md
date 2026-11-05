# Overnight progress — 2026-11-05

User authorized unattended work and merge to main after validation. Implementation is complete in
codex/os9-cooperative-scheduling, including integration of main's overlays, info cards and OS 9 keys.

Completed: explicit resolution effects/completions; coarse-to-target sequence; finest-only line
decoration; coverage repair guaranteeing exact final pixels; bundled half-degree data in X11;
32-row immutable GWorld presentation continuations with camera-generation cancellation.
47/47 combined host tests pass, including live two-job X11 quiet idle and real-data full-pixel
comparisons. Eight focused sanitizer tests and both Retro68 compile probes pass; OS 9 package,
fork roundtrip and floppy size checks pass. Policy/format checks pass. Host timing/provenance is
saved in docs/perf/progressive-resolution-host.json and docs/progressive-resolution-plan.md.

Final steps in progress: record the final verified Tiger launch/reset evidence, commit the final
data/docs/integration tests, and fast-forward main if its checkout remains clean. Never compile
on maul; use explicit private mountpoints and verify the application data fork.

Remaining limits: native OS 9 acceptance, G4 higher-resolution performance, measured input-to-pixel
latency, bounded moving-frame/overlay/snapshot-conversion work. The one-degree OS 9 payload is kept.
