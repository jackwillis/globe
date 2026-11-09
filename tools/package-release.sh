#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Digital-release artifacts from one build (#47): StuffIt, MacBinary, committed source and checksums
# in <build>/release/. Needs the Classic build's GlobeFloppy.bin, podman,
# Rust build image, Python 3, git and unar (for round-trip verification). Publishes nothing.
set -eu
cd "$(dirname "$0")/.."
build_dir=${1:-build-os9-floppy}
CONTAINER=${CONTAINER:-podman}
RUST_IMAGE=${RUST_IMAGE:-docker.io/library/rust:slim}
[ -f "$build_dir/GlobeFloppy.bin" ] || { echo "no $build_dir/GlobeFloppy.bin: build first" >&2; exit 2; }
out="$build_dir/release"
mkdir -p "$out" "$build_dir/sit-target"
rev=$(git rev-parse --short HEAD)
revision=$(git rev-parse HEAD)
# Source is always from the exact revision, not any uncommitted local workspace edits.
if ! git diff --quiet || ! git diff --cached --quiet; then
  if [ "${GLOBE_RELEASE_STRICT:-0}" = 1 ]; then
    echo 'release: tracked tree is dirty; refuse strict packaging' >&2; exit 2
  fi
  echo 'warning: uncommitted changes exist; release source archive is from committed HEAD only' >&2
fi
# One date for every entry so the same inputs give the same archive.
epoch=${SOURCE_DATE_EPOCH:-$(git log -1 --format=%ct)}
"$CONTAINER" run --rm -v "$PWD/tools/sit-pack":/w:Z -v "$PWD/$build_dir/sit-target":/t:Z \
    -w /w -e CARGO_TARGET_DIR=/t "$RUST_IMAGE" cargo build --release --locked >/dev/null
sit="$out/Globe-$rev.sit"
"$build_dir/sit-target/release/sit-pack" -m "${SIT_METHOD:-13}" --date "$epoch" -o "$sit" \
    --mac GlobeFloppy="$build_dir/GlobeFloppy.bin" \
    --text ReadMe=os9/ReadMe.txt --text License=LICENSE \
    --text DataCredit=data/LANDCLASS-NOTICE.txt
cp "$build_dir/GlobeFloppy.bin" "$out/Globe-$rev.bin"
# Round trip with an independent reader: both forks must match the MacBinary.
check=$(mktemp -d)
trap 'rm -rf "$check"' EXIT HUP INT TERM
unar -q -o "$check/sit" -forks visible "$sit" >/dev/null
unar -q -o "$check/bin" -forks visible "$out/Globe-$rev.bin" >/dev/null
cmp "$check/sit/Globe-$rev/GlobeFloppy" "$check/bin/GlobeFloppy"
cmp "$check/sit/Globe-$rev/GlobeFloppy.rsrc" "$check/bin/GlobeFloppy.rsrc"
# GNU tar and gzip timestamps are determined by the commit, not build wall time.
# Include the full tracked corresponding source, not a link to an online repository.
source_archive="$out/Globe-$rev-source.tar.gz"
git archive --format=tar HEAD | gzip -n > "$source_archive"
python3 tools/release_manifest.py --output "$out/RELEASE.json" \
    --revision "$revision" --epoch "$epoch" \
    --retro68-image "${RETRO68_IMAGE:-ghcr.io/autc04/retro68}" \
    --rust-image "$RUST_IMAGE" \
    "$sit" "$out/Globe-$rev.bin" "$source_archive"
(cd "$out" && sha256sum "Globe-$rev.sit" "Globe-$rev.bin" "Globe-$rev-source.tar.gz" RELEASE.json > SHA256SUMS)
echo "round trip ok (data and resource forks); binaries, corresponding source, manifest and checksums in $out"
