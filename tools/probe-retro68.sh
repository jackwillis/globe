#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Compile the portable core and the feature probe with Retro68's PowerPC and 68k compilers inside
# its container. Compile-only (-c): it answers "does this language/library subset build?", not
# "does it run?". Usage: tools/probe-retro68.sh [image]   (CONTAINER=docker to override podman)
set -eu
cd "$(dirname "$0")/.."
CONTAINER=${CONTAINER:-podman}
IMAGE=${1:-${RETRO68_IMAGE:-ghcr.io/autc04/retro68}}
STD=${STD:--std=c++20}
TC=/Retro68-build/toolchain

$CONTAINER run --rm -v "$PWD":/src:Z -w /src "$IMAGE" sh -ec '
for cxx in powerpc-apple-macos-g++ m68k-apple-macos-g++; do
	echo "== $cxx"
	'"$TC"'/bin/$cxx --version | head -1
	for f in tools/retro68-probe/probe.cpp src/core/geometry.cpp src/core/projection.cpp src/core/trackball.cpp src/core/app.cpp src/core/controls.cpp src/core/countries.cpp src/core/places.cpp src/core/trace.cpp src/data/grid_codec.cpp src/data/landclasses.cpp src/data/elevation.cpp src/data/countries.cpp src/data/places.cpp src/data/atlas.cpp src/data/crc32.cpp src/render/framebuffer.cpp src/render/raster.cpp src/render/render.cpp src/render/terrain.cpp src/render/places.cpp src/render/highlight.cpp src/render/daynight.cpp src/render/measure.cpp src/render/overlays.cpp src/render/label_layout.cpp src/render/zoom.cpp src/render/shown_frame.cpp src/render/refine_runner.cpp src/platform/common/pixel_format.cpp src/platform/common/elevation_packs.cpp; do
		'"$TC"'/bin/$cxx '"$STD"' -fno-exceptions -fno-rtti -Wall -Wextra -Wpedantic -Iinclude -c "$f" -o /tmp/probe.o 2>&1 \
			| grep -E "probe:|error|warning" || true
		if '"$TC"'/bin/$cxx '"$STD"' -fno-exceptions -fno-rtti -Iinclude -c "$f" -o /tmp/probe.o 2>/dev/null; then
			echo "ok   $f"
		else
			echo "FAIL $f"
		fi
	done
done
'
