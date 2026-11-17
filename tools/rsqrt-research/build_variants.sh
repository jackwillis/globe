#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Usage: build_variants.sh <git-rev> <workdir> [variant...]   (variant "base" = macro undefined)
# Builds rsqrt-trace and globe-line-layer-bench for each variant, identical flags, host compiler.
set -eu
rev=$1; work=$2; shift 2
here=$(cd "$(dirname "$0")" && pwd)
for v in "$@"; do
  d="$work/rv$v"
  rm -rf "$d"; "$here/apply_variant.sh" "$rev" "$v" "$d" >/dev/null
  def=""; [ "$v" != base ] && def="-DGLOBE_RSQRT_VARIANT=$v"
  cmake -S "$d" -B "$d/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLOBE_BUILD_TESTS=ON \
    -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG $def -I$d/tools/rsqrt-research" >"$d/cmake.log" 2>&1
  ninja -C "$d/build" >"$d/ninja.log" 2>&1 && echo "built $v" || { echo "FAILED $v"; tail -20 "$d/ninja.log"; }
done
