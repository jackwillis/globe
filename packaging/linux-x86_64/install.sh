#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Installs or removes Poodle Earth from this archive. POSIX sh; needs no root unless the prefix does.
#   ./install.sh [--prefix DIR] [install]    default prefix /usr/local; honours DESTDIR
#   ./install.sh [--prefix DIR] uninstall    removes exactly the files MANIFEST lists
set -eu
here=$(cd "$(dirname "$0")" && pwd)
prefix=/usr/local
action=install
while [ $# -gt 0 ]; do
  case $1 in
    --prefix) [ $# -ge 2 ] || { echo "install.sh: --prefix needs a directory" >&2; exit 2; }
              prefix=$2; shift 2 ;;
    install|uninstall) action=$1; shift ;;
    -h|--help) sed -n '5,7p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "install.sh: unknown argument: $1" >&2; exit 2 ;;
  esac
done
case $prefix in /*) ;; *) echo "install.sh: --prefix must be an absolute path" >&2; exit 2 ;; esac
root=${DESTDIR:-}$prefix
[ -f "$here/MANIFEST" ] || { echo "install.sh: no MANIFEST beside this script" >&2; exit 2; }
# MANIFEST lines are "<mode> <path relative to the prefix>".
case $action in
install)
  while read -r mode path; do
    [ -f "$here/$path" ] || { echo "install.sh: archive is missing $path" >&2; exit 1; }
  done < "$here/MANIFEST"
  while read -r mode path; do
    mkdir -p "$root/$(dirname "$path")"
    cp "$here/$path" "$root/$path"
    chmod "$mode" "$root/$path"
  done < "$here/MANIFEST"
  echo "installed under $root; run $root/bin/poodle-earth (needs an X display and libX11)" ;;
uninstall)
  while read -r mode path; do rm -f "$root/$path"; done < "$here/MANIFEST"
  rmdir "$root/share/doc/poodle-earth" 2>/dev/null || true
  echo "removed Poodle Earth files from $root (empty parent directories were left alone)" ;;
esac
