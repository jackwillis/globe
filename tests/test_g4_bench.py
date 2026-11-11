#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Offline runner contract: validate provenance, preserve samples, reject unstable output."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / 'tools/g4_bench.py'


class RunnerTest(unittest.TestCase):
    def test_offline_bundle(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            binary = root / 'globe-line-layer-bench'
            binary.write_text('#!' + sys.executable + '\n'
                              'print(\'{"results":[{"radius":220,"mode":"layer",'
                              '"wall_median_ms":12,"checksum":"abcd"}]}\')\n')
            binary.chmod(0o755)
            manifest = {'schema': 1, 'git_sha': 'a' * 40, 'files_sha256': {
                binary.name: hashlib.sha256(binary.read_bytes()).hexdigest()}}
            (root / 'manifest.json').write_text(json.dumps(manifest))
            output = root / 'run.json'
            command = [sys.executable, str(SCRIPT), 'run', '--bundle', str(root),
                       '--output', str(output), '--repeat', '2', '--warmup', '1',
                       '--workload', 'line-layer']
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            saved = json.loads(output.read_text())
            self.assertEqual(saved['build']['git_sha'], 'a' * 40)
            rows = saved['results']['line-layer']
            self.assertEqual(len(rows['samples']), 2)
            self.assertEqual(rows['summary']['case0.layer.r220.wall_median_ms']['median'], 12)
            self.assertGreater(rows['samples'][0]['peak_rss_kib'], 0)
            # An existing baseline is never overwritten.
            before = output.read_bytes()
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)
            self.assertEqual(output.read_bytes(), before)
            # Edited/transferred binaries must match the build manifest before execution.
            binary.write_text(binary.read_text() + '# modified\n')
            command[command.index('--output') + 1] = str(root / 'changed.json')
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('hash mismatch', result.stderr)
            self.assertFalse((root / 'changed.json').exists())
            # Reproducibility failure must not produce a seemingly valid result file.
            counter = root / 'counter'
            binary.write_text('#!' + sys.executable + '\n'
                              'from pathlib import Path\n'
                              f'p=Path({str(counter)!r})\n'
                              'n=int(p.read_text())+1 if p.exists() else 0\n'
                              'p.write_text(str(n))\n'
                              'import json\n'
                              'print(json.dumps({"results":[{"checksum":str(n)}]}))\n')
            manifest['files_sha256'][binary.name] = hashlib.sha256(binary.read_bytes()).hexdigest()
            (root / 'manifest.json').write_text(json.dumps(manifest))
            result = subprocess.run(command, capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('unstable', result.stderr)
            self.assertFalse((root / 'changed.json').exists())


if __name__ == '__main__':
    unittest.main()
