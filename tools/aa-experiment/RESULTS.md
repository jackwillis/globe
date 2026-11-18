# PE-AA-01 Phase 3B — prototype handoff, 2026-11-18

**Ready for implementation/plan review, not accepted for production or a scored run.**
No geographic quality score, performance measurement, G4 access, merge, release or PR was performed.
No production source, default, accepted ADR, golden pin, workflow or milestone was changed.

## Implementation

A calls the existing production draw_line unchanged. B is 136 lines of first-party portable C++20
plus a 40-line interface, compiled with exceptions and RTTI disabled. It forms width-one segment
rectangles, round interior joins and butt caps, uses a fixed distance-profile coverage approximation,
accumulates maximum coverage in one byte-per-pixel source-feature mask, and blends once per feature.
Exact consecutive duplicate vertices are skipped without removing nonzero short edges. All visible
chains of a source feature share the union mask. Reversal canonicalizes rectangle arithmetic;
different source features retain production order. Invalid features are rejected before writes.

The approximate filter is intentionally exposed to rejection: it is not exact area coverage or
exact signed distance within intersecting primitive unions. No inference of visual effectiveness,
relative cost, Classic behavior or adoption follows from passing synthetic correctness.

B scratch payload is exactly **307200 bytes at 640x480**, plus a 16-byte workspace object on this
x86-64 host. A caller budget and nothrow allocation bound setup; injected null and insufficient-budget
tests pass. There is no explicit allocation during rendering. Geometry/terrain/framebuffers are
shared experiment memory, not included in the candidate scratch claim. The host extractor reserves
bounded pools (262144 points, 32768 chains, 8192 features), checks overflow and stops on failure.

## Evidence

Host: Linux x86-64; GCC 16.2.1 and Clang 22.1.8. Release defaults -O3 -DNDEBUG; no fast-math,
LTO, architecture tuning or diagnostic-relaxation flags. Standalone GCC and Clang C++20 builds pass.
Clang links the same pinned GCC-built baseline core; this is a B/interface host compatibility check,
not an independently Clang-built full application or a PPC claim.

| Check | Evidence |
|---|---|
| Synthetic C++ correctness | 206836 checks, zero failures; short strokes, phases, all octants, reversal, endpoint values, mixed-color RGB565 rounding, joins, closed chains, self-overlap, duplicate features, invalid coordinates, viewport/row clipping, budget/null allocation |
| Independent reference | Exact half-endpoint rectangle, reversal, per-feature union, colored last-writer overlap, disc area/symmetry and bounded little-endian reader controls pass |
| Geographic A replay | **32/32 exact every-pixel matches** against unchanged production lines-at-end refinement |
| Terrain-only fixture stability | **32/32 identical at work budgets 1, 7 and 256** |
| Serialized geometry | **32/32 parsed under explicit caps/coordinate/flag/trailing-byte checks** |
| Sanitizers | B and harness ASan/UBSan tests and 32-fixture export pass; baseline core remains uninstrumented |
| Sanitized determinism | **128/128 raw background/A/B/geometry files byte-match Release** |
| Existing production checks | framebuffer, render, shaded, terrain, line_layer: **5/5 pass**; pins untouched |
| Formatting/policy | clang-format 22.1.8 on four experiment C++ files; git diff --check; tools/check-policy.sh pass |
| Native diagnostics | **64 PNGs, each 640x480**; Map/Relief, all frozen views/radii; concealed X/Y labels, identity key separate |

LeakSanitizer is disabled because of the established ptrace restriction. No clean leak-check claim,
whole-application sanitizer coverage, Retro68 compile or native hardware acceptance is made.
Only the relevant production tests were run; no claim of the complete CTest suite or CI is made.

The images have been checked for successful native-size decoding, not scored or treated as visual
acceptance. Open review.html at 100% browser zoom; record observations before opening
PRIVATE-identity.json. Full source/fixture/output hashes are in the evidence manifest. The image
labels use fixed seed 20261118; no B tuning followed geographic images.

## Reproduction and frozen plan

Use the exact commands in README.md, with private fresh paths. The core and experiment build
separately; the experiment is not added to the root application build. The 32 views/styles use the
same production data, camera, effective palette, background and serialized projected geometry for
both arms. Read PROTOCOL.md for source hashes, semantic boundaries, independent sample oracle,
pixel masks, ratio-of-means gap-closed rule, four-block ABBA/BAAB schedule and rejection criteria.
There is intentionally no executable performance runner or geographic scoring command in this phase.

Current hold: independent review of the prototype, native images and frozen plan. A later authorized
scored run still needs the full colored geographic oracle's convergence controls, a reviewed timing
runner, matching provenance and an explicit execution decision. B can be rejected for ineffective
quality, lost contrast/detail, >25% line-path median overhead, memory or portability failure.

## Deviations and limits

* AGG arm C is deferred with both complete Phase 3A evidence archives linked unchanged in PROTOCOL.md.
  Promising chain interiors are retained as a qualified result; endpoint/overlap acceptance is not
  retroactively granted. No further AGG source or adapter work occurred.
* The three-arm schedule is replaced by paired A/B blocks with exact run-count arithmetic; no samples
  have been taken. Numeric thresholds are inherited proposals awaiting review, not learned limits.
* The original repository's .git is read-only. A --no-hardlinks private clone and fresh linked worktree
  at /tmp/globe-aa-ab-phase3b isolate the task; no other agent worktree/build was touched. Live GitHub
  REST confirmed the baseline SHA; the optional Project view was not inspected.
* A new test initially failed compilation due to an omitted workspace argument in the harness; fixed
  before fixture admission. No baseline or decision threshold was altered to make tests pass.
* A full independent geographic reference is specified and its straightforward sample-membership
  implementation supplied, but running/converging it over the geographic matrix is outside this
  phase. Native images are diagnostics, not a substitute for that later measurement.

Next specific step: independently review B's coverage model and concealed image pairs, then decide
whether to authorize the frozen scored A/B experiment. No hardware step is implied.
