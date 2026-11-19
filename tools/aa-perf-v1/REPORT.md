# PE-AA-01-PERF pilot: steering handoff

2026-11-19, Refs #25 (does not complete it). Claude Code. Worktree `/home/jack/code/globe-aa-perf`,
branch `claude/pe-aa-perf-pilot-20261119`, stacked on PR #196 head `bb7d8af6eef091c24c7656f4c2d2af1ea2f3e6c2`
(decision.py SHA-256 `ef237ea3…5ccbd`; amendment v1 `9a2214c`). New directory `tools/aa-perf-v1/` only.
Synthetic inputs only; `decision.py` is executed unmodified and checked by hash on every run.

**The G4 has not been touched.** All measurements below are Linux x86-64. There is no NetBSD/macppc
datum, so every statement about the G4 is a stated unknown or an explicitly labelled sensitivity.

## Primary question and the honest answer today

*Can the exact-rational decision mathematics process realistic-sized conditions within acceptable
memory and computation budgets on NetBSD/macppc?* **Not yet answerable.** On the Linux host the answer
is yes by a wide margin (one 74,796-pixel condition extrapolates to about 41–72 s CPU and about 130 MiB).
Whether the G4 stays within budget depends on one number this pilot cannot produce without the owner run:
the G4's slowdown factor against this host (table in §Sensitivity).

## Observed data (Linux x86-64, Python 3.14.8, stdlib only, 3 repetitions per cell)

Host: AMD Ryzen 5 6600H, 12 logical CPUs, 15 GiB RAM, little-endian, 64-bit, 30-bit Python digits.
**Not a quiet host**: 1-minute load 3.4 at start and 4.0–5.3 during the run, other agents' jobs running,
and `/proc/meminfo` showed 7.9 of 8.0 GiB swap in use before the run began (unrelated to the harness; the
measured processes recorded `ru_majflt = 0`, `ru_nswap = 0`). Repetition spread stayed small (stdev/mean
0.4–2.1% at stage D), but absolute timings carry that interference. Full machine-readable data:
`results/linux-x86_64-ladder.jsonl` (all 20 cells, every repetition, environment, rusage) and
`results/linux-x86_64-fullframe-bookkeeping.jsonl`; regenerate the tables with `analyze.py summary --md`.
Every one of the 20 cells reproduced its pinned input and exact-result digests (`digest_check: match`).

Evaluation CPU, milliseconds per masked pixel (median of 3), `decision.condition()` only:

| Class | A 100 | B 1,000 | C 5,000 | D 10,000 | log-log exponents A→B→C→D | linear fit R² |
|---|---:|---:|---:|---:|---|---:|
| exact (0 uncertain) | 0.55 | 0.52 | 0.52 | 0.55 | 0.98, 1.00, 1.08 | 0.9992 |
| narrow (≤1/2560) | 0.67 | 0.71 | 0.70 | 0.70 | 1.03, 0.99, 1.00 | 1.0000 |
| wide (≤1/64) | 0.95 | 0.88 | 0.93 | 0.93 | 0.97, 1.04, 0.99 | 0.9999 |
| crossing (every channel straddles a tie) | 0.88 | 0.89 | 0.90 | 0.97 | 1.00, 1.01, 1.10 | 0.9988 |
| mixed (30/30/20/20) | 0.79 | 0.71 | 0.73 | 0.77 | 0.95, 1.02, 1.07 | 0.9994 |

Other measured quantities (stage D, 10,000 masked pixels in a 41,072-pixel frame):

* Wall time ≈ CPU time (CPU/wall 0.988–0.992): the instrument is CPU-bound, single-threaded.
* Input construction 0.12–0.14 s (about 13 µs per masked pixel, under 2% of evaluation); digest 0.02 s.
* Aggregate `decide()` over 16 geometry IDs × 2 styles from one evaluated condition: about 1 ms. Negligible.
  (It reuses one condition; it is not 32 independent evaluations.)
* Peak resident memory: 20.9 MiB at interpreter + import baseline; 33.8–35.7 MiB after stage D (growth
  12.8–14.8 MiB). Growth is linear: about 135 B per unmasked frame row plus about 0.9 KiB per masked row,
  because this harness holds every row in a Python list.
* Full-frame bookkeeping probe (not a full condition): 307,200 rows with only 100 masked pixels evaluate in
  0.49–0.52 s wall (about 1.6 µs per frame row, gross) and reach 60.5 MiB peak (growth 39.6 MiB).

## Analysis

1. **Scaling.** Cost is linear in masked pixels to within measurement noise (exponents 0.95–1.10, fit R² ≥
   0.9988). The mild upward drift at stage D for `crossing`, `exact` and `mixed` (1.07–1.10) is of the size
   the host's load could produce; one run on a busy machine cannot separate noise from a real super-linear
   term, so linearity is "supported", not "established".
2. **Interval complexity.** Cost per pixel orders as exact 0.55 < narrow 0.70 < mixed 0.77 < wide 0.93 <
   crossing 0.97 ms: tie-crossing intervals cost about 1.75× an exact pixel, because they split into several
   quantizer cells. Exact pixels still cost half a crossing one, so the all-zero case is not a free lunch.
3. **RAM on the G4.** On x86-64 the measured model predicts about 130 MiB for one fully materialized
   74,796-pixel condition (21 baseline + 40 frame rows + 75k × 0.9 KiB). That is below 256 MB physical but
   above the harness's default 128 MiB cell limit, and the 32-bit PowerPC figure is unmeasured (smaller
   pointers and 24-byte ints should reduce it; allocator and OS overhead are unknown). Whether the real
   pipeline holds rows or streams them (`BoundsStream` in aa-interval-v1) was not measured here, so this is
   an upper-style estimate for this harness. The ladder up to stage D needs about 35 MiB on Linux.
4. **Is ~146 s per condition plausible?** The PR #196 estimate was 1.9 ms per uncertain pixel. This host
   measured 0.70–0.97 ms per uncertain pixel (mixed 0.77 ms over 70% uncertain), i.e. 52–72 s for 74,796
   pixels, 2–2.7× faster. That does not contradict 146 s as an upper figure on this host; it shows the earlier
   number was a different, slower synthetic frame or a busier moment (the earlier inputs are not in this
   branch, so the cause is unknown). For the certified instrument, every pixel is "precise at radius 1/2560",
   which resembles the `narrow` class (0.70 ms; 52 s per condition; 0.46 h for 32 conditions on this host).
   Certificate interval shapes were not read, so that resemblance is an assumption.
5. **What would justify an optimization experiment.** Only a measured G4 per-pixel cost whose projected
   32-condition total is unacceptable, or a memory peak near the G4's limit. Neither exists yet. Nothing
   here was optimized, and nothing in these data argues for a mathematical change.

### Sensitivity: the unknown G4 factor (illustrative, not a prediction)

If the G4 were X times slower than this host's `narrow` rate (0.70 ms per pixel). No claim is made about X.

| Factor X | Stage C (5,000 px) per repetition | Stage D (10,000 px) per repetition | one 74,796-px condition | 32 conditions |
|---:|---:|---:|---:|---:|
| 20 | 70 s | 2.3 min | 17 min | 9.3 h |
| 50 | 3.0 min | 5.8 min | 44 min | 23 h |
| 100 | 5.8 min | 11.7 min | 1.5 h | 47 h |

The owner's stage A smoke test gives a first, startup-dominated estimate of X in about a minute of G4 time and stage B refines it, which is why the procedure starts there.
Uncertainty compounds: Python version and build on the G4, 32-bit big-int digit costs, cache and memory
bandwidth, and certificate interval distributions can each move X.

## Not run, not claimed

* No NetBSD/macppc execution of any kind; its Python version, `ru_maxrss` units, `swapctl` output format,
  `setitimer` guard and `RLIMIT_CPU` behaviour are assumptions (NetBSD reports KiB, as Linux does) that the
  smoke test is designed to confirm. `--rlimit-data-mib` is untested everywhere except as a no-op.
* No cross-host comparison (no second host). `analyze.py` computes ratios only for cells with identical input
  and result digests, and the digests are the portable-correctness check for big-endian 32-bit PowerPC.
* No certificate, geographic, candidate, bootstrap, fine-resolution or full 74,796-pixel run; stage D
  (10,000) ran on Linux only because Linux had healthy A–C and a large budget margin.
* Not run: CMake/CTest, Retro68, any CI beyond the local commands below. The inherited clang-format failure
  in `tools/aa-experiment/synthetic_driver.cpp` is unchanged and unrelated.
* The timer is process CPU and `perf_counter` wall; resolution is 1 ns here and unknown on NetBSD.
* One interpreter build, one busy host, one run of the committed baseline (an earlier identical run produced
  the digest pins and agrees within the same noise).

## Commands and results (Linux x86-64, Python 3.14.8)

```sh
python3 -B -m unittest discover -s tools/aa-perf-v1 -p 'test_*.py' -v      # 30 tests OK (24 at the baseline commit), ~2.6 s
python3 -B tools/aa-perf-v1/run.py selftest                                # selftest: ok
python3 -B tools/aa-perf-v1/run.py ladder --label linux-x86_64 --stages A,B,C,D --allow-busy --out results/linux-x86_64-ladder.jsonl   # 191 s, 20/20 cells ok, digests match
python3 -B tools/aa-perf-v1/run.py ladder --label linux-x86_64-fullframe-bookkeeping --stages A --classes exact,mixed --frame 307200 --allow-busy --out results/linux-x86_64-fullframe-bookkeeping.jsonl
python3 -B -m unittest discover -s tools/aa-decision-v1 -p 'test_*.py'     # parent suite (see PR)
sha256sum -c tools/aa-perf-v1/source-freeze.sha256
bash tools/check-policy.sh
```

## Recommended next action (one)

Steering review of this harness and `NETBSD-PROCEDURE.md`. If approved, the owner runs only the
read-only preconditions, `selftest` and the stage A smoke test on the idle G4 (procedure §§1–3) and returns
the output. That single result confirms Python/instrumentation compatibility and digest correctness on
PowerPC and gives the slowdown factor X; stages B and C and any decision about condition-scale feasibility
or optimization follow from it, each separately authorized.
