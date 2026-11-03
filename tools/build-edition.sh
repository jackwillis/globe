#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Two release payloads; testing/sanitizers remain development options, not extra editions.
set -eu
cd "$(dirname "$0")/.."
case ${1:-} in
full)
    cmake -S . -B build-full -DCMAKE_BUILD_TYPE=Release -DGLOBE_BUILD_TESTS=OFF
    cmake --build build-full -j2 --target globe-x11
    python3 tools/floppy_report.py --edition full --app build-full/globe-x11 \
      --document LICENSE --document data/LANDCLASS-NOTICE.txt --output build-full/size-report.json
    ;;
floppy) tools/build-os9.sh --floppy ;;
*) echo 'usage: tools/build-edition.sh full|floppy' >&2; exit 2 ;;
esac
