#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
# Repository rules a machine can check. POSIX sh; run from the repo root (CI does).
#   1. No Apple copyright headers or classic-Mac archive/disk-image files in the tree.
#   2. Every first-party source, script and build file carries the licence tag.
fail=0
bad() { echo "policy: $1"; fail=1; }

if git ls-files 'include/*' 'src/*' 'tests/*' 'tools/*' | \
    xargs grep -lIE 'Copyright.*Apple (Computer|Inc)|Apple Computer, Inc\.' 2>/dev/null | \
    grep -v '^tools/check-policy.sh$' | grep .; then
	bad "a file carries an Apple copyright notice; Apple material must not be copied into the tree"
fi
if git ls-files | grep -iE '\.(sit|hqx|smi|img|dsk|rsrc|bin)$' | grep .; then
	bad "classic-Mac archive or disk-image file tracked; build artifacts are not committed"
fi

missing=$(git ls-files 'include/*' 'src/*' 'tests/*' 'tools/*' 'CMakeLists.txt' | \
    grep -E '\.(cpp|hpp|cc|h|sh|py|txt)$' | \
    xargs grep -L 'SPDX-License-Identifier: GPL-3.0-or-later')
[ -z "$missing" ] || { echo "$missing"; bad "missing 'SPDX-License-Identifier: GPL-3.0-or-later'"; }

[ "$fail" = 0 ] && echo "policy: ok"
exit $fail
