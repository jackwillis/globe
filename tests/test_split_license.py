#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("sl", ROOT / "tools/split_license.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
text = (ROOT / "LICENSE").read_bytes()
first, second = m.split(text)
assert first + second == text and 0 < len(first) <= m.LIMIT and 0 < len(second) <= m.LIMIT
assert first.endswith(b"\n\n")
try:
    m.split(b"x" * 70000)
except ValueError:
    pass
else:
    raise AssertionError("unsplittable text accepted")
print("ok")
