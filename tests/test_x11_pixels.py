# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Require successful readback of every presented frame at the requested Xvfb depth."""
import json
import subprocess
import sys

result = subprocess.run(sys.argv[2:], capture_output=True, text=True)
print(result.stdout, end="")
print(result.stderr, end="", file=sys.stderr)
result.check_returncode()
stats = json.loads(result.stdout)
assert stats["visual_depth"] == int(sys.argv[1]), stats
assert stats["frames"] > 100, stats
assert stats["verified_frames"] == stats["presentations"], stats
