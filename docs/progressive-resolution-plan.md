# Progressive terrain resolution — implemented 2026-11-05

The full X11 edition embeds one-degree and half-degree elevation. Motion remains plain vectors. Settling chooses the same zoom target as before (coarsest grid meeting a four-pixel equatorial-cell target, otherwise the finest available), then renders distinct available resolutions from coarse to that target. At ordinary zoom it stops at one degree. At higher zoom it paints half-degree terrain over the coarse image. Both terrain levels retain circle-outward reveal; graticule/coastlines/borders reveal once after the final terrain level. The OS 9 floppy retains its one-degree payload.

Explicit X11 --elevation imports retain the original coarse base plus up to three user-supplied packs; they replace the optional built-in fine pack. Later equal-resolution imports win. The combined terrain-cache budget stays 32 MiB; optional built-in fine allocation failure retains the coarse fallback with a diagnostic.

## Model and ownership boundaries

`start_refinement(Model&)` and `update` deterministically emit `Effects.refine = Refine{generation, level}`. A platform executes the request using the selected immutable pack and mutable rendering workspace. `RefinementFinished{generation, level}` advances to the next distinct width; obsolete, duplicate and out-of-order completions do nothing. Input/resize invalidates the entire sequence. Changing resolution retains the camera generation. Frontends start from effects rather than detecting stage entry. The `refined GENERATION LEVEL` trace verb records completions.

The model contains resolution choices and completion state, without pixels, file paths or clocks. Source bytes remain stable and outlive their views/caches. Renderer scratch remains preallocated and mutable. Each platform owns one current job. Existing generation and workspace-token checks reject obsolete writes.

## Safe bitmap replacement

A subsequent level preserves the existing bitmap during preparation and circle passes. The final canonical terrain pass records coverage in a preallocated, row-aligned one-bit framebuffer plane. A bounded row repair restores background/ocean only where the finer terrain did not write. This removes coarse islands and differing limb chords without clearing away the fine image. Fine land and fine ocean are both drawn. Lines follow repair. There is one terrain framebuffer, rather than a retained bitmap per resolution; the existing platform presentation/overlay buffers retain their own roles.

The bitplane costs 38,400 bytes at 640 x 480 and is allocated with the framebuffer, not during a job. `byte_size()` remains native pixel bytes for copying; `coverage_byte_size()` reports scratch separately. Normal rasterization leaves coverage off. There is still no allocation inside RefinementJob::step.

## Data and costs

The committed `data/elevation-0.5deg.gelv` is 720 x 360, 259,752 bytes, SHA-256 `b51499b5c0a4dc09dd5ac62d27b6a520033b2411c858b71728e5e5df0f5e7906`. Two complete conversions from the existing pinned ETOPO 2022 60s surface source plus current pinned Natural Earth 50m lakes reproduced identical bytes. No additional source download was needed. Provenance and reproduction are in data/SOURCES.md.

Both global terrain caches retain 17,970,422 logical bytes including the class grid, excluding framebuffer/scratch, source bytes, allocator and platform overhead. A finer elevation grid improves relief and elevation-derived land/water detail; biome classes remain one-degree detail and vectors remain Natural Earth 110m. Finer biome sampling could be rebuilt from the existing RESOLVE source after generalizing its typed grid/builder. Finer coastlines/borders need separate vector inputs. Quarter-degree exceeds the present cache budget with the current representation and is not added.

## Measured tradeoff and validation

Nine-repeat host comparison at high zoom: progressive compute 42.22 ms, first completed terrain ring 1.47 ms, complete coarse terrain 9.55 ms. Finest-only compute 31.64 ms and first completed ring 5.69 ms. The before-change finest renderer took 33.01 ms. Final checksums agree. Normal one-degree compute was 12.22 ms versus 13.35 ms before, with equal checksums; these sequential samples do not establish a speedup. See [raw measurements](perf/progressive-resolution-host.json). Benchmarks exclude presentation and scheduled line-reveal sleeps; first-ring timing is a concrete proxy, not a claim about subjective usefulness or input latency. `--refine-finest` is the benchmark oracle; `--refine` exercises the sequence.

47 host tests pass, including real half-degree progressive/fresh full-pixel comparisons at two zooms in both styles, adversarial coarse/fine land/ocean grids at polar/limb views, protocol ordering/cancellation, trace roundtrips and an actual two-job X11 run that returns to quiet idle. Existing one-level Map/Relief pins remain unchanged. Focused ASan/UBSan/LeakSanitizer tests pass. PowerPC/68k compile probes, the OS 9 package/fork/size checks, formatting and policy checks pass. Small math portability fixes in the integrated overlay code retain finite-input behavior on 68k.

[OS 9 scheduling audit](os9-scheduling.md) covers the snapshot presentation continuation and Tiger evidence. G4 fine-resolution compute/presentation timing and native OS 9 acceptance remain unmeasured. The OS 9 floppy intentionally does not gain the finer data workload.
