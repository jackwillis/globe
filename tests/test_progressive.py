# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Real half-degree data must finish byte-identically to finest-only rendering."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
bench, pack = sys.argv[1:]
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    for zoom in (3, 20):
        for relief in (False, True):
            trace = root / 'view.txt'
            trace.write_text(f'wheel 0 {zoom}\ntick 150\ntick 151\n' +
                             ('style 200\n' if relief else ''))
            results = []
            for flag, name in (('--refine', 'progressive'), ('--refine-finest', 'fresh')):
                ppm = root / (name + '.ppm')
                result = subprocess.run([bench, '--elevation', pack, flag, '--repeat', '1',
                                         '--dump-ppm', str(ppm), str(trace)],
                                        capture_output=True, text=True, check=True)
                stats = json.loads(result.stdout)
                assert stats['selected_elevation_level'] == 1, stats
                assert stats['refine_levels'] == (2 if name == 'progressive' else 1), stats
                assert stats['first_refine_band_ms'] > 0, stats
                results.append((ppm.read_bytes(), stats['frame_checksum']))
            assert results[0] == results[1], (zoom, relief)
print('real half-degree progressive/fresh pixels agree at two zooms in both styles')
