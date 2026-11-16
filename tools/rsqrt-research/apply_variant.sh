#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Usage: apply_variant.sh <git-rev> <variant> <outdir>
# Exports <git-rev> into <outdir>, routes inverse_length() through rsqrt_lab::dispatch(), and adds
# the research trace target to the *scratch* CMakeLists. Never touches the working tree.
set -eu
rev=$1; variant=$2; out=$3
here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
mkdir -p "$out"
git -C "$repo" archive "$rev" | tar -x -C "$out"
mkdir -p "$out/tools/rsqrt-research"
cp "$here"/*.hpp "$here"/*.cpp "$out/tools/rsqrt-research/"
python3 - "$out" <<'PY'
import sys,re
p=sys.argv[1]+"/src/render/render.cpp"
s=open(p).read()
old="double inverse_length(double l2) {\n"
assert s.count(old)==1
s=s.replace(old, old+"#ifdef GLOBE_RSQRT_VARIANT\n    return rsqrt_lab::dispatch(l2);\n#endif\n")
s=s.replace('#include "globe/raster.hpp"','#ifdef GLOBE_RSQRT_VARIANT\n#include "render_variant.hpp"\n#endif\n#include "globe/raster.hpp"',1)
open(p,"w").write(s)
old2="        const double wanted = std::ceil(std::sqrt(ratio2));\n"
assert s.count(old2)==1
s=s.replace(old2,"#ifdef GLOBE_RSQRT_VARIANT\n        ++rsqrt_lab::g_steps_sqrt;\n#endif\n"+old2)
open(p,"w").write(s)
c=sys.argv[1]+"/CMakeLists.txt"
t=open(c).read()
t+="\nadd_executable(rsqrt-trace tools/rsqrt-research/rsqrt_trace.cpp)\ntarget_link_libraries(rsqrt-trace PRIVATE globe_embedded_atlas globe_platform_common globe_clock_posix)\n"
open(c,"w").write(t)
PY
echo "$variant" > "$out/.variant"
