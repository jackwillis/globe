# Commands and observed results — Linux x86-64, 2026-11-18

Working directory: /tmp/globe-aa-review-corrections, isolated worktree in private scratch clone.
GCC 16.2.1, Clang 22.1.8, NumPy 2.5.3. CMake Release -O3 -DNDEBUG;
C++20, -fno-exceptions -fno-rtti. No renderer timers invoked.

```sh
cmake -S /tmp/globe-aa-review-corrections -B /tmp/aa-corrections-core -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/aa-corrections-core --target globe_core -j 4
cmake -S /tmp/globe-aa-review-corrections/tools/aa-experiment -B /tmp/aa-corrections-build -G Ninja -DGLOBE_CORE_BUILD=/tmp/aa-corrections-core -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/aa-corrections-build -j 4
ctest --test-dir /tmp/aa-corrections-build --output-on-failure
python3 tools/aa-experiment/validate_oracle.py /tmp/aa-corrections-build/aa_synthetic_driver /tmp/aa-corrections-evidence
python3 tools/aa-experiment/review_instruments.py
python3 tools/aa-experiment/statistics_plan.py
mkdir /tmp/aa-corrections-fixtures
/tmp/aa-corrections-build/aa_fixtures /tmp/globe-aa-review-corrections /tmp/aa-corrections-fixtures > /tmp/aa-corrections-fixtures/verification.jsonl 2> /tmp/aa-corrections-fixtures/verification.log
python3 tools/aa-experiment/prepare_blind.py /tmp/pe-aa-phase3b-fixtures /tmp/aa-corrections-blind /tmp/aa-corrections-private/identity.json
cmake -S /tmp/globe-aa-review-corrections/tools/aa-experiment -B /tmp/aa-corrections-san -G Ninja -DGLOBE_CORE_BUILD=/tmp/aa-corrections-core -DAA_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/aa-corrections-san -j 4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/aa-corrections-san --output-on-failure
cmake -S /tmp/globe-aa-review-corrections/tools/aa-experiment -B /tmp/aa-corrections-clang -G Ninja -DGLOBE_CORE_BUILD=/tmp/aa-corrections-core -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++
cmake --build /tmp/aa-corrections-clang -j 4
ctest --test-dir /tmp/aa-corrections-clang --output-on-failure
python3 -m py_compile tools/aa-experiment/oracle_rows.py tools/aa-experiment/validate_oracle.py tools/aa-experiment/review_instruments.py tools/aa-experiment/statistics_plan.py tools/aa-experiment/prepare_blind.py tools/aa-experiment/run_reference.py
git diff --check
```

All builds/CTest pass. Original aa_tests remains 206836 checks, including 196608 blend-table
checks; do not represent all as geometry cases. 50 numeric B/oracle controls pass reference
convergence; numerical approximation errors are reported, not asserted away. Independent row
versus slow membership, analytic rectangle/disc/color-order and metric/bootstrap controls pass.
50/50 raw synthetic outputs match GCC Release versus ASan/UBSan and versus Clang Release.
Imported production core was not sanitizer-instrumented. LeakSanitizer disabled for the known
environment restriction; no leak-clean or target-runtime claim. The new Python oracle is not
covered by C++ sanitizers.

Exporter: 32/32 exact A/production replay and terrain budgets 1/7/256; all 128 regenerated
geometry/background/A/B files byte-identical to original fixtures. Comparison scripts used
Python byte equality (all files) and subprocess output equality (all 50 frozen synthetic cases).

Provenance recovery: git fsck --no-reflogs --unreachable, git cat-file -s/-p on candidate blobs,
SHA256 comparison against pre-image-source-freeze.json; neither instrument version recovered.
No historical semantics/diff claim is supported. The new freeze lists every current instrument.

Container availability checks `podman images --format '{{.Repository}}:{{.Tag}}'` and
`docker images --format '{{.Repository}}:{{.Tag}}'` both failed before startup: read-only
/run/user/1000/libpod sticky-bit requirement. Retro68/aa.cpp compile probe not run. No machine
permissions changed. No full CTest/CI, geographic reference generation/scoring, timings,
G4 access, production changes, PR merge or release.
