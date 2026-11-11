# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Check the benchmark's explicitly partial payload ledger against resource sizes."""
import json
from pathlib import Path
import subprocess
import sys

bench, root = sys.argv[1:]
trace = str(Path(root) / "tests/data/trace1.txt")
for width, height, mode in [(80, 60, []), (81, 61, ["--reference"]), (80, 60, ["--refine"])]:
    run = subprocess.run(
        [bench, "--size", f"{width}x{height}", "--repeat", "1", *mode, trace],
        check=True, capture_output=True, text=True,
    )
    report = json.loads(run.stdout)
    memory = report["memory_payload"]
    assert memory["scope"] == "scene_and_primary_framebuffer"
    assert memory["units"] == "bytes" and not memory["accounting_error"]
    assert "terrain" in memory["excludes"] and "RSS" in memory["excludes"]
    expected = {
        "scene": report["scene_bytes"],
        "framebuffer": width * height * 2 + ((width + 7) // 8) * height,
    }
    for category, size in expected.items():
        counter = memory[category]
        assert counter["current"] == counter["peak"] == size
        assert counter["successful_allocations"] == 2
        assert counter["failed_allocations"] == 0
    assert memory["total_current"] == memory["total_peak"] == sum(expected.values())
print("bench memory: scoped counters agree with resource payloads in scene/reference/refine modes")
