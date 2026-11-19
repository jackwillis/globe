# PE-AA-01-PERF: G4 stage A smoke test (preliminary)

2026-11-19, Refs #25, Claude Code under explicit owner authorization (stage A only; ordinary `claude`
account over the existing SSH route; no root, installs, services or changes outside `/tmp`, which I removed
afterwards). Harness exactly as PR #202 at `8cedd888eb2a1754f38ba770bac5770785a7e47d`; the archive
(SHA-256 `3ffda16b…409257`) and all nine files in `source-freeze.sha256` verified OK on the G4.
Raw data: `results/netbsd-macppc-A.jsonl`, `results/netbsd-macppc-preflight.json`.

## Environment

NetBSD 11.1 macppc (GENERIC), PowerPC 7400 rev 2.9, 1 CPU, 256 MiB RAM, 512 MiB swap (0 used before and
after), idle (load 0.00, nothing but base daemons and X at the login screen, 0 users), ~95 MiB free by
`vmstat`. Python 3.13.14 (pkgsrc, `/usr/pkg/bin/python3.13`; `python3` is not on the default PATH),
big-endian, 32-bit (`maxsize` 2147483647), 30-bit int digits. `process_time` is `getrusage` (1 µs),
`perf_counter` 30 ns. All instrumentation worked: `resource`, `setitimer` guard, `ru_maxrss`, loadavg.
`swapctl`/`sysctl` live in `/sbin`/`/usr/sbin`, outside the default PATH; the harness found `swapctl`
because PATH was extended.

## Results

* `preflight`: ok. `selftest`: **ok** (25 s): PRNG vectors, decision.py hash, and stage A inputs and exact
  results of all five classes reproduce the Linux pins.
* Stage A ladder (`--reps 1 --cell-wall-s 120 --cpu-s 150 --mem-mib 128 --total-wall-s 900`, outer
  `ulimit -t 900`): 31 s, 5/5 cells `ok`, `digest_check: match` (input, `condition()` result and aggregate
  `decide()` digests identical to Linux). Load rose to 0.57 (1-min), no paging (`pi`/`po` 0), major faults 0,
  swaps 0, swap 0 used afterwards.

| Class | G4 eval CPU s | G4 ms/masked px | Linux CPU s (median of 3) | G4/Linux CPU | G4 build s | G4 aggregate s |
|---|---:|---:|---:|---:|---:|---:|
| exact | 3.010 | 30.1 | 0.055 | 54.6 | 0.063 | 0.049 |
| narrow | 4.082 | 40.9 | 0.067 | 61.4 | 0.076 | 0.050 |
| wide | 5.163 | 51.7 | 0.095 | 54.1 | 0.079 | 0.053 |
| crossing | 5.100 | 51.0 | 0.088 | 58.0 | 0.073 | 0.073 |
| mixed | 4.281 | 42.9 | 0.079 | 54.0 | 0.076 | 0.050 |

Peak RSS: 19.7 MiB at start, 19.7–19.8 MiB after evaluation (growth ≤ 12 KiB; 100 pixels in a 411-row frame).
That `ru_maxrss` is KiB on NetBSD is assumed; 19.7 MiB is plausible for Python 3.13 and was not cross-checked
against `top`. Wall ≈ CPU (CPU/wall ≥ 0.999).

## Preliminary slowdown, and what it does not show

G4/Linux CPU ratio **54–61×** for these cells. One repetition of 100 pixels each (Linux had three, on a busy
host), so the ratio has no measured spread and may include startup or cache bias; it is not a throughput claim.
Applying it to the Linux ladder only as a sensitivity (the REPORT's X table; X≈55 sits between the 50 and 100
rows): about 41 ms per uncertain pixel, so stage B (1,000 px) ≈ 30–52 s per cell and per repetition, stage C
(5,000 px) ≈ 2.5–4.3 min, one 74,796-px condition ≈ 38–65 min and 32 conditions ≈ 20–35 h of CPU. Those last two
figures are extrapolations from 100 pixels on one host and are not evidence; memory at scale on 32-bit is
unmeasured.

Harness issue found (not fixed; the frozen source is unchanged): `swapctl -s` on NetBSD prints 512-byte
blocks, not "KB", so `swap_info()` stores the raw text with `used_kib: null` and the ladder's swap-growth stop
is inactive on NetBSD. I recorded swap by hand (0 used before and after). A fix would be a separately reviewed
change.

## Recommendation

Stage B is justified and low-risk if separately authorized: `--stages B --reps 3` costs about 5 classes × 3 ×
(30–52 s) ≈ 8–13 min of idle G4 time, uses ~25 MiB by the Linux model, and its 100-vs-1,000-pixel comparison
is the first G4 scaling datum. Run it from a fresh stage-A-gated invocation with the same limits; stage C only
after reviewing B. The ladder's own swap guard needs the `swapctl` parsing fix, or manual before/after swap
checks, until then.
