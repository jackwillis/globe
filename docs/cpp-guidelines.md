# C++ Guidelines — Poodle Earth

**Status:** Proposed engineering guidance; not a replacement for accepted ADRs or `AGENTS.md`.

Poodle Earth uses **portable C++20** to run an offline geographic explorer on resource-constrained and historically supported computers. Write readable code with predictable costs, explicit ownership, deterministic behavior, and a clear boundary between portable logic and native operating-system integration.

These guidelines guide **new and modified code**. They do not authorize a repo-wide refactor, new release scope, or changes to an accepted architecture decision. Where guidance conflicts, follow `AGENTS.md`, accepted ADRs, and the relevant platform contracts; flag contradictions rather than silently changing policy.

## 1. Prefer straightforward C++20

- Prefer value types, `enum class`, small functions, `constexpr`, RAII, `std::array`, `std::span`, `std::string_view`, `std::optional`, and `std::unique_ptr` **when the supported toolchains provide them**.
- Prefer explicit, readable algorithms to clever template machinery. Use templates when they eliminate duplication or improve type safety; avoid abstractions justified only by possible future use.
- Do not introduce exceptions or RTTI. The core is built with `-fno-exceptions -fno-rtti`; use an explicit success/failure contract instead. Avoid RTTI-dependent designs and exception-based library interfaces.
- Do not introduce a C++20 feature merely because it exists. Modules, coroutines, ranges, and other library features need a demonstrated use case and successful supported-target builds. C++20 language mode is **not** proof that all C++20 library facilities exist in every target's standard library.
- Prefer the established codebase conventions. Do not rewrite functioning code solely to adopt a more fashionable idiom.

## 2. Ownership, views, and lifetimes

- Use RAII for owned resources and one clear owner where possible. Prefer `std::unique_ptr` for exclusive dynamic ownership; use shared ownership only when a concrete lifecycle requires it.
- Treat pointers, references, `std::span`, and `std::string_view` as borrowed views unless a specific API documents otherwise. State which object owns the backing bytes and how long they remain valid.
- Never retain a borrowed view across atlas replacement, pack eviction, framebuffer recreation, or a deferred operation without proving that the owner survives. Prefer a stable ID, offset, or an explicit owned copy at these boundaries.
- Keep ownership transfers visible through return values and move operations. Avoid hidden global ownership and undocumented aliasing.
- Treat resource-accounting tickets and actual resource lifetimes as one invariant; a move, cancellation, or early return must not lose or double-count a charge.

The existing `Atlas` is a useful model: it validates a caller-owned buffer once and retains a bounded, non-owning view. The existing `Framebuffer` illustrates exclusive ownership and explicit resource accounting.

## 3. Bounded memory and predictable work

- Make memory growth visible. Bound viewport dimensions, counts, decompression sizes, arrays, per-job workspaces, and derived arithmetic **before allocating**.
- Allocate and reserve at startup, pack loading, framebuffer recreation, or **explicit refinement-job creation**. Ordinary frame drawing and input handling should not grow containers or perform surprise allocations.
- `std::vector` and `std::string` are allowed, but capacity growth must be intentional in memory-sensitive paths. Avoid accidental copies of large collections and temporary buffers.
- Be explicit about allocation failure. Under `-fno-exceptions`, an ordinary allocation or standard-container growth is **not automatically a recoverable error**. Use an appropriate fallible allocation mechanism in paths required to handle low memory, and test those paths.
- Every new rendering allocation should document its trigger, maximum size, accounting category, and release/cancellation behavior.
- Keep interactive work bounded and interruptible. Counted work units alone do not guarantee an elapsed-time bound; measure relevant hardware when asserting responsiveness.

## 4. Preserve deterministic application logic

- Keep the Elm-style **`Model / Msg / update / Effects`** boundary. Core decisions depend on explicit model state and messages, not wall clocks, platform events, filesystem state, or hidden mutable singletons.
- Inject time and other external observations through platform messages or specified interfaces. No direct clock reads, networking, or OS/window-system headers in `include/globe`, `src/core`, `src/data`, or `src/render`.
- Treat published atlas data and per-job view plans as logically immutable. Mutate framebuffers and dedicated workspaces where necessary, without compromising replayability.
- Honor generation/cancellation tokens: stale or duplicate completion must not mutate the currently presented view. A repaint should reuse valid pixels rather than restart the render.
- Follow accepted rendering decisions such as `docs/adr/0002-rendering-shared-code-and-usability.md`; changes to pixel order or refinement semantics require explicit review and golden-test evidence.

## 5. Numeric and geographic correctness

- Document units and coordinate systems (degrees versus radians, world versus screen coordinates, pixel pitch versus byte count). Prefer named types or clearly named conversions where confusion is plausible.
- Check finite floating-point inputs, ranges, and overflow before integer conversions or indexing. Clamp **before** narrowing. Do not rely on undefined signed overflow.
- Validate latitude and longitude assumptions, including antimeridian, poles, zoom extremes, and off-screen geometry.
- Keep the semantics of rounding, clipping, line direction, and rasterization stable unless a visible change is intentional and tested.
- Avoid `-ffast-math` or comparable semantics-changing optimization by default. If proposed, evaluate as a separate experiment with cross-compiler pixel, numerical, and hardware evidence.
- Do not mistake identical host outputs for a proof of cross-architecture floating-point equivalence. Preserve deterministic fixture and replay checks across the supported environments.

## 6. Parse external bytes defensively

- Treat every atlas and edition pack as untrusted input regardless of its provenance. Validate magic, version, counts, offsets, lengths, checksums, and structural invariants before constructing views.
- Check integer overflow **before** multiplication or addition when calculating sizes; require the entire requested span to lie within the buffer.
- Decode byte order explicitly. Do not reinterpret packed file bytes as a native C++ struct or depend on host alignment or endianness.
- Keep readers bounded, side-effect-free where practical, and clear about ownership. Reject unknown versions and malformed records with explicit errors, not unchecked fallback behavior.
- Test zero-length, truncated, oversized, corrupted, misaligned, and endian-sensitive cases. See `include/globe/atlas.hpp` for existing contracts.

## 7. Keep platform adapters small and honest

- Portable code may depend on the C++20 subset proven by supported toolchains; it must not depend on GCC-only, Clang-only, POSIX-only, Windows-only, or Apple-only behavior without a justified, isolated adapter.
- Put QuickDraw/Toolbox, X11, future FLTK, Win32, and browser integration under their respective platform boundaries. Keep conversion of native pixels, native cursors, clocks, event loops, and file operations there.
- Prefer a little concrete adapter duplication to a large speculative cross-platform interface. Extract shared behavior only after two actual use cases show a stable abstraction.
- Treat **GCC and Clang as primary independent host compilers**; maintain a supported compiler matrix as feasible. Retro68's GCC cross-build is a distinct compatibility gate. An MSVC build can be evaluated when Windows support warrants it.
- Compilation for a target is not a claim of successful execution, compatibility, usability, or performance on that target.

## 8. Style and reviewability

- Follow the checked-in `.clang-format`; don't restyle unrelated files. The checked-in `.clang-tidy` is the project's current curated lint baseline, not a mandate to enable every available check.
- Use meaningful names and describe non-obvious invariants, ownership, data provenance, or numerical choices in comments. Avoid comments that merely restate the syntax.
- Keep changes narrow. Prefer one clearly testable feature or correction over a simultaneous subsystem redesign. Avoid adding frameworks, interfaces, configuration switches, or generic utilities without a concrete consumer.
- Keep source licensing headers consistent with repository policy. Do not introduce new runtime dependencies or third-party data/licenses without required authorization.

## 9. Test the contract that changed

Start with focused tests, then applicable host and target checks. A typical host verification sequence is:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Use the existing policy/format/lint workflows and run the supported GCC/Clang variants when relevant. Use ASan/UBSan on hosts for memory or undefined-behavior-sensitive changes. For portable changes, run the repository's Retro68 compatibility probe when available; for Classic-specific changes, use the actual cross-build and packaging checks. **Never access the G4 without explicit owner authorization.**

- **Rendering:** deterministic traces and goldens; protect cancellation, resize, re-entry, incremental refinement, and final-frame semantics. When pixels intentionally change, state why and compare against a suitable independent oracle or reviewed expected result.
- **Memory:** bounded allocation, budget accounting, ownership/eviction behavior, and low-memory failures.
- **Data formats:** property/boundary tests and malformed-data rejection.
- **Performance:** exact build, compiler, optimization flags, input, hardware/OS, timing method, and memory measurements; distinguish host, cross-build, emulator, and physical-hardware evidence.

Do not alter golden images or benchmark baselines merely to make CI green. Record the intended semantic change and its evidence.

## 10. Review checklist for contributors and coding agents

Before requesting review, be able to answer:

1. **Behavior:** What user-visible or internal contract changed? What is intentionally unchanged?
2. **Ownership:** Who owns each new buffer/view/resource, and what invalidates it?
3. **Bounds:** What are the worst-case memory and work costs? Are allocations permitted at that phase?
4. **Portability:** Which C++20 facilities, standard-library features, and platform boundaries are involved? Which actual compilers/targets were checked?
5. **Correctness:** Which edge cases, numeric assumptions, invalid inputs, and stale operations were tested?
6. **Evidence:** Which exact commands passed, on what system? What remains untested, especially on Classic Mac OS and the G4?
7. **Scope:** Did this PR solve the assigned problem without an unrelated abstraction or policy change?

### Related project documents

- [`AGENTS.md`](../AGENTS.md) — binding agent safety, authority, and invariants
- [`docs/architecture.md`](architecture.md) — as-built layers and behavior
- [`docs/toolchain.md`](toolchain.md) — documented target toolchains and evidence
- [`docs/agents/codex.md`](agents/codex.md) — agent testing instructions
- [`docs/adr/0002-rendering-shared-code-and-usability.md`](adr/0002-rendering-shared-code-and-usability.md) — accepted rendering decisions
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) — background reference, not an additional blanket CI policy
