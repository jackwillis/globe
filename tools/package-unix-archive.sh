#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Linux x86-64 archive of the X11 Reader from the staged CMake install (#122), with the matching
# source archive and checksums, in <out>/. Builds from a `git archive` of committed HEAD, so the
# binary and the source archive are the same revision. Publishes nothing.
#   tools/package-unix-archive.sh [OUT_DIR]      default build-unix-archive
#   GLOBE_RELEASE_STRICT=1   refuse a dirty tree
#   GLOBE_SMOKE=0            skip the Xvfb launch of the unpacked archive
set -eu
cd "$(dirname "$0")/.."
out=${1:-build-unix-archive}
[ "$(uname -s)" = Linux ] && [ "$(uname -m)" = x86_64 ] || { echo "package-unix-archive: Linux x86_64 host required" >&2; exit 2; }
revision=$(git rev-parse HEAD)
rev=$(git rev-parse --short HEAD)
version=$(sed -n 's/^project(globe VERSION \([0-9][0-9.]*\).*/\1/p' CMakeLists.txt)
[ -n "$version" ] || { echo 'package-unix-archive: no version in CMakeLists.txt' >&2; exit 2; }
dirty=0
if ! git diff --quiet || ! git diff --cached --quiet; then dirty=1; fi
if [ "$dirty" = 1 ]; then
  [ "${GLOBE_RELEASE_STRICT:-0}" != 1 ] || { echo "package-unix-archive: strict mode needs a clean tree" >&2; exit 2; }
  echo "warning: working tree is dirty; the archive is built from committed HEAD only" >&2
fi
name="poodle-earth-$version-$rev-linux-x86_64"
epoch=${SOURCE_DATE_EPOCH:-$(git log -1 --format=%ct)}
rm -rf "$out"
mkdir -p "$out/work"
out=$(cd "$out" && pwd)
work=$out/work
trap 'rm -rf "$work"' EXIT HUP INT TERM

# The complete corresponding source, and the tree the binary is built from.
git archive --format=tar --prefix="poodle-earth-$version-$rev/" HEAD | gzip -n > "$out/poodle-earth-$version-$rev-source.tar.gz"
tar -xzf "$out/poodle-earth-$version-$rev-source.tar.gz" -C "$work"
src=$work/poodle-earth-$version-$rev
cmake -S "$src" -B "$work/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLOBE_BUILD_TESTS=OFF >/dev/null
cmake --build "$work/build" --target globe-x11 >/dev/null
pkg=$work/pkg/$name
mkdir -p "$pkg"
DESTDIR=$pkg cmake --install "$work/build" --prefix / >/dev/null
[ -x "$pkg/bin/poodle-earth" ] || { echo "package-unix-archive: staged install has no bin/poodle-earth" >&2; exit 1; }

# Record what the binary needs, from the binary itself.
needed=$(readelf -d "$pkg/bin/poodle-earth" | sed -n 's/.*Shared library: \[\(.*\)\]/\1/p' | tr '\n' ' ')
rpath=$(readelf -d "$pkg/bin/poodle-earth" | grep -cE 'RPATH|RUNPATH' || true)
glibc=$(objdump -T "$pkg/bin/poodle-earth" | grep -oE 'GLIBC_[0-9.]+' | sort -Vu | tail -1)
[ "$rpath" = 0 ] || { echo "package-unix-archive: binary has RPATH/RUNPATH" >&2; exit 1; }
os_name=$(sed -n 's/^PRETTY_NAME=//p' /etc/os-release | tr -d '"')
cat > "$pkg/BUILD-INFO.txt" <<INFO
product=Poodle Earth X11 Reader (embedded data)
version=$version
revision=$revision
tree=$([ "$dirty" = 1 ] && echo "dirty (archive built from committed HEAD)" || echo clean)
target=linux-x86_64
glibc_required=${glibc#GLIBC_}
needed_libraries=${needed% }
built_on=$os_name, $(uname -m)
compiler=$(c++ --version | head -1)
cmake=$(cmake --version | head -1)
INFO
cat > "$pkg/SOURCE.txt" <<SRC
Poodle Earth $version, revision $revision.
Source: https://github.com/jackwillis/poodle-earth (the tree of that commit), and
the file poodle-earth-$version-$rev-source.tar.gz published beside this archive,
which is that tree. The binary here was built from that tree with:
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DGLOBE_BUILD_TESTS=OFF
  cmake --build build --target globe-x11
  DESTDIR=stage cmake --install build --prefix /
This program is GPL-3.0-or-later; see share/doc/poodle-earth/LICENSE.
SRC
cp packaging/linux-x86_64/INSTALL.txt packaging/linux-x86_64/install.sh "$pkg/"
# MANIFEST: what install.sh copies, "<mode> <path>".
(cd "$pkg" && find bin share -type f | sort | while read -r f; do
   printf '%s %s\n' "$(stat -c %a "$f")" "$f"; done) > "$pkg/MANIFEST"

# Same inputs, same archive: fixed order, owner and times.
tar --sort=name --mtime="@$epoch" --owner=0 --group=0 --numeric-owner -C "$work/pkg" -cf - "$name" \
  | gzip -n > "$out/$name.tar.gz"
(cd "$out" && sha256sum "$name.tar.gz" "poodle-earth-$version-$rev-source.tar.gz" > SHA256SUMS)

# Round trip: unpack, install to a scratch prefix, compare, launch, remove.
chk=$work/check
mkdir -p "$chk/unpack" "$chk/prefix"
tar -xzf "$out/$name.tar.gz" -C "$chk/unpack"
"$chk/unpack/$name/install.sh" --prefix "$chk/prefix" >/dev/null
cmp "$chk/prefix/bin/poodle-earth" "$work/build/poodle-earth"
cmp "$chk/prefix/share/doc/poodle-earth/LICENSE" LICENSE
if [ "${GLOBE_SMOKE:-1}" = 1 ] && command -v xvfb-run >/dev/null 2>&1; then
  xvfb-run -a -s "-screen 0 800x600x24" "$chk/prefix/bin/poodle-earth" --size 160x120 \
    --script tests/data/trace1.txt --exit-when-done --stats | grep -q '"frames": 113'
  smoke=ok
else
  smoke=skipped
fi
"$chk/unpack/$name/install.sh" --prefix "$chk/prefix" uninstall >/dev/null
left=$(find "$chk/prefix" -type f | wc -l)
[ "$left" = 0 ] || { echo "package-unix-archive: uninstall left $left file(s)" >&2; exit 1; }
echo "ok: $out/$name.tar.gz (glibc >= ${glibc#GLIBC_}; Xvfb smoke $smoke); source archive and SHA256SUMS beside it"
