# Owner-run procedure: PE-AA-01-PERF on the NetBSD/macppc G4

2026-11-19, Refs #25. **Not yet authorized to run.** Nothing here has been executed on the G4;
the harness was exercised on Linux only. Run it only after steering review. It installs nothing,
needs no root, changes no system setting, and writes only under the two directories you choose
below. Commands are POSIX `sh`; `PY` is whichever Python 3.7+ interpreter the G4 already has
(`python3`, or e.g. `python3.12`). No `timeout`, `/usr/bin/time`, GNU tool or `/proc` is used.

## 0. Prepare on the laptop (no G4 involvement)

```sh
cd /home/jack/code/globe-aa-perf
git archive --format=tar.gz -o /tmp/aa-perf-pilot.tar.gz HEAD tools/aa-decision-v1 tools/aa-perf-v1
sha256sum /tmp/aa-perf-pilot.tar.gz
```

Hand `/tmp/aa-perf-pilot.tar.gz` to the G4 by your usual route and unpack it (the archive is not
committed anywhere):

```sh
mkdir -p /tmp/aa-perf && cd /tmp/aa-perf && tar xzf /path/to/aa-perf-pilot.tar.gz
mkdir -p /tmp/aa-perf-out
```

## 1. Preconditions (read-only checks; stop if any fails)

```sh
PY=python3
$PY --version                      # needs 3.7 or newer
uptime                             # 1-minute load should be under 0.5 with nothing else running
ps -ax | head -40                  # confirm no demo or build of yours is running
vmstat 1 3                         # free memory should be well over 150 MB; no paging activity
swapctl -l                         # swap in use should be 0 (or at least stable)
df -k /tmp                         # a few MB free
sysctl hw.model hw.physmem64 hw.ncpu
```

If another session is using the G4, or swap is in use, do not run; the measurement would be
untrustworthy and the machine's other work matters more.

## 2. Self-test and environment record

```sh
cd /tmp/aa-perf
$PY -B tools/aa-perf-v1/run.py preflight > /tmp/aa-perf-out/preflight.json
$PY -B tools/aa-perf-v1/run.py selftest
```

`selftest` must print `selftest: ok`. It checks the random-number vectors, that `decision.py` is
the reviewed file, and that stage A inputs and exact results reproduce the Linux digests: the
correctness test for big-endian 32-bit PowerPC. Expect well under a minute or two. If it prints
`FAIL`, stop and send back the output; do not run the ladder.

## 3. Smoke test (stage A, one repetition)

```sh
cd /tmp/aa-perf
$PY -B tools/aa-perf-v1/run.py ladder --label netbsd-macppc --stages A --reps 1 \
    --cell-wall-s 300 --cpu-s 400 --mem-mib 128 --total-wall-s 1200 \
    --out /tmp/aa-perf-out/netbsd-macppc-smoke.jsonl
```

Expect five lines `A <class> ok <seconds>` and `done: ladder complete`. Linux needs about 0.1 s per
cell; a few seconds to a minute on the G4 is unremarkable. While it runs, `top` in another shell
should show the Python child's resident size in the tens of MB (this is a sanity check on the
harness's assumption that NetBSD `ru_maxrss` is in KiB).

## 4. Bounded ladder

Go one step at a time and look at the numbers between steps. Cost is roughly linear in masked
pixels, so each step is about ten times the previous one. Linux measured about 1 ms per masked
pixel for uncertain intervals (`results/`); multiply by however much slower the smoke test showed.

```sh
cd /tmp/aa-perf
# Stages A and B, three repetitions each.
$PY -B tools/aa-perf-v1/run.py ladder --label netbsd-macppc --stages A,B --reps 3 \
    --cell-wall-s 1200 --cpu-s 1500 --mem-mib 128 --total-wall-s 3600 \
    --out /tmp/aa-perf-out/netbsd-macppc-AB.jsonl
```

Only if A and B finished `ok`, `digest_check` matched and memory and speed look acceptable:

```sh
$PY -B tools/aa-perf-v1/run.py ladder --label netbsd-macppc --stages C --reps 1 \
    --cell-wall-s 3000 --cpu-s 3600 --mem-mib 128 --total-wall-s 7200 \
    --out /tmp/aa-perf-out/netbsd-macppc-C.jsonl
```

Stage C at one repetition is the largest authorized step; use three repetitions only if one
repetition was quick. **Stage D (10,000 pixels) and anything larger are not part of this
procedure; ask first.** A full 74,796-pixel condition is not authorized.

Each stage re-checks the previous stage first (`gate ... advance/STOP`) and refuses to advance
when its linear projection (with a 25% margin) would exceed a cell, memory or total budget.
A cell that exceeds its wall limit, RSS limit or CPU limit is recorded and ends the ladder; so
does swap use growing by more than 8 MiB or a digest mismatch.

To run detached from a remote shell, wrap it so the limits survive, and note the PID printed on the
first line:

```sh
( ulimit -t 7200; nohup nice -n 10 $PY -B tools/aa-perf-v1/run.py ladder ... > /tmp/aa-perf-out/run.log 2>&1 & )
```

## 5. Abort

* Foreground: Ctrl-C. The child is killed; finished cells are already in the JSONL.
* Graceful: `touch /tmp/aa-perf-out/<file>.jsonl.ABORT`. The ladder stops before the next cell.
* Hard: `kill <pid printed on the first line>`, then `ps -ax | grep 'run.py cell'` and `kill` that
  one child PID (only that one; do not use `pkill` by name, other sessions may be running).
* Backstop with no action from you: every cell stops itself at its `--cell-wall-s` (1 Hz timer),
  at `--mem-mib` resident, and at `--cpu-s` (`RLIMIT_CPU`).

## 6. Post-run checks and return

```sh
uptime; vmstat 1 3; swapctl -l      # compare with step 1: swap should be unchanged
ls -l /tmp/aa-perf-out
```

Return `/tmp/aa-perf-out/*.jsonl`, `preflight.json`, the `*.log` files and the output of the
step-1 and step-6 commands by your usual route. On the laptop, place the JSONL under
`tools/aa-perf-v1/results/` and run:

```sh
python3 -B tools/aa-perf-v1/analyze.py summary tools/aa-perf-v1/results/*.jsonl --md
```

Clean up when finished (your call): `rm -rf /tmp/aa-perf /tmp/aa-perf-out`. `/tmp` is wiped at
reboot anyway, and the harness leaves no other file anywhere.

## What to send back if something looks wrong

The `selftest` or `ladder` console output, `preflight.json`, and `python3 --version`. A missing
instrument is not a failure: the harness records `null` for unavailable memory, swap or load
figures and carries on. If the G4's environment cannot give trustworthy measurements (Python too
old, load or swap present, selftest digest mismatch), stop and report that instead of forcing the
run.
