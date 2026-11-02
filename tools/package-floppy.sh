#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Run inside the Retro68 container after the designated app build.
set -eu
cd "$(dirname "$0")/.."
build_dir=${1:-build-os9-floppy}
TC=/Retro68-build/toolchain/bin
image="$build_dir/GlobeFloppy-1440.dsk"
python3 -c 'import sys; f=open(sys.argv[1],"wb"); f.truncate(1474560); f.close()' "$image"
"$TC/hformat" -l Globe "$image"
"$TC/hmount" "$image"
trap '"$TC/humount" >/dev/null 2>&1 || true' EXIT HUP INT TERM
"$TC/hcopy" -m "$build_dir/GlobeFloppy.bin" :GlobeFloppy
"$TC/hcopy" -t os9/ReadMe.txt :ReadMe
"$TC/hcopy" -t LICENSE :License
"$TC/hcopy" -m :GlobeFloppy "$build_dir/roundtrip.bin"
"$TC/hls" -la : > "$build_dir/hfs-files.txt"
"$TC/hvol" > "$build_dir/hfs-volume.txt"
"$TC/humount"
trap - EXIT HUP INT TERM
python3 tools/floppy_report.py --app "$build_dir/GlobeFloppy.bin" \
    --roundtrip "$build_dir/roundtrip.bin" --disk "$image" \
    --document os9/ReadMe.txt --document LICENSE --volume "$build_dir/hfs-volume.txt" --output "$build_dir/size-report.json"
