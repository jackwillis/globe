# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Check actual one-shot wakes and quiet idle under the caller's private Xvfb."""
import json
import subprocess
import sys

result = subprocess.run(sys.argv[1:], capture_output=True, text=True, check=True)
print(result.stdout, end="")
stats = json.loads(result.stdout)
assert stats["quality_stage"] == 2, stats  # Refine
assert stats["wake_ticks"] == 2, stats  # Settle and Refine; the lines are a layer now, no reveal wakes
assert stats["refine_bands"] > 0, stats
assert stats["verified_frames"] == stats["presentations"], stats
assert stats["idle_frames"] == 0, stats
assert stats["idle_cpu_ms"] < 50, stats  # two-second wait, generous scheduling tolerance
