# Progressive terrain resolution — proposed plan

Baseline: 1363c43, remote default branch. Runtime behavior is unchanged. The primary checkout's uncommitted day/night changes were not copied into this worktree.

## Intended behavior

Plain vectors while moving, then one-degree terrain, then half-degree terrain when zoom warrants it, all painted progressively into one framebuffer. Reveal coastlines, borders and graticule after the final terrain level. Keep circle-outward refinement, cancellation on input and quiet idle after completion.

## Existing support

GELV supports variable dimensions. X11 imports up to three optional packs beside the embedded grid, constrained by a combined 32 MiB terrain cache budget. quality_for already selects a level using a four-screen-pixel equatorial cell target; later equal-resolution packs override earlier ones. RenderQuality already carries elevation_level. OS 9 currently uses one embedded TerrainScene directly.

There is no Refine message today: Tick enters QualityStage::Refine and platforms construct one job on stage entry. Repeated requests must therefore become independent of stage entry.

## Implementation sequence

1. Build an optional 720 x 360 half-degree GELV with the existing elevation builder's --cell-degrees 0.5 and the pinned lakes input used by the current baseline. Verify source hashes, reproducible output and provenance, then exercise the existing X11 --elevation import. Keep the floppy payload at one degree initially.
2. Extract a pure resolution plan: distinct registered resolutions sorted coarse to fine, stopping at the current zoom-selected target. Preserve equal-resolution override precedence; avoid finer work when zoom does not warrant it.
3. Introduce Refine{generation, level} and RefineFinished{generation, level}, or equivalent typed request/completion effects. Level identifies a registered grid. Store current and target resolution in the model, reuse RenderQuality.elevation_level, and start jobs from explicit effects rather than stage-entry detection. Report completion once. Reject stale, duplicate and out-of-order completions. Keep one camera generation across the sequence; input/resize cancels all remaining work.
4. Separate terrain refinement from final line decoration. Finish each terrain level before requesting its successor. Reveal and repair lines once after the final level. Wire the contract through X11, OS 9 and benchmark/replay paths; a single-pack build runs one terrain step. Preserve paused-drag presentation gating, expose behavior and upload throttling.
5. Implement safe replacement coverage. Current jobs clear the whole background/ocean before drawing, so chaining them unchanged erases the coarse image. Preserve prior pixels between levels, but explicitly replace fine ocean as well as land to remove coarse islands. Different mesh resolutions also have different limb chords. Prototype bounded region replacement (clear and redraw all fine geometry touching a region before presenting it), or a bounded final repair that removes coarse remnants. Compare the visual result with the circle reveal before choosing. A no-clear flag alone is insufficient. Keep one bitmap, without a retained bitmap per resolution.

## Do we need downloads?

No new source download is needed for half-degree elevation. The 60-arc-second ETOPO source has finer samples than either output and is present at ~/scratch/globe-data/etopo/ETOPO_2022_v1_60s_N90W180_surface.nc. The pinned 50m lakes ZIP is present under ~/scratch/globe-data/naturalearth/. Recheck hashes before conversion. docs/elevation-import.md records a prior half-degree pack of 259,752 bytes; rebuild with today's lakes treatment rather than assuming its historical checksum applies.

Finer elevation improves relief and elevation-derived land/water geometry. Biome classes remain one-degree detail because the current grid is sampled into each terrain cache. A finer class grid could be rebuilt from the existing RESOLVE archive, currently present under /tmp/globe-landclass-source/, after generalizing the builder and typed-grid contract. Preserve that temporary source if needed later.

Coastlines and borders remain Natural Earth 110m vectors. Improving their detail is a separate change requiring finer vector inputs, not a prerequisite for progressive terrain.

## Costs and validation

Half-degree has four times the cells of one-degree. Both caches together should consume roughly 17–18 MiB with current structures; measure actual required_bytes and total retained memory. Quarter-degree has sixteen times the one-degree cells and would exceed the current cache budget with today's representation; defer it pending cache changes. More rendering steps increase total settle time, so measure the benefit to time-to-first-image.

Test resolution ordering, selection thresholds, equal-resolution overrides, missing levels and obsolete completion rejection. Require final progressive pixels to equal a fresh finest-only render for both styles, multiple zooms, poles, land/ocean changes and limb/background coverage. Test cancellation within and between levels, resize, paused drag/release, repaint without restart and zero idle wakes. Preserve one-level pins where behavior is unchanged. Measure first coarse image, first finer region, total completion, worst continuation, memory and presentation cost on host and G4, and run frontend/Retro68 checks. Work-unit bounds do not guarantee a wall-clock limit.

## Scheduling prerequisite and data preparation

See [the OS 9 audit](os9-scheduling.md). Initial aggressive scheduling settings regressed Tiger completion time; select settings from measurements rather than assuming more frequent yields are free. Progressive resolution remains proposed until replacement coverage and the explicit request/completion protocol are implemented and verified.

A fresh half-degree build with the current pinned lakes input produced 259,752 bytes, SHA-256 `b51499b5c0a4dc09dd5ac62d27b6a520033b2411c858b71728e5e5df0f5e7906`, retained in `build-scheduling-evidence/elevation-0.5deg.gelv` (ignored build artifact). The host import reports two packs and 17,970,422 bytes of logical terrain cache. The ordinary drag benchmark still selects one degree at its final zoom and preserves `e86d02e2424d2a47`; a high-zoom trace is needed to exercise the finer level. No source download was needed. This optional generated artifact is outside Git and not added to the floppy payload.

Two complete half-degree conversions with the current lakes input were byte-identical. A high-zoom trace (`wheel 1000 20`, then ticks 1150 and 1151) selected level 1 and produced host refined checksum `54122d1e8caefe9b` over two repetitions. This validates the existing import path, not a new multi-resolution sequence.
