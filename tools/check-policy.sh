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

# 3. Layering for portability (Mac OS 9 is a target): the portable directories may not include
#    operating-system or window-system headers, read a clock, or use printf size modifiers that
#    Retro68's library lacks (%ll, %z). Platform code lives under src/platform/.
portable='include/*.hpp include/globe/*.hpp src/core/* src/data/* src/render/*'
# shellcheck disable=SC2086
if git ls-files $portable | xargs grep -nE '#include <(sys/|unistd|X11/|ctime|time\.h|pthread|fcntl|dirent|windows|errno|cerrno|cstdio|stdio\.h|cstdlib|stdlib\.h)|clock_gettime|getrusage|XOpenDisplay' 2>/dev/null | grep -v '^src/core/trace.cpp:.*#include <c\(stdio\|stdlib\)>' | grep .; then
	bad "portable code (include/, src/core, src/data, src/render) must not use OS, clock or stdio headers (trace.cpp's text formatting is the one reviewed exception)"
fi
# shellcheck disable=SC2086
if git ls-files $portable | xargs grep -nE '%[-0-9.]*(ll|z)[diouxX]' 2>/dev/null | grep .; then
	bad "portable code uses printf size modifiers (%ll, %z) that Retro68 does not support"
fi
if git ls-files 'src/platform/common/*' | xargs grep -nE '#include <(X11/|sys/)' 2>/dev/null | grep -v clock_posix | grep .; then
	bad "src/platform/common is shared by every platform and must not include X11 or sys/ headers (clock_posix.cpp excepted)"
fi

missing=$(git ls-files 'include/*' 'src/*' 'tests/*' 'tools/*' 'CMakeLists.txt' | \
    grep -E '\.(cpp|hpp|cc|h|sh|py)$|(^|/)CMakeLists\.txt$' | \
    xargs grep -L 'SPDX-License-Identifier: GPL-3.0-or-later')
[ -z "$missing" ] || { echo "$missing"; bad "missing 'SPDX-License-Identifier: GPL-3.0-or-later'"; }

[ "$fail" = 0 ] && echo "policy: ok"
exit $fail
