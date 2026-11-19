#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Reproduce the six formerly surviving review mutants, in memory only.

No source/evidence is edited. Each selected control must pass on the original
module (the normal suite) and fail/error on the exact consequential mutation.
"""
import importlib.util
import io
import json
import sys
import types
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CASES = [
    ('second-pass hash omitted', 'adapter',
     "raw = self.checked(root / 'tiles' / f'{i:04d}.json.gz', row['sha256'], MAX_JSON)",
     "raw = bounded_read(root / 'tiles' / f'{i:04d}.json.gz', MAX_JSON)", True,
     'test_admission_integration', 'IntegrationTests.test_second_pass_mutation_and_partial_stream_have_no_receipt'),
    ('prefix split 4137', 'adapter', 'i < 4138', 'i < 4137', False,
     'test_admission_integration', 'IntegrationTests.test_admit_and_full_stream_completion_boundary'),
    ('any positive denominator', 'adapter', "p['denominator'] == DEN", "p['denominator'] > 0", False,
     'test_interval', 'AdmissionTests.test_noncanonical_positive_denominators'),
    ('floor instead of half-up', 'adapter', '(2*m*a+DEN)//(2*DEN), (2*m*b+DEN)//(2*DEN)',
     'm*a//DEN, m*b//DEN', False,
     'test_interval', 'AdmissionTests.test_interior_quantizer_boundaries_in_admission'),
    ('precision loosened twice', 'adapter', '<= 2*DEN', '<= 4*DEN', False,
     'test_interval', 'AdmissionTests.test_precision_equality_and_one_integer_over'),
    ('duplicate pixel guard omitted', 'metrics', 'or seen[index]', 'or False', False,
     'test_interval', 'MetricsTests.test_equal_count_duplicate_and_omitted_pixels'),
]


def main():
    results = []
    # Restore these imports per mutant; test globals must refer to that mutant.
    original_modules = {key: sys.modules.get(key) for key in ('adapter', 'metrics', 'test_interval')}
    for name, target, old, new, last_only, test_module, test_name in CASES:
        source = (ROOT/(target+'.py')).read_text()
        if old not in source:
            raise RuntimeError('mutation anchor missing: '+name)
        if last_only:
            before, after = source.rsplit(old, 1)
            source = before+new+after
        else:
            source = source.replace(old, new)
        mutant = types.ModuleType(target)
        mutant.__file__ = str(ROOT/(target+'.py'))
        sys.modules[target] = mutant
        exec(compile(source, mutant.__file__, 'exec'), mutant.__dict__)
        spec = importlib.util.spec_from_file_location('review_mutant_test', ROOT/(test_module+'.py'))
        tests = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(tests)
        suite = unittest.defaultTestLoader.loadTestsFromName(test_name, tests)
        result = unittest.TextTestRunner(stream=io.StringIO()).run(suite)
        killed = result.testsRun == 1 and not result.wasSuccessful()
        results.append({'mutant': name, 'control': test_name, 'killed': killed,
                        'failures': len(result.failures), 'errors': len(result.errors)})
        for key, original in original_modules.items():
            if original is None:
                sys.modules.pop(key, None)
            else:
                sys.modules[key] = original
        if not killed:
            raise AssertionError('survived: '+name)
    print(json.dumps({'mutants_killed': len(results), 'results': results}, indent=2))


if __name__ == '__main__':
    main()
