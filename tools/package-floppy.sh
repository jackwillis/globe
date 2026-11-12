#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Run inside the Retro68 container after the designated app build. Gives the application its Finder
# name "Poodle Earth" (PoodleEarth-Finder.bin) and puts it with the notices on a 1.44 MB HFS image.
set -eu
cd "$(dirname "$0")/.."
build_dir=${1:-build-os9-floppy}
TC=/Retro68-build/toolchain/bin
app="$build_dir/PoodleEarth-Finder.bin"
python3 tools/macbinary_rename.py "$build_dir/PoodleEarth.bin" "$app" "Poodle Earth"
image="$build_dir/PoodleEarth-1440.dsk"
python3 -c 'import sys; f=open(sys.argv[1],"wb"); f.truncate(1474560); f.close()' "$image"
"$TC/hformat" -l "Poodle Earth" "$image"
"$TC/hmount" "$image"
trap '"$TC/humount" >/dev/null 2>&1 || true' EXIT HUP INT TERM
"$TC/hcopy" -m "$app" ":Poodle Earth"
"$TC/hcopy" -t os9/ReadMe.txt :ReadMe
"$TC/hcopy" -t LICENSE :License
"$TC/hcopy" -t data/DATA-CREDITS.txt :DataCredit
"$TC/hcopy" -m ":Poodle Earth" "$build_dir/roundtrip.bin"
"$TC/hls" -la : > "$build_dir/hfs-files.txt"
"$TC/hvol" > "$build_dir/hfs-volume.txt"
"$TC/humount"
trap - EXIT HUP INT TERM
python3 tools/floppy_report.py --app "$app" \
    --roundtrip "$build_dir/roundtrip.bin" --disk "$image" \
    --document os9/ReadMe.txt --document LICENSE --document data/DATA-CREDITS.txt --volume "$build_dir/hfs-volume.txt" --output "$build_dir/size-report.json"
