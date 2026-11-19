#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Apply consequential mutations to decision.py IN MEMORY and require the suite to fail.

No source file is edited. Each mutant is compiled from the original text with exact
anchor replacements and the whole test module is re-imported against it.
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
    ('ties round to the lower code', [(
        'y = maximum*value + F(1, 2)\n    return y.numerator // y.denominator',
        'y = maximum*value - F(1, 2)\n    return -((-y.numerator) // y.denominator)')]),
    ('quantizer cells closed at both ends', [(
        'hi, closed = (upper, True) if upper < right else (right, False)',
        'hi, closed = min(upper, right), True')]),
    ('Q chosen independently of R', [(
        'out.append((q, lo, hi, closed))', 'out.append((q, lower, upper, True))')]),
    ('bit replication omitted', [('return (code << 3) | (code >> 2)', 'return code << 3')]),
    ('target weight 1/4 instead of 1/5', [('TARGET = F(1, 5)', 'TARGET = F(1, 4)')]),
    ('local ratio 1/5 instead of 1/10', [('LOCAL_RATIO = F(1, 10)', 'LOCAL_RATIO = F(1, 5)')]),
    ('J keeps only the floor branch', [
        ('elif e_a.lower >= kink:', 'elif False:'),
        ('min(f1.lower, f2.lower), min(f1.upper, f2.upper)', 'f2.lower, f2.upper')]),
    ('J keeps only the ratio branch', [
        ('if e_a.upper <= kink:', 'if False:'),
        ('min(f1.lower, f2.lower), min(f1.upper, f2.upper)', 'f1.lower, f1.upper')]),
    ('J upper uses the optimistic branch', [(
        'min(f1.lower, f2.lower), min(f1.upper, f2.upper)', 'min(f1.lower, f2.lower), max(f1.upper, f2.upper)')]),
    ('J equality fails', [('(PASS, \'J.upper<=0\') if upper <= 0', '(PASS, \'J.upper<=0\') if upper < 0')]),
    ('J violation at equality', [('(FAIL, \'J.lower>0\') if lower > 0', '(FAIL, \'J.lower>0\') if lower >= 0')]),
    ('floor equality eligible', [("'ELIGIBLE' if d.lower > FLOOR", "'ELIGIBLE' if d.lower >= FLOOR")]),
    ('D.upper == floor not limited', [('if d.upper <= FLOOR:', 'if d.upper < FLOOR:')]),
    ('H equality fails', [('elif h.lower >= 0:', 'elif h.lower > 0:')]),
    ('H.upper == 0 fails', [('elif h.upper < 0:', 'elif h.upper <= 0:')]),
    ('eligibility ignored for verdicts', [(
        "    elif state == 'CROSSING':", "    elif False:")]),
    ('negative scale swaps wrong flags', [(
        'return Range(self.upper*k, self.upper_attained, self.lower*k, self.lower_attained)',
        'return Range(self.upper*k, self.lower_attained, self.lower*k, self.upper_attained)')]),
    ('geometry ids not required distinct', [(
        "sorted(c.geometry_id for c in conditions) == list(range(GEOMETRIES))", 'True')]),
    ('styles may be mixed', [("len({c.style for c in conditions}) == 1", 'True')]),
    ('FAIL ignored in conjunction', [('if FAIL in statuses:', 'if False:')]),
    ('partial conjunction may pass', [('return r if complete or r == FAIL else INCONCLUSIVE', 'return r')]),
    ('duplicate pixel guard omitted', [('and not seen[index],', ',')]),
]


def run(mutant_source, label):
    mutant = types.ModuleType('decision')
    mutant.__file__ = str(ROOT/'decision.py')
    sys.modules['decision'] = mutant
    exec(compile(mutant_source, mutant.__file__, 'exec'), mutant.__dict__)
    spec = importlib.util.spec_from_file_location('mutant_tests_'+label, ROOT/'test_decision.py')
    tests = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tests)
    suite = unittest.defaultTestLoader.loadTestsFromModule(tests)
    return unittest.TextTestRunner(stream=io.StringIO()).run(suite)


def main():
    original = (ROOT/'decision.py').read_text()
    baseline = run(original, 'baseline')
    if not baseline.wasSuccessful():
        raise SystemExit('baseline suite must pass before mutants are meaningful')
    results = []
    for index, (name, edits) in enumerate(CASES):
        source = original
        for old, new in edits:
            if source.count(old) != 1:
                raise RuntimeError('mutation anchor missing or ambiguous: '+name)
            source = source.replace(old, new)
        result = run(source, str(index))
        failing = sorted({t.id().split('.')[-1] for t, _ in result.failures + result.errors})
        results.append({'mutant': name, 'killed': bool(failing), 'failing_tests': failing})
        if not failing:
            raise AssertionError('survived: '+name)
    sys.modules.pop('decision', None)
    print(json.dumps({'baseline_tests': baseline.testsRun, 'mutants_killed': len(results),
                      'results': results}, indent=1))


if __name__ == '__main__':
    main()
