#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Split the GPL text into two verbatim halves that SimpleText (32 KB limit) can open.

    split_license.py LICENSE OUTDIR   ->  OUTDIR/License-1.txt, OUTDIR/License-2.txt

The split is at the blank line nearest the middle; joining the parts reproduces LICENSE exactly.
"""
import sys
from pathlib import Path

LIMIT = 32 * 1024 - 1


def split(text: bytes):
    middle = len(text) // 2
    best = None
    pos = text.find(b"\n\n")
    while pos != -1:
        cut = pos + 2
        if best is None or abs(cut - middle) < abs(best - middle):
            best = cut
        pos = text.find(b"\n\n", pos + 1)
    if best is None:
        raise ValueError("no blank line to split at")
    first, second = text[:best], text[best:]
    if len(first) > LIMIT or len(second) > LIMIT:
        raise ValueError("a part exceeds SimpleText's 32 KB limit")
    assert first + second == text
    return first, second


def main() -> None:
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    first, second = split(Path(sys.argv[1]).read_bytes())
    out = Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    (out / "License-1.txt").write_bytes(first)
    (out / "License-2.txt").write_bytes(second)


if __name__ == "__main__":
    main()
