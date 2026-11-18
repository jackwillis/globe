# PE-AA-01 Phase 3B: host-only A/B prototype

This directory is an isolated experiment, not an application feature. It does not participate
in the root build, change production sources or defaults, bundle AGG, add CI automation or
grant hardware access. Read [PROTOCOL.md](PROTOCOL.md) before execution.

Current authorization: synthetic correctness, production replay verification, fixture export
and native diagnostic images. **Do not run a scored quality or performance benchmark yet.**
The protocol was recorded before implementation at SHA-256
`f6135e62c3cb28fca2fd85b043466e1b0aebb6f4aea25d9fca3639252c670624`.
Review B, the reference and the measurement plan first. A full scored runner is intentionally absent.

## Architecture and memory

* `aa.hpp`/`aa.cpp`: portable first-party distance-profile AA, no clocks/I/O/platform headers,
  C++20, exceptions/RTTI disabled. One byte per pixel, nothrow setup and caller budget.
  Reuse one coverage buffer; max accumulation per source feature, then blend once. Visible
  chains of a source feature share the buffer; different features retain production order.
* `fixtures.cpp`: bounded host-only extractor and verifier. Calls unchanged production terrain
  and `draw_line` for A. Reproduces the pinned arc projection path only inside this experiment.
  Caps: 262144 points, 32768 chains and 8192 features; input files <=16 MiB. Allocation/overflow
  failures stop generation rather than publish incomplete evidence. These common geometry
  pools and terrain/framebuffers are not counted as B's incremental coverage scratch.
* `test_aa.cpp`: short strokes, reversals, fractional positions, joins, loops, repeated features,
  invalid coordinates, clipping and injected allocation-failure checks. These are correctness
  controls, not geographic quality scores.
* `reference.py`: independent analytic membership and high-resolution last-writer sample oracle.
  Its small rectangle/disc/color-order controls run now; full geographic scoring does not.
  No NumPy, Pillow or third-party graphics library is needed.
* `images.py`: bounded serialized-geometry verification, stdlib PNG export, concealed X/Y labels
  and hashes. Seed 20261118 is fixed before image generation. `PRIVATE-identity.json` is separate
  from the review HTML; reviewers should record observations before opening it.

B's mask is 307200 bytes at 640x480, plus a 16-byte workspace object on this x86-64 host.
There is no per-feature explicit allocation. The byte budget covers mask payload, not allocator
overhead, framebuffer/geometry memory or future Classic partition requirements. Failure injection
returns nullptr before drawing; invalid geometry is checked in full before any output writes.

Approximation limitation: B estimates coverage from primitive signed distances. It does not compute
exact union area or exact signed distance inside overlapping unions. In particular short-stroke
contrast, oblique width, joins, overlap and motion can still fail the independent quality criteria.
No quality benefit or speedup has been established.

## Reproduce correctness and fixtures

Use fresh build/output paths; do not borrow another agent's artifacts. From repository root:

```sh
cmake -S . -B /tmp/pe-aa-phase3b-core -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLOBE_BUILD_TESTS=ON -DCMAKE_DISABLE_FIND_PACKAGE_X11=ON
cmake --build /tmp/pe-aa-phase3b-core --target globe_core -j4
cmake -S tools/aa-experiment -B /tmp/pe-aa-phase3b-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLOBE_CORE_BUILD=/tmp/pe-aa-phase3b-core
cmake --build /tmp/pe-aa-phase3b-build -j4
ctest --test-dir /tmp/pe-aa-phase3b-build --output-on-failure
python3 tools/aa-experiment/reference.py
mkdir /tmp/pe-aa-phase3b-fixtures
/tmp/pe-aa-phase3b-build/aa_fixtures "$PWD" /tmp/pe-aa-phase3b-fixtures > /tmp/pe-aa-phase3b-fixtures/verification.jsonl 2> /tmp/pe-aa-phase3b-fixtures/verification.log
python3 tools/aa-experiment/images.py /tmp/pe-aa-phase3b-fixtures "$PWD"
```

Compiler flags in this host Release build are CMake's `-O3 -DNDEBUG`, with C++20/no exceptions/no RTTI.
No wall-time measurements are taken. A later comparison must pin identical flags for both arms and
record the compiler and binary hash. Core-build CMake ownership is checked by the standalone build.

The exporter verifies all 32 fixtures against unchanged lines-at-end rendering before writing the
paired output. It also checks terrain-only output with work budgets 1,7,256. It records zero quality
metrics and uses no renderer timing clock. Serialized `.geometry` files are explicitly little-endian,
including binary64 coordinates. Raw `.rgb565` files are little-endian 640x480 values. The oracle parser
checks size, counts, flags, coordinates, truncation and trailing bytes. Images remain exactly 640x480.

Allocation/undefined behavior checks use a separate standalone configuration with `-DAA_SANITIZE=ON`.
The imported baseline core is unchanged; only B and the experimental harness are instrumented in that
configuration. Use `ASAN_OPTIONS=detect_leaks=0` in this environment because LeakSanitizer cannot run
under its ptrace restrictions. This is not a clean LeakSanitizer result or target acceptance.

## Review boundary

Review the frozen quality target, the distance approximation, sample counts and cost gates. Evaluate
native images with concealed identity. The later test must be able to reject B for weak quality,
lost/faint detail, poor motion behavior, >25% line-path median overhead, resource failure or absent
portable behavior. Do not revise thresholds or tune B after scored results without a dated amendment.
The future oracle needs full colored-geography convergence verification and the timed runner needs
independent review before execution. No G4/Classic compatibility follows from host tests.
