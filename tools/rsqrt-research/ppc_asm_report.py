#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Compile ppc_probe.cpp with a PowerPC cross GCC at several -mcpu settings and report, per candidate,
instruction count and the library calls it makes. Compile-only. usage: ppc_asm_report.py <g++> <outdir>"""
import re, subprocess, sys, json, os
cxx, outdir = sys.argv[1], sys.argv[2]
os.makedirs(outdir, exist_ok=True)
here = os.path.dirname(os.path.abspath(__file__))
report = {"compiler": subprocess.check_output([cxx, "--version"], text=True).splitlines()[0], "cpus": {}}
for cpu in ["generic", "603e", "750", "7400"]:
    flags = ["-O2", "-std=c++20", "-fno-exceptions", "-fno-rtti", "-S", "-I", here]
    if cpu != "generic": flags += [f"-mcpu={cpu}"]
    asm = os.path.join(outdir, f"ppc_probe-{cpu}.s")
    subprocess.check_call([cxx] + flags + [os.path.join(here, "ppc_probe.cpp"), "-o", asm])
    text = open(asm).read()
    funcs = {}
    for m in re.finditer(r"^(f_\w+):\n(.*?)^\s*\.size\s+\1,", text, re.S | re.M):
        body = [l.strip() for l in m.group(2).splitlines()]
        ins = [l for l in body if l and not l.startswith(('.', '#')) and not l.endswith(':')]
        calls = sorted({re.sub(r'@.*', '', l.split()[1]) for l in ins if l.startswith('bl ')})
        fpops = [l.split()[0] for l in ins if l.split()[0].startswith('f')]
        funcs[m.group(1)] = {"instructions": len(ins), "calls": calls,
                             "fp_ops": {o: fpops.count(o) for o in sorted(set(fpops))}}
    report["cpus"][cpu] = {"flags": " ".join(flags), "functions": funcs}
json.dump(report, open(os.path.join(outdir, "ppc-asm-report.json"), "w"), indent=1)
for cpu, d in report["cpus"].items():
    print(f"== -mcpu={cpu}")
    for f, v in d["functions"].items():
        print(f"  {f:16s} {v['instructions']:3d} ins  calls={v['calls']}  fp={v['fp_ops']}")
