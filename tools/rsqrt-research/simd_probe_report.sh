#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Compile-only autovectorization probe. usage: simd_probe_report.sh <outdir>
set -u
out=$1; mkdir -p "$out"; here=$(cd "$(dirname "$0")" && pwd)
run() { # name compiler flags...
  n=$1; shift; c=$1; shift
  $c -O3 -std=c++20 -fno-exceptions -fno-rtti -fopt-info-vec-optimized "$@" -S "$here/simd_probe.cpp" -o "$out/simd-$n.s" 2> "$out/simd-$n.vec.txt"
  echo "== $n ($c $*)"; echo "   vectorized loops reported: $(grep -c 'loop vectorized' "$out/simd-$n.vec.txt")"
  grep 'loop vectorized' "$out/simd-$n.vec.txt" | sed 's/^.*simd_probe.cpp:/   line /' | head
  echo "   vector insns: $(grep -cE '\b(v[a-z0-9]+fp|vperm|vsel|lvx|stvx|mulps|addps|mulpd|addpd|subps|subpd|shufps|unpck|movaps|movups|cmpltps|cmpleps)\b' "$out/simd-$n.s")"
}
run x86_64_O3 g++
run ppc32_O3_7400_altivec powerpc-linux-gnu-g++-12 -mcpu=7400 -maltivec -mabi=altivec
run ppc32_O3_generic powerpc-linux-gnu-g++-12
