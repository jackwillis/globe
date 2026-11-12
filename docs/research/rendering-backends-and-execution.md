# Rendering backends, execution and display adaptation — exploratory design

**Status:** Research hypotheses, 2026-11-12; neither a backend selection nor an approved refactor. Preserve the current single-threaded, cooperative RGB565 CPU renderer as the **reference implementation and fallback**.

See [architecture](../architecture.md), [ADR 0002](../adr/0002-rendering-shared-code-and-usability.md), [refinement order](../refinement-order.md), [text layout](../design/text-layout.md), [G4 measurements](../perf/renderer-integration-20261110.md), [#2 rendering epic](https://github.com/jackwillis/globe/issues/2), [#18 latency](https://github.com/jackwillis/globe/issues/18), [#34 quality tiers](https://github.com/jackwillis/globe/issues/34), [#59 diagnostics](https://github.com/jackwillis/globe/issues/59), [#97 falsification](https://github.com/jackwillis/globe/issues/97), [#55 memory ledger](https://github.com/jackwillis/globe/issues/55), and the cross-platform note in [PR #111](https://github.com/jackwillis/globe/pull/111). An experiment program is proposed separately (do not assume it is merged).

## Three independently variable concerns

1. **Render implementation:** software CPU rasterization, potential OpenGL *geometry* rendering (not merely upload a CPU bitmap), or other APIs if evidence warrants.
2. **Execution strategy:** current bounded cooperative continuations; potentially a background/parallel CPU executor. Changing the executor does not require rewriting the rasterizer. Neither all jobs nor the OS 9 Toolbox can safely be treated as thread-safe by default.
3. **Presentation:** QuickDraw GWorld/CopyBits, XImage, or an optional GPU upload/display path. Presentation throughput is *not* identical to geometric rendering throughput.

Also separate **visual look** (Map/Relief), **geographic/detail quality** (geometry/terrain/label density), **display format** (1/2/4/8/16/32-bit output capabilities) and **backend selection**. A monochrome screen need not imply low geographic detail; a slow true-color CPU may still need reduced detail.

## Existing implementation, not greenfield

- `Scene` caches geometry and caps; `Framebuffer` holds RGB565 pixels plus a coverage bitplane.
- `RefinementJob` has bounded, cancellable work phases; the limb pass preserves a particular final overlap order. `RefineRunner` owns shared lifecycle and `ZoomPreview` caches terrain. `ShownFrame` handles overlays during presentation.
- OS 9 converts RGB565 -> QuickDraw RGB555 and presents in bounded, adaptive CopyBits slices. Shared `pixel_format` already converts RGB565 to several X11 16/24/32-bit layouts.
- Line-first refinement already has a logical/private RGB565 line layer, with sparse-pixel composition. Do not infer that each conceptual layer requires a screen-sized buffer.
- Existing target timing and memory evidence must remain distinct from new hypothetical GPU/SIMD gains.

## Proposed logical layer description

Background/terrain -> geographic vectors (coasts/borders/rivers) -> place symbols -> cartographic labels -> highlights/measuring -> transient presentation annotations.

Layer description should specify **semantics, ordering, geometry, style, visibility, clipping and resource lifetimes**. A backend may consume different prepared representations: the CPU path may need screen-space spans while a shader-based GPU path could transform geometry itself. Do not require every backend to execute the identical projection implementation. Stable identities and hit-testing remain consistent.

**Text:** a possible glyph-mask compositing backend should receive fonts and metrics from native glyph providers, while the existing native text drawing remains the baseline. Positioned text runs or glyph commands are a logical layer, not necessarily an allocated bitmap. Never stamp text permanently into the terrain cache or re-scale text from a zoom-preview snapshot.

## Execution and ownership model

The Elm-style main update owns authoritative application state. Workers, if introduced, receive immutable inputs or exclusively owned mutable workspaces with explicit byte budgets; they publish results via a main-loop message **only when the generation and resource tokens match**. A cooperative executor must remain fully functional. Any parallel work must preserve final-image rules, stable cell/limb draw ordering and cancellation safety; parallel direct writes to a shared framebuffer are unsafe without deterministic ownership or compositing. A worker cannot call arbitrary classic Toolbox services; OS 9 Multiprocessing Services are only a research target after runtime/API audits.

Prefer bounded pending work, explicit cancellation, stable memory peaks, and a deterministic reference path over a framework-like thread-pool API. Benchmarks must include worker stack/scratch memory and UI wake latency.

## Backend capabilities and fallback

A future capability probe may describe: available color depths, graphics API and **actual** hardware-accelerated renderer, maximum texture/viewport, supported pixel formats, memory ceiling, optional worker threads, and font/glyph services. A driver advertising OpenGL is not proof of hardware acceleration. An early CPU + OpenGL texture-presenter experiment tests *presentation*, not GPU terrain rasterization.

Suggested selection order: always support the CPU renderer; benchmark/accept an optional accelerated implementation on each target; use a safe fallback on initialization or allocation failure. Avoid automatic display-depth changes, unconsented preference writes or live backend switching until resource-lifetime tests exist. Hide irrelevant options on platforms with one implementation.

## Pixel fidelity and tests

The CPU reference retains exact cross-run hashes and existing final terrain/limb invariants. An independent GPU implementation might differ at edges because of clipping, precision, interpolation and antialiasing; compare geometric/semantic invariants and visual tolerances instead of asserting pixel equality across unlike backends.

Measure separately: scene preparation, projection, visible geometry, cell/triangle fills, line drawing, glyph creation/cache hit, overlay composition, RGB565 conversion, upload/CopyBits, GPU command completion, swap, first useful frame, complete presentation, input latency and memory peaks. Record whether measurements are headless versus on-screen.

## Initial evidence gates

| Experiment | What must be true to proceed |
| --- | --- |
| Timing decomposition | Repeatable stage timings and actual display submission on the G4, controlled workload/hashes |
| AltiVec kernels | Scalar reference, auto-vectorized and explicit SIMD kernel; real end-to-end win with correct pixels and measured scratch/ABI costs |
| Hardware GL probe | Renderer/vendor/version/driver and evidence of *hardware* acceleration on NetBSD, Tiger or another actual OS |
| GPU experiment | A/B: CPU/native vs CPU/GL presentation; B/C: CPU/GL presentation vs GL geometry; matched scene/OS and controlled synchronization |
| Adaptation | Distinguish color-depth conversion cost from low/medium/high geometry fidelity, display readability and resource budget |
| Glyph composition | Native drawing vs native-font bitmap masks with bounded cache; fonts not redistributed |
| Parallel work | Single-worker vs bounded multiworker with semantic equivalence, no data races, proper cancellation and G4-safe fallback |

Do not adopt a generic scene-graph/render-graph framework, multiple GPU APIs, Vulkan, Direct3D or a general-purpose UI library merely to future-proof the project. A second *real* implementation should reveal the necessary API.

**Decision gate:** an accepted design change requires correct behavior, consistent provenance, actual platform measurements, documented memory impact and a reviewable implementation issue. Do not alter v0.1 release obligations.
