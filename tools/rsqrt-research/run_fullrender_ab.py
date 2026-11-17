#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Interleaved A/B/B/A whole-render timing of the scratch variants built by build_variants.sh.

usage: run_fullrender_ab.py <workdir> <out.json> [--blocks N] [--cpu C] [--radii a,b] variant...
Each run is a fresh process (rsqrt-trace). Raw per-run seconds and the frame-chain hash are kept.
The first block is a discarded warm-up. Blocks alternate forward and reverse variant order."""
import argparse, json, os, statistics, subprocess, sys

ap = argparse.ArgumentParser()
ap.add_argument("workdir"); ap.add_argument("out")
ap.add_argument("--blocks", type=int, default=15); ap.add_argument("--cpu", default="1")
ap.add_argument("--radii", default=""); ap.add_argument("--reps", default="1")
ap.add_argument("variants", nargs="+")
a = ap.parse_args()
runs = {v: [] for v in a.variants}; hashes = {v: set() for v in a.variants}
for blk in range(a.blocks + 1):
    order = a.variants if blk % 2 == 0 else a.variants[::-1]
    for v in order:
        cmd = ["taskset", "-c", a.cpu, f"{a.workdir}/rv{v}/build/rsqrt-trace", "--time", a.reps]
        if a.radii: cmd += ["--radii", a.radii]
        r = json.loads(subprocess.check_output(cmd))
        hashes[v].add(r["chain_hash"])
        if blk > 0:  # block 0 is the warm-up
            runs[v].append(r["render_seconds_total"])
res = {"radii": a.radii or "all", "blocks": a.blocks, "cpu": a.cpu, "reps": a.reps, "variants": {}}
base = statistics.median(runs[a.variants[0]])
for v in a.variants:
    xs = runs[v]
    res["variants"][v] = {"median_s": statistics.median(xs), "min_s": min(xs), "max_s": max(xs),
                          "rel_to_first_pct": 100 * (statistics.median(xs) / base - 1),
                          "hashes": sorted(hashes[v]), "raw_s": xs}
json.dump(res, open(a.out, "w"), indent=1)
for v, d in res["variants"].items():
    print(f"{v:5s} median {d['median_s']*1000:9.2f} ms  min {d['min_s']*1000:9.2f}  max {d['max_s']*1000:9.2f}  vs {a.variants[0]} {d['rel_to_first_pct']:+6.2f}%  hashes {d['hashes']}")
