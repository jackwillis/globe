# PE-AA-01 numerical-instrument performance pilot (PE-AA-01-PERF)

2026-11-19, Refs #25. Research-only, synthetic-only, standard library only. Measures the
reviewed `tools/aa-decision-v1/decision.py` (PR #196 at `bb7d8af`, decision.py SHA-256
`ef237ea3…5ccbd`) **unmodified**. It changes no threshold, quantizer, interval mathematics or
certification criterion, reads no geographic input and performs no optimization. This is the
offline Python scientific instrument, not Poodle Earth's interactive renderer.

**Hardware boundary:** the harness is authored and exercised on the Linux development host only.
Running it on the G4 is the owner's step; see `NETBSD-PROCEDURE.md`. Nothing here connects to any
machine.

## Files

| File | Role |
|---|---|
| `perf.py` | SplitMix64 workload generator, digests, cell measurement, optional OS instrumentation |
| `run.py` | `selftest`, `preflight`, bounded `ladder` (one child process per cell), internal `cell` |
| `analyze.py` | `summary` (tables, scaling, extrapolation, cross-host), `expected` (writes pins) |
| `expected.json` | SHA-256 pins of every standard cell's inputs and exact results (Linux x86-64) |
| `test_perf.py` | 24 controls: determinism, class properties, limits, ladder gating, fallbacks |
| `results/` | Linux baseline JSONL and logs; `netbsd-macppc-*` files arrive from the owner run |
| `NETBSD-PROCEDURE.md` | Exact owner-run commands, limits, abort and return steps |
| `REPORT.md` | Observed data, extrapolations, limitations, one recommended next action |
| `source-freeze.sha256` | Hashes of the reviewed decision.py and these files |

## Workloads

Integer-only SplitMix64 seeded from `SHA-256("PE-AA-01-perf-v1|class|masked|frame|seed")`, so
Python version, word size and byte order cannot change the stream. Intervals use the certified
denominator `255·4^16`; narrow radius is the certified 1/2560, wide is 1/64.

| Class | Masked-pixel intervals |
|---|---|
| `exact` | zero width; a quarter sit exactly on an expanded RGB565 level, the rest uniform |
| `narrow` | width ≥ 2 units, radius up to 1/2560 around a uniform centre |
| `wide` | radius up to 1/64 |
| `crossing` | every channel strictly straddles one half-up quantizer tie (R, B at 31, G at 63) |
| `mixed` | 30% exact, 30% narrow, 20% wide, 20% crossing per pixel |

Candidates A and B are bytes near the interval midpoint (A ±8, B = A for 10%, else A ±12), so
neither is a degenerate all-zero input. The frame defaults to the certified mask density
(`ceil(masked × 307200 / 74796)`), so full-frame bookkeeping scales with the numerical workload;
`--frame N` fixes it independently. Unmasked rows share one exact-zero tuple. Stage sizes:
A 100, B 1,000, C 5,000, D 10,000 masked pixels. **No stage reaches the 74,796-pixel
certified mask, and none is authorized.**

## What is measured, per cell (workload × stage), per repetition

* **Input construction** (`build_wall_s`, `build_cpu_s`) and **input digest** time, once per cell,
  reported apart from evaluation.
* **Numerical evaluation**: one `decision.condition()` call (wall via `perf_counter`, CPU via
  `process_time`), and its result digest.
* **Aggregate decision evaluation**: `decision.decide()` over 16 geometry IDs × 2 styles built from
  that one evaluated condition with `dataclasses.replace`. It times only the aggregation arithmetic,
  not 32 independent evaluations.
* **Peak resident memory**: `ru_maxrss` at start, after construction and after evaluation, plus
  page-fault, swap and context-switch deltas from `getrusage`. Every cell is a fresh interpreter, so
  a peak is that cell's own.
* **Throughput**: masked px/s and ms per masked and per uncertain pixel (median of repetitions).
* Load average before and after; swap in use between stages.

Every individual repetition is stored in the JSONL; `analyze.py` adds min / median / mean / max /
population stdev, a least-squares fit with R², log-log exponents between stages and a labelled
extrapolation to 74,796 pixels.

## Correctness across hosts

Each cell records an input digest and result digests (canonical JSON of the exact `Fraction`
endpoints). `expected.json` pins them from the Linux baseline. A host that is not bit-for-bit
identical gets `digest_check: MISMATCH`, status `digest_mismatch`, and the ladder stops.
`analyze.py` forms a cross-host CPU ratio only where input and result digests are equal.

## Bounds and abort

Per cell: wall limit (1 Hz in-child timer, plus a parent timeout 30 s later), `RLIMIT_CPU`, and a
resident-memory limit checked at the same 1 Hz tick. Optional `--rlimit-data-mib` is off by default
(untested on NetBSD). Per ladder: a total wall budget, projection before every stage advance
(linear × 1.25) against the cell, memory and total budgets, stop on any non-ok cell, stop if swap in
use grows by more than 8 MiB, refuse to start if the 1-minute load exceeds 1.0 (`--allow-busy`
overrides). Stage D must be named explicitly. Abort with Ctrl-C, `touch <out>.ABORT`, or `kill` of
the printed parent PID; completed cells stay in the JSONL.

Every OS facility is optional: no `resource` → no RSS or rusage; no `setitimer` → only the parent
timeout; no `/proc`, `swapctl` or `sysctl` → null fields. Nothing uses `timeout`, `/usr/bin/time`,
GNU utilities, root, NumPy, pytest or any third-party package. Python 3.7+ syntax is enforced by a
test (`ast.parse(feature_version=(3, 7))`). The harness writes only the `--out` file;
`sys.dont_write_bytecode` keeps `__pycache__` off the host.

## Reproduce on Linux

```sh
python3 -B -m unittest discover -s tools/aa-perf-v1 -p 'test_*.py' -v
python3 -B tools/aa-perf-v1/run.py selftest
python3 -B tools/aa-perf-v1/run.py ladder --label linux-x86_64 --out /tmp/linux.jsonl --stages A,B,C,D
python3 -B tools/aa-perf-v1/analyze.py summary /tmp/linux.jsonl --md
sha256sum -c tools/aa-perf-v1/source-freeze.sha256
```
