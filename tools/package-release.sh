#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Digital-release artifacts from one build (#47): StuffIt, MacBinary, HFS disk image, committed
# source, manifest and checksums in <build>/release/. Needs the Classic build from
# tools/build-os9.sh (PoodleEarth-Finder.bin, PoodleEarth-1440.dsk, build-info.txt), podman, the Rust
# build image, Python 3, git, unar and hfsutils (round-trip verification). Publishes nothing.
#   GLOBE_RELEASE_STRICT=1  refuse a dirty tree or a build made from any other revision.
set -eu
cd "$(dirname "$0")/.."
build_dir=${1:-build-os9-floppy}
CONTAINER=${CONTAINER:-podman}
RUST_IMAGE=${RUST_IMAGE:-docker.io/library/rust:slim}
for f in PoodleEarth-Finder.bin PoodleEarth-1440.dsk build-info.txt; do
  [ -f "$build_dir/$f" ] || { echo "no $build_dir/$f: build first (tools/build-os9.sh)" >&2; exit 2; }
done
rev=$(git rev-parse --short HEAD)
revision=$(git rev-parse HEAD)
version=$(sed -n 's/^project(globe VERSION \([0-9][0-9.]*\).*/\1/p' CMakeLists.txt)
[ -n "$version" ] || { echo 'release: no version in CMakeLists.txt' >&2; exit 2; }
name="PoodleEarth-$version-$rev"
# The application must come from this exact, clean revision; the source archive is that revision.
built=$(sed -n 's/^revision=//p' "$build_dir/build-info.txt")
tree=$(sed -n 's/^tree=//p' "$build_dir/build-info.txt")
retro68_image=$(sed -n 's/^retro68_image=//p' "$build_dir/build-info.txt")
dirty=0
if ! git diff --quiet || ! git diff --cached --quiet; then dirty=1; fi
if [ "$built" != "$revision" ] || [ "$tree" != clean ] || [ "$dirty" = 1 ]; then
  if [ "${GLOBE_RELEASE_STRICT:-0}" = 1 ]; then
    echo "release: strict mode needs a clean tree and a build of $revision (build: $built, $tree; tree dirty: $dirty)" >&2
    exit 2
  fi
  echo "warning: build revision/tree does not match a clean HEAD; the source archive is committed HEAD only" >&2
fi
out="$build_dir/release"
rm -rf "$out"
mkdir -p "$out" "$build_dir/sit-target"
# One date for every entry so the same inputs give the same archive.
epoch=${SOURCE_DATE_EPOCH:-$(git log -1 --format=%ct)}
"$CONTAINER" run --rm -v "$PWD/tools/sit-pack":/w:Z -v "$PWD/$build_dir/sit-target":/t:Z \
    -w /w -e CARGO_TARGET_DIR=/t "$RUST_IMAGE" cargo build --release --locked >/dev/null
# Record the image actually used by its digest, when the local engine knows one.
rust_ref=$("$CONTAINER" image inspect --format '{{index .RepoDigests 0}}' "$RUST_IMAGE" 2>/dev/null || true)
rust_ref=${rust_ref:-$RUST_IMAGE}
sit="$out/$name.sit"
"$build_dir/sit-target/release/sit-pack" -m "${SIT_METHOD:-13}" --date "$epoch" -o "$sit" \
    --mac "Poodle Earth=$build_dir/PoodleEarth-Finder.bin" \
    --text ReadMe=os9/ReadMe.txt \
    --text "License (1 of 2)=$build_dir/license/License-1.txt" \
    --text "License (2 of 2)=$build_dir/license/License-2.txt" \
    --text DataCredit=data/DATA-CREDITS.txt
cp "$build_dir/PoodleEarth-Finder.bin" "$out/$name.bin"
cp "$build_dir/PoodleEarth-1440.dsk" "$out/$name.dsk"
# Round trip with independent readers: every route must give the same application forks.
check=$(mktemp -d)
trap '"humount" >/dev/null 2>&1 || true; rm -rf "$check"' EXIT HUP INT TERM
unar -q -o "$check/sit" -forks visible "$sit" >/dev/null
unar -q -o "$check/bin" -forks visible "$out/$name.bin" >/dev/null
hmount "$out/$name.dsk" >/dev/null
hcopy -m ":Poodle Earth" "$check/from-dsk.bin"
for doc in "ReadMe" "License (1 of 2)" "License (2 of 2)" DataCredit; do hcopy -t ":$doc" "$check/dsk-$doc"; done
hls -l ":ReadMe" ":License (1 of 2)" ":License (2 of 2)" ":DataCredit" | awk '$2 != "TEXT/ttxt" { bad = 1 } END { exit bad }' || { echo "release: HFS documents are not TEXT/ttxt" >&2; exit 1; }
humount >/dev/null
unar -q -o "$check/dsk" -forks visible "$check/from-dsk.bin" >/dev/null
for route in "$check/sit/$name" "$check/dsk"; do
  cmp "$route/Poodle Earth" "$check/bin/Poodle Earth"
  cmp "$route/Poodle Earth.rsrc" "$check/bin/Poodle Earth.rsrc"
done
# The texts the user reads are the repository's (LF there, CR on the Mac) in every route.
for pair in "ReadMe:os9/ReadMe.txt" "License (1 of 2):$build_dir/license/License-1.txt" \
            "License (2 of 2):$build_dir/license/License-2.txt" "DataCredit:data/DATA-CREDITS.txt"; do
  doc=${pair%%:*}; src=${pair#*:}
  tr '\r' '\n' < "$check/dsk-$doc" | cmp - "$src"
  tr '\r' '\n' < "$check/sit/$name/$doc" | cmp - "$src"
done
# ...and the two halves are exactly the GPL text, each small enough for SimpleText.
cat "$build_dir/license/License-1.txt" "$build_dir/license/License-2.txt" | cmp - LICENSE
for part in "$build_dir"/license/License-*.txt; do [ "$(wc -c < "$part")" -lt 32768 ]; done
# GNU tar and gzip timestamps are determined by the commit, not build wall time.
# The complete tracked corresponding source, so the release does not rest on an online repository.
source_archive="$out/$name-source.tar.gz"
git archive --format=tar --prefix="globe-$version-$rev/" HEAD | gzip -n > "$source_archive"
python3 tools/release_manifest.py --output "$out/RELEASE.json" \
    --revision "$revision" --epoch "$epoch" \
    --retro68-image "${RETRO68_IMAGE:-${retro68_image:-ghcr.io/autc04/retro68}}" \
    --rust-image "$rust_ref" \
    "$sit" "$out/$name.bin" "$out/$name.dsk" "$source_archive"
(cd "$out" && sha256sum "$name.sit" "$name.bin" "$name.dsk" "$name-source.tar.gz" RELEASE.json > SHA256SUMS)
echo "round trip ok (data and resource forks via .sit, .bin and .dsk); artifacts, source, manifest and checksums in $out"
