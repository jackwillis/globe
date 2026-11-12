#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
set -eu
cd "$(dirname "$0")/.."
CONTAINER=${CONTAINER:-podman}
IMAGE=${RETRO68_IMAGE:-ghcr.io/autc04/retro68}
case ${1:-} in
''|--floppy) ;;
*) echo 'usage: tools/build-os9.sh [--floppy]' >&2; exit 2 ;;
esac
"$CONTAINER" run --rm -e GLOBE_TIMING_TITLE -v "$PWD":/src:Z -w /src "$IMAGE" sh -ec '
  cmake -S os9 -B build-os9-floppy -DGLOBE_TIMING_TITLE=${GLOBE_TIMING_TITLE:-OFF} -DCMAKE_TOOLCHAIN_FILE=/Retro68-build/toolchain/powerpc-apple-macos/cmake/retroppc.toolchain.cmake
  cmake --build build-os9-floppy -j2
  tools/package-floppy.sh build-os9-floppy'
# Record which source and toolchain made this build, so packaging can refuse a stale or dirty one.
{
  echo "revision=$(git rev-parse HEAD)"
  if [ -z "$(git status --porcelain --untracked-files=no)" ]; then echo "tree=clean"; else echo "tree=dirty"; fi
  digest=$("$CONTAINER" image inspect --format '{{index .RepoDigests 0}}' "$IMAGE" 2>/dev/null || true)
  echo "retro68_image=${digest:-$IMAGE}"
} > build-os9-floppy/build-info.txt
