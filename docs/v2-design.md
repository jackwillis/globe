# Globe — Architecture and Design Specification

Status: **proposed**, 2026-11-01. This document describes a fresh implementation; the existing G4/NetBSD X11 prototype is a **reference**, not a dependency or a codebase to copy wholesale.

## 1. Product vision

Globe is a tiny, offline-first, software-rendered, interactive 3D Earth for PowerPC G4 running NetBSD/X11 and native Classic Mac OS 9. It displays a complete, useful coarse Earth immediately, and improves the view progressively, without relying on any infrastructure operated by the project author.

Future functionality includes place metadata, flat cartographic mode, local routing, and Ruby-authored custom map layers. They are **out of scope for the first renderer milestone**.

## 2. Non-negotiable requirements

- **Responsive manipulation:** During camera movement, render coarse globe outlines at an interactive target cadence. Never await network, tile parsing, or terrain generation in an input handler.
- **Progressive refinement:** After input stops, render increasingly detailed cached geometry; if necessary, acquire additional permitted data asynchronously. Preserve a coherent coarse view throughout.
- **View-dependent LOD:** Allocate detail according to projected screen-space error, visibility and budgets. Include horizon clipping, back-hemisphere rejection, viewport culling and projected-size thresholds.
- **Offline-first:** Embedded minimal atlas and persistent local converted data cache. Network failures cannot prevent startup or basic navigation.
- **Zero maintainer-hosted infrastructure:** No project-operated server, tile service, database or proxy. Source adapters for independently hosted data, with configurable endpoints and attribution, and manual local file import.
- **Native platforms:** Portable modern C++ core; X11 and OS 9 Toolbox/QuickDraw frontends. No OS-specific calls in the geographic or rasterization core.
- **Deterministic design:** Elm-inspired Model/Msg/Update/Effects, injectable clock and I/O, replayable message traces, and generation-tagged asynchronous jobs.
- **Memory discipline:** Fixed and measurable budgets; bounded parsing, preallocated workspaces where possible, no unbounded queue or hidden large allocation.

## 3. The floppy challenge

Stretch distribution target: fit the **classic Mac OS 9 application plus a minimal global atlas** on a 1.44 MB high-density floppy disk image, provided target hardware can read that media. This means *launching the application from an installed Mac OS*, not booting an operating system from the floppy.

- Treat 1.44 MB as a strict packaging experiment, **not** a verified current binary size; HFS filesystem overhead and Mac file resource forks reduce usable payload.
- Prefer a small 1-bit/low-color UI asset set, a compact world coast/border dataset quantized into a custom embedded format, no full dynamic scripting VM, and no bundled TLS trust store for the floppy edition.
- Optional network pack, detailed atlas and Ruby features may ship as additional archives/installations; do not burden the minimal launch path.
- Provide a normal disk/archive release even if the floppy target proves impractical.
- Measure compiled application and packed data before promising floppy compatibility. Keep a CI packaging size report; optionally enforce a budget on an explicitly designated `floppy` build.

## 4. Interaction-quality state machine

Camera interaction, idle time and available data determine the **render plan**, not raw network availability.

| Stage | Entry | Display | Work budget |
|---|---|---|---|
| Moving | Input events or camera animation | Bundled/cached coarse contours; major borders; minimal labels | Highest frame priority, stop costly refinement |
| Settle | No camera input for a short tunable interval (starting experiment: 120–200 ms) | Refine currently cached tile geometry and labels | Cancel/deprioritize outdated jobs |
| Refine | Camera remains stable | Higher LOD geometry, lakes/rivers, actual elevation shading | Incremental bounded tasks |
| Decorate | Stable view and spare budget | Cached/prebaked coherent noise, softer land/ocean shading, optional atmosphere | Lowest priority; never required for completeness |
| Fetch | Missing desirable data, source allowed | Retain best local fallback while downloading/converting | Token bucket, request deduplication, retries/backoff |

**Important:** Rendering is event-driven. Do not render 30 idle frames/sec when nothing changes. Settle timers control refinement scheduling, not minimum response latency. Camera interactions should invalidate expensive screen-space work but not destroy reusable decoded geographic tiles.

### Scheduling priority

1. Pump OS events; update camera; present a responsive frame.
2. Decode/prepare the *visible center* at coarse or missing LOD.
3. Refine visible regions by estimated image contribution.
4. Layout important labels and selected feature overlays.
5. Generate cached terrain shading and procedural texture.
6. Fetch permitted missing detail, prefetch near viewport, write cache, prune disk.

Remote I/O must use nonblocking/polled interfaces on OS 9; a cooperative scheduler cannot make blocking sockets nonblocking merely by wrapping them in a job.

## 5. LOD and globe projection

- Start with **orthographic projection** of a unit sphere and a virtual trackball; consider perspective later.
- Convert geodetic latitude/longitude into Earth-centered unit vectors, transform into camera space, test visibility against the horizon, clip crossing segments, then project into pixel coordinates.
- Segments that are geographically long must be geodesically subdivided as needed; handle antimeridian and poles without discontinuities.
- Index tiles using geographic bounds plus conservative 3D bounding volumes. Avoid full geometry decode for invisible tiles.
- Precompute multi-resolution geometry offline for bundled layers. For downloaded detailed geometry, convert and simplify locally with topology-aware care. Cache derived native tiles.

### Screen-space priority / quality

Candidate tile priority should consider: `visible_fraction`, maximum projected error in pixels, estimated screen occupancy, feature importance, and temporal stability. A conceptual priority is:

`priority ~ max(0, pixel_error - target_error) * projected_area * semantic_weight`

This is **not** a final formula; benchmark it. Because orthographic projection foreshortens near-horizon features, use projected dimensions or bounds, not simply geographic distance from the central lat/lon. Preserve visible silhouette coastlines even when their projected area is small; they strongly affect perceived globe shape. Use LOD hysteresis to prevent popping when zoom thresholds or rotations fluctuate. Use a common parent geometry during asynchronous child refinement to avoid temporary holes.

Target per-frame geometry, pixels written, and frame time are more important than maximizing vertex count. Dynamically reduce LOD when the frame budget is missed.

## 6. Geometry and appearance

Layer order: ocean background -> land fills -> relief/shading -> coastlines -> political boundaries -> rivers/lakes -> cities and labels -> selection/annotation/route overlays.

The first implementation should use a small, optimized 16-bit RGB565 framebuffer and nearest/bounded scanline primitives. Offscreen-buffer ownership and QuickDraw pixel layout are delegated to platform adapters. For an 800x600 RGB565 frame, one buffer requires 960,000 bytes (~938 KiB); a second full buffer doubles this. Benchmark actual copy/blit costs.

**Perlin/coherent noise:** not a replacement for real elevation. Precompute or lazily calculate object-space (sphere-continuous) textures into reusable caches. Do not evaluate multi-octave noise per output pixel on each drag frame. A later elevation data adapter can supply accurate relief; noise provides cartographic texture. Provide a **no-effects** quality profile.

## 7. Data and infrastructure

### Bundled

An atlas builder on a modern machine creates a tiny, deterministic global package from appropriately licensed Natural Earth geometry. Validate and quantify size at build time. Bundle provenance and attribution information (Natural Earth is public domain).

### Optional online detail

Provider interface: `discover -> request (bounded size/range) -> decode -> normalize -> simplify/quantize -> atomic cache commit`.

Initial prospective sources: Natural Earth release files and geoBoundaries for borders; Wikidata for on-demand place metadata. Detailed OSM-derived data is a future adapter. Do **not** assume public OSM tile services permit bulk downloads or unrestricted offline scraping; respect each provider's usage terms and attribution. PMTiles range-request support is possible only where a suitable archive and policy actually exist.

Capabilities include streaming parsing with size limits; cancellation, rate limits, dedupe, conditional requests, response validation, `Retry-After`, exponential backoff, ETag/Last-Modified where supported, and per-provider caps.

### Cache

- RAM cache of decoded visible geometry plus retained parent LOD, with explicit entry and total size budgets, pinned active tiles and eviction.
- Disk cache of source/converted tiles, content version, provider, license/attribution, source timestamps, and integrity checks.
- Atomic writes/rename after validation. Never use a partially converted tile.
- Offline mode, clearable cache, human-readable cache statistics, and source selection.
- Designed to tolerate disappearance or policy changes of all network providers.

## 8. Modern C++ / Elm Architecture

- Target modern C++20 where Retro68's chosen compiler *and standard library* support it; maintain compile probes for `std::variant`, `std::span`, `std::optional`, constexpr code, exceptions/RTTI assumptions and relevant ABI features.
- `Model` contains lightweight canonical UI/app state: camera, selection, navigation history, quality, tile state handles. No copied framebuffer or world geometry.
- `Msg`: user input, timer fired, tile ready, parse failed, cache committed, data provider throttled, etc.
- `update(Model&, Msg) -> Effects` is deterministic when clock/random/network inputs are passed in messages. Commands are validated and side effects executed outside update.
- Effect executor is cooperative on OS 9 and may use alternative scheduling on POSIX as long as message order/state semantics remain testable.
- `View` builds a rendering request; renderer consumes a read-only snapshot and stable resource handles. Avoid a virtual-DOM framework.
- `constexpr` for lookup tables, flags, palette generation and format specialization; not huge compile-time geographic datasets.

## 9. Repository structure

```
include/globe/        public portable interfaces
src/core/            model, update, projection, LOD, scheduler
src/render/          CPU rasterization and clipping
src/data/            decoding, atlas, providers, caches
src/platform/x11/    native X11 adapter
src/platform/macos9/ Classic Mac Toolbox / QuickDraw adapter
tools/atlas-builder/ modern-machine preprocessing
tests/               unit, property, golden-frame, decoder-fuzz fixtures
docs/                design, ADRs, measured perf
.github/workflows/   host CI and optional cross-compile CI
```

Do not require the prototype in the new build graph. Add a link and a migration/inspiration note when its actual repository URL is known. Do not copy code until reviewed for design and licensing.

## 10. Engineering quality gates

- Build: CMake, Ninja where available; Linux/NetBSD host builds and Retro68 proof-of-life builds.
- Format: `clang-format` pinned version and style; formatting check in CI.
- Lint: `clang-tidy` on host targets; compiler warnings `-Wall -Wextra -Wpedantic` plus justified per-target exceptions.
- Tests: camera projection; horizon crossing; antimeridian; LOD/hysteresis; tile queues; budget accounting; corruption/truncation; replay determinism.
- Sanitizers: ASan/UBSan on supported modern hosts; fuzz input parsers on modern host, not the G4.
- Benchmarks: 467 MHz G4 on NetBSD 11/X11 and native OS 9. Track time to coarse-first-frame, frames during drag, settle-to-refinement, rendered segments, peak app memory, disk bytes, network requests and idle CPU.
- Continuous size report for app binary and embedded atlas. Avoid making unmeasured speed/memory promises.

## 11. Milestones and acceptance tests

**M0: Skeleton and portable projection.** CI builds host core, lint and tests; endian-safe core types; initial README and architecture. A test camera projects a known center point into center screen and clips the rear hemisphere.

**M1: Responsive globe.** Port *concepts* from the reference prototype; continuous rotate/zoom with a bundled coarse atlas; no network dependency. Measure drag-frame latency on G4.

**M2: Staged refinement.** A stable camera triggers cached finer outlines with no blank tiles; motion immediately returns to coarse mode; tile scheduling doesn't starve the UI.

**M3: Center-weighted LOD and persistent cache.** Screen-error-based detail and hysteresis; visible-region budget; simulated corrupted/missing cached tiles recover gracefully.

**M4: Optional data acquisition.** One bounded public source adapter; chunked download and local conversion; offline restart reproduces refined area; policy-compliant failure modes.

**M5: Presentation and terrain.** Cached relief/noise/shading, native Mac UI, stress/size profiling. Floppy build feasibility measured, not assumed.

## 12. Open decisions / explicit non-goals

- Whether 1.44 MB is viable for the specific OS 9 build, fonts and world outline corpus.
- Exact quantization and tile format; measured memory target; text rendering backend; precomputed versus on-device terrain representation.
- Ongoing API compatibility and license/attribution for each provider.
- Future flat map, routing, Ruby scripting, custom fantasy maps **are not blockers** for M0–M3.
- No 3D triangle mesh engine, full terrain elevation solver, roads or photorealistic imagery for first launch.

## 13. Recommended first agent task

Implement **M0 only** in this clean repository, preserving portability. Add a pure C++ orthographic projector with comprehensive tests and a tiny framebuffer API, format/lint CI, and a Retro68 feature probe. Do not rewrite the old prototype, add network I/O, or choose a complex custom tile format yet. Keep commits narrow; record toolchain/compiler limitations and measured results.
