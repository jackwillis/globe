# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Default full-edition grids run two jobs, present correctly, then become quiet."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
with tempfile.TemporaryDirectory() as directory:
    trace = Path(directory) / 'view.txt'
    trace.write_text('wheel 0 20\ntick 150\ntick 151\n')
    result = subprocess.run([sys.argv[1], '--size', '320x240', '--script', str(trace),
                             '--idle-seconds', '2', '--verify-pixels', '--stats'],
                            capture_output=True, text=True, check=True)
    stats = json.loads(result.stdout)
    assert stats['elevation_pack_count'] == 2, stats
    assert stats['refinement_jobs'] == 2, stats
    assert stats['refine_bands'] > 16, stats
    assert stats['idle_frames'] == 0, stats
    assert stats['verified_frames'] == stats['presentations'], stats
    assert stats['idle_cpu_ms'] < 50, stats
    print(result.stdout, end='')
