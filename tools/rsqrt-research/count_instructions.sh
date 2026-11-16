#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Deterministic whole-render instruction counts (callgrind "Collected") per scratch variant.
# usage: count_instructions.sh <workdir> <radii|all> variant...      host x86-64 only; not G4 evidence
set -eu
work=$1; radii=$2; shift 2
for v in "$@"; do
  arg=""; [ "$radii" != all ] && arg="--radii $radii"
  n=$(valgrind --tool=callgrind --callgrind-out-file=/tmp/cg.$$ "$work/rv$v/build/rsqrt-trace" $arg 2>&1 >/tmp/cg.$$.out | sed -n 's/.*Collected : \([0-9]*\).*/\1/p')
  echo "{\"variant\":\"$v\",\"radii\":\"$radii\",\"instructions\":$n,\"trace\":$(cat /tmp/cg.$$.out)}"
done
rm -f /tmp/cg.$$ /tmp/cg.$$.out
