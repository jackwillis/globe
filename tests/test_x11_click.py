# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""A click without dragging and window exposes must not render again (ADR behavior rules)."""
import json
import subprocess
import sys

result = subprocess.run(sys.argv[1:], capture_output=True, text=True, check=True)
print(result.stdout, end="")
stats = json.loads(result.stdout)
# One render for the first exposure. Exposes resend pixels; a click, and a wobble inside the drag
# slop, change nothing.
assert stats["frames"] == 1, stats
assert stats["presentations"] >= 4, stats  # first frame plus three resends
assert stats["verified_frames"] == stats["presentations"], stats
assert stats["quality_stage"] == 2, stats  # still settled (Refine)
