# Project notes (state at wrap-up, 2026-11-01)

Read with `docs/handoff.md` (rules, commands, dated pass notes) and `docs/adr/0001-m2-to-m5-decisions.md`
(all decisions and addenda). This file records what is not in the code.

## Where things stand
- Branch `v2`. Unmerged side branches, each one reviewed by its author only and host-tested:
  `claude/smooth-quad` (shaded triangle fill plus G4 bench), `claude/lakes-source` (Natural Earth lakes
  record), `claude/refine-profile` (G4 refine profiler), `claude/click-rules` (click and repaint rules in
  the core, commit 987cd37, 30/30 host tests, G4 not run), `claude/compression-experiment`
  (measurement script). Merge order suggestion: click-rules (rebase `x11/main.cpp`; v2 already has a
  cached-pixel expose change from Codex, 4cd730b, so expect overlap), then smooth-quad, refine-profile,
  lakes-source, compression-experiment.
- Uncommitted work left by Codex (not reviewed, not tested by Claude): CI workflow, README, OS 9 build
  and floppy-size scripts, `docs/floppy-size*`. Build, test and read before committing.
- Never run on real hardware: tip5 (`/tmp/globe-x11-tip5` on maul), OS 9 front end, click-rules.
- Removed or open: coastline toggle removed; cursor feedback and toolbar exist; gray base while moving,
  palette styles, Antarctica white, lakes in the pack, dissolve order, lines-once-per-refine (biggest
  measured speed-up), fetch inside the app and TLS from `../weather` are not done.

## Measured facts worth keeping
- G4 640x480: refine 786 ms with terrain, 456 ms without; lines are redrawn per band (about 16x the
  28 ms single pass). Shaded fill adds about 78 ms at 1x zoom. libm: sqrt 1.2 us, atan2 0.6 us.
- Compression (payload only): 1 degree grid 64,800 -> 29,140 (Paeth + arithmetic); 0.25 degree
  1,036,800 -> 342,041; atlas points 32,480 -> 26,736 with delta-varint.

## Lessons
- Never compile on maul; cross-build, push under new names, `chmod 755`; never touch the owner's
  running binaries (tip3/4/5); display access is the owner's; no `pkill -f`.
- Retro68 lacks `std::hypot` and `%ll`/`%z`; run `tools/probe-retro68.sh` after core changes.
- Test the tests: mutation-check new tests; several passed while testing nothing.
- ETOPO file is `~/scratch/globe-data/etopo/` (row 0 is south); Natural Earth ZIPs are in
  `~/scratch/globe-data/naturalearth/`; neither is committed.
- Owner behavior rules: window moves and exposes never re-render; a click without a drag changes
  nothing; two releases only (floppy, full); no relay.
