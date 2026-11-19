# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Controls for the PE-AA-01 performance harness. Synthetic only, standard library only."""
import argparse
import ast
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import time
import unittest

sys.dont_write_bytecode = True
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import analyze  # noqa: E402
import perf  # noqa: E402
import run  # noqa: E402

decision = perf.decision
PINS = run.load_expected(run.EXPECTED)


class Workloads(unittest.TestCase):
    def test_prng_vectors(self):
        r = perf.SplitMix64(0)
        self.assertEqual([r.next() for _ in range(3)],
                         [0xE220A8397B1DCDAF, 0x6E789E6AA1B965F4, 0x06C45D188009454F])

    def test_below_is_in_range_and_unbiased_enough(self):
        r = perf.SplitMix64(7)
        seen = [r.below(6) for _ in range(6000)]
        self.assertEqual(set(seen), set(range(6)))
        self.assertTrue(all(800 < seen.count(v) < 1200 for v in range(6)))

    def test_determinism_and_seed_sensitivity(self):
        d = []
        for seed in (1, 1, 2):
            rows, mask, a, b, _ = perf.build_workload('mixed', 60, 300, seed)
            d.append(perf.input_digest('mixed', 60, 300, seed, rows, mask, a, b))
        self.assertEqual(d[0], d[1])
        self.assertNotEqual(d[0], d[2])

    def test_frame_and_mask_shape(self):
        self.assertEqual(perf.auto_frame(100), 411)
        self.assertEqual(perf.auto_frame(perf.FULL_MASK), perf.FULL_FRAME)
        rows, mask, a, b, st = perf.build_workload('narrow', 50, 200, 3)
        self.assertEqual((len(rows), len(mask), len(a), len(b), sum(mask)), (200, 200, 600, 600, 50))
        self.assertEqual([r[0] for r in rows], list(range(200)))
        for i, lo, hi, den in rows:
            self.assertEqual(den, perf.DEN)
            if not mask[i]:
                self.assertEqual((lo, hi), ((0, 0, 0), (0, 0, 0)))
        with self.assertRaises(ValueError):
            perf.build_workload('narrow', 190, 200, 3)

    def test_class_properties(self):
        for cls in perf.CLASSES:
            rows, mask, a, b, st = perf.build_workload(cls, 300, 800, 5)
            masked = [r for r in rows if mask[r[0]]]
            self.assertTrue(all(0 <= x <= y <= perf.DEN for _, lo, hi, _ in masked for x, y in zip(lo, hi)))
            widths = [y - x for _, lo, hi, _ in masked for x, y in zip(lo, hi)]
            if cls == 'exact':
                self.assertEqual(st['uncertain_masked'], 0)
                self.assertEqual(max(widths), 0)
            elif cls == 'narrow':
                self.assertEqual(st['uncertain_masked'], 300)
                self.assertTrue(0 < max(widths) <= 2 * perf.R_NARROW + 2)
            elif cls == 'wide':
                self.assertGreater(max(widths), 10 * perf.R_NARROW)
            elif cls == 'crossing':
                for _, lo, hi, den in masked:
                    for ch in range(3):
                        pieces = decision.cells(perf.Fraction(lo[ch], den), perf.Fraction(hi[ch], den),
                                                decision.CHANNEL_MAX[ch])
                        self.assertGreaterEqual(len(pieces), 2, 'interval must cross a quantizer tie')
            else:
                self.assertTrue(all(st['kinds'][k] > 20 for k in perf.CLASSES[:4]), st['kinds'])
                self.assertEqual(sum(st['kinds'].values()), 300)

    def test_candidates_are_not_trivial(self):
        _, mask, a, b, _ = perf.build_workload('mixed', 400, 1000, 9)
        idx = [3 * i + c for i in range(1000) if mask[i] for c in range(3)]
        self.assertGreater(len({a[i] for i in idx}), 100)
        self.assertGreater(sum(a[i] != b[i] for i in idx), 500)
        self.assertGreater(sum(a[i] == b[i] for i in idx), 50)


class SwapParsing(unittest.TestCase):
    NETBSD = 'total: 1048576 blocks allocated, 0 blocks used, 1048576 blocks available'  # recorded on maul

    def test_netbsd_blocks_are_512_bytes(self):
        self.assertEqual(perf.parse_swapctl_summary(self.NETBSD), {'total_kib': 524288, 'used_kib': 0})
        used = 'total: 1048576 blocks allocated, 20480 blocks used, 1028096 blocks available'
        self.assertEqual(perf.parse_swapctl_summary(used)['used_kib'], 10240)
        self.assertEqual(perf.parse_swapctl_summary(used.replace('20480', '3'))['used_kib'], 2)  # rounds up

    def test_kb_form_and_meminfo(self):
        self.assertEqual(perf.parse_swapctl_summary('total: 262132 KB allocated, 12 KB used, 262120 KB available'),
                         {'total_kib': 262132, 'used_kib': 12})
        self.assertEqual(perf.parse_meminfo_swap('SwapTotal:  8388604 kB\nSwapFree:   504896 kB\n'),
                         {'total_kib': 8388604, 'used_kib': 7883708})

    def test_unrecognized_text_is_unknown_not_zero(self):
        for text in ('', None, 'no swap devices', 'total: lots', 'total: 5 pages allocated, 0 pages used'):
            self.assertIsNone(perf.parse_swapctl_summary(text), text)
        self.assertIsNone(perf.parse_meminfo_swap('MemTotal: 1 kB\n'))

    def test_swap_info_uses_swapctl_off_linux_and_finds_sbin(self):
        saved = perf._run_text, perf.parse_meminfo_swap
        perf.parse_meminfo_swap = lambda text: None   # as on NetBSD, which has no /proc/meminfo
        tried = []

        def fake(argv):
            tried.append(argv[0])
            return self.NETBSD if argv[0] == '/sbin/swapctl' else None
        perf._run_text = fake
        try:
            info = perf.swap_info()
            self.assertEqual((info['used_kib'], info['total_kib'], info['source']), (0, 524288, 'swapctl -s'))
            self.assertEqual(tried, ['swapctl', '/sbin/swapctl'])
            perf._run_text = lambda argv: 'unexpected format'
            self.assertIsNone(perf.swap_info()['used_kib'])
            perf._run_text = lambda argv: None
            self.assertIsNone(perf.swap_info())
        finally:
            perf._run_text, perf.parse_meminfo_swap = saved


class Measurement(unittest.TestCase):
    def test_decision_source_is_the_reviewed_file(self):
        freeze = os.path.join(os.path.dirname(HERE), 'aa-decision-v1', 'source-freeze.sha256')
        with open(freeze) as f:
            want = dict(line.split()[::-1] for line in f)['tools/aa-decision-v1/decision.py']
        self.assertEqual(perf.file_sha256(os.path.join(perf.DECISION_DIR, 'decision.py')), want)
        self.assertEqual(PINS['decision_sha256'], want)

    def test_cell_matches_direct_evaluation_and_pins(self):
        rec = perf.run_cell('mixed', 100, 411, 1, 2, expected=PINS)
        self.assertEqual((rec['status'], rec['digest_check']), ('ok', 'match'))
        rows, mask, a, b, _ = perf.build_workload('mixed', 100, 411, 1)
        direct = decision.condition('map', 0, rows, mask, a, b)
        want = perf.result_digest(perf.condition_result(direct))
        self.assertEqual([e['digest'] for e in rec['evals']], [want, want])
        self.assertEqual(len(rec['aggregates']), 2)
        self.assertTrue(rec['aggregates'][0]['complete'])
        self.assertTrue(all(e['wall_s'] > 0 and e['cpu_s'] >= 0 for e in rec['evals']))

    def test_every_stage_a_cell_reproduces_its_pin(self):
        for cls in perf.CLASSES:
            rec = perf.run_cell(cls, 100, perf.auto_frame(100), 1, 1, expected=PINS)
            self.assertEqual(rec['digest_check'], 'match', cls)

    def test_tampered_pin_is_detected(self):
        pins = json.loads(json.dumps(PINS))
        key = perf.cell_key('exact', 100, 411, 1)
        pins['cells'][key]['result_digest'] = '0' * 64
        rec = perf.run_cell('exact', 100, 411, 1, 1, expected=pins)
        self.assertEqual((rec['status'], rec['digest_check']), ('digest_mismatch', 'MISMATCH'))

    def test_missing_os_instrumentation_degrades(self):
        saved = perf.resource, perf.signal
        perf.resource = perf.signal = None
        try:
            self.assertIsNone(perf.peak_rss_kib())
            self.assertIsNone(perf.rusage_snapshot())
            rec = perf.run_cell('narrow', 20, 100, 1, 1, wall_s=5, mem_kib=1024)
            self.assertEqual(rec['status'], 'ok')
            self.assertIsNone(rec['peak_rss_kib'])
            self.assertEqual(perf.capture_env('x')['label'], 'x')
        finally:
            perf.resource, perf.signal = saved

    def test_guard_limits(self):
        with self.assertRaises(perf.CellLimit):
            g = perf.Guard(0.001, None)
            time.sleep(0.01)
            g._tick(None, None)
        with self.assertRaises(perf.CellLimit):
            perf.Guard(None, 1)._tick(None, None)
        perf.Guard(None, 10 ** 9)._tick(None, None)

    def test_guard_timer_interrupts_pure_python_work(self):
        t = time.monotonic()
        with self.assertRaises(perf.CellLimit):
            with perf.Guard(0.1, None):
                while time.monotonic() - t < 5:
                    pass
        self.assertLess(time.monotonic() - t, 3)

    def test_source_is_python37_syntax(self):
        for d in (HERE, perf.DECISION_DIR):
            for name in os.listdir(d):
                if name.endswith('.py'):
                    with open(os.path.join(d, name)) as f:
                        ast.parse(f.read(), filename=name, feature_version=(3, 7))


def fake_cell(cls, masked, wall, rss_growth_kib=1024, status='ok'):
    ev = {'wall_s': wall, 'cpu_s': wall, 'digest': 'r'}
    return {'type': 'cell', 'class': cls, 'masked': masked, 'status': status, 'frame': masked * 4, 'seed': 1,
            'input': {'build_wall_s': 0.1, 'digest_wall_s': 0.1}, 'evals': [dict(ev, rep=0)] * 3,
            'aggregates': [{'wall_s': 0.01, 'cpu_s': 0.01, 'digest': 'a'}] * 3,
            'baseline_rss_kib': 20000, 'peak_rss_kib': 20000 + rss_growth_kib}


class Ladder(unittest.TestCase):
    LIM = {'cell_wall_s': 1000, 'mem_kib': 128 * 1024, 'total_wall_s': 5000}

    def test_projection_gates(self):
        ok, _, _ = run.project_next([fake_cell('mixed', 100, 0.1)], 1000, 3, self.LIM, 0)
        self.assertTrue(ok)
        ok, why, _ = run.project_next([fake_cell('mixed', 100, 40.0)], 1000, 3, self.LIM, 0)
        self.assertFalse(ok)
        self.assertIn('cell limit', why)
        ok, why, _ = run.project_next([fake_cell('mixed', 100, 0.1, rss_growth_kib=20000)], 1000, 3, self.LIM, 0)
        self.assertFalse(ok)
        self.assertIn('MiB', why)
        ok, why, _ = run.project_next([fake_cell('mixed', 100, 5.0)], 1000, 3, self.LIM, 4900)
        self.assertFalse(ok)
        self.assertIn('ladder budget', why)
        ok, _, _ = run.project_next([fake_cell('mixed', 100, 0.1, status='rss_limit')], 1000, 3, self.LIM, 0)
        self.assertFalse(ok)

    def args(self, out, **kw):
        base = dict(label='fake', out=out, stages='A,B,C', classes='exact,mixed', frame='auto', reps=3, seed=1,
                    cell_wall_s=1000, cpu_s=2000, mem_mib=128, total_wall_s=5000, swap_mib=8, max_load=1e9,
                    allow_busy=True, allow_unknown_swap=False, expected=run.EXPECTED)
        base.update(kw)
        return argparse.Namespace(**base)

    def read(self, path):
        with open(path) as f:
            return [json.loads(line) for line in f]

    def test_runs_all_stages_with_a_fake_runner(self):
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, 'r.jsonl')
            calls = []

            def runner(cell_args, wall):
                calls.append(cell_args)
                i = cell_args.index('--masked')
                return fake_cell(cell_args[1], int(cell_args[i + 1]), 0.001)
            self.assertEqual(run.ladder(self.args(out), runner), 0)
            self.assertEqual(len(calls), 6)
            recs = self.read(out)
            self.assertEqual([r['type'] for r in recs].count('gate'), 2)
            self.assertEqual(recs[0]['type'], 'env')
            self.assertEqual(recs[-1]['completed_stages'], ['A', 'B', 'C'])

    def test_bad_cell_stops_the_ladder(self):
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, 'r.jsonl')
            n = []

            def runner(cell_args, wall):
                n.append(1)
                return fake_cell('exact', 100, 0.001, status='wall_limit')
            self.assertEqual(run.ladder(self.args(out), runner), 1)
            self.assertEqual(len(n), 1)
            self.assertEqual(self.read(out)[-1]['stopped'], 'cell A/exact: wall_limit')

    def test_projection_blocks_advance(self):
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, 'r.jsonl')
            n = []

            def runner(cell_args, wall):
                n.append(1)
                return fake_cell(cell_args[1], 100, 50.0)
            self.assertEqual(run.ladder(self.args(out), runner), 0)
            self.assertEqual(len(n), 2)  # stage A only
            self.assertIn('gate before stage B', self.read(out)[-1]['stopped'])

    def test_abort_file(self):
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, 'r.jsonl')
            open(out + '.ABORT', 'w').close()
            self.assertEqual(run.ladder(self.args(out), lambda a, w: self.fail('must not run')), 3)

    def test_busy_host_refused(self):
        with tempfile.TemporaryDirectory() as d:
            self.assertEqual(run.ladder(self.args(os.path.join(d, 'r.jsonl'), max_load=-1.0, allow_busy=False),
                                        lambda a, w: self.fail('must not run')), 2)

    def test_real_child_process_end_to_end(self):
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, 'r.jsonl')
            code = run.main(['ladder', '--label', 'unit', '--out', out, '--stages', 'A', '--classes',
                             'exact,narrow', '--reps', '1', '--allow-busy', '--max-load', '1e9'])
            self.assertEqual(code, 0)
            cells = [r for r in self.read(out) if r['type'] == 'cell']
            self.assertEqual([(c['class'], c['status'], c['digest_check']) for c in cells],
                             [('exact', 'ok', 'match'), ('narrow', 'ok', 'match')])
            self.assertEqual(analyze.summary([out])['not_ok'], [])

    def with_swaps(self, readings, **kw):
        """Run a 2-stage fake ladder while perf.swap_info() yields `readings` in turn."""
        seq = iter(readings)
        saved = perf.swap_info
        perf.swap_info = lambda: next(seq, readings[-1])
        try:
            with tempfile.TemporaryDirectory() as d:
                out = os.path.join(d, 'r.jsonl')
                code = run.ladder(self.args(out, stages='A,B', classes='exact', **kw),
                                  lambda a, w: fake_cell('exact', int(a[a.index('--masked') + 1]), 0.001))
                return code, (self.read(out) if os.path.exists(out) else [])
        finally:
            perf.swap_info = saved

    def test_swap_growth_stops_and_unknown_never_counts_as_monitoring(self):
        ok = {'used_kib': 100, 'total_kib': 500000}
        code, recs = self.with_swaps([ok])
        self.assertEqual(code, 0)
        self.assertTrue(recs[0]['params']['swap_monitored'])
        grew = {'used_kib': 100 + 9 * 1024, 'total_kib': 500000}
        code, recs = self.with_swaps([ok, grew])
        self.assertEqual((code, recs[-1]['stopped']), (1, 'swap use grew %d KiB' % (9 * 1024)))
        code, recs = self.with_swaps([ok, {'used_kib': None, 'total_kib': None}])
        self.assertEqual((code, recs[-1]['stopped']), (1, 'swap reading became unavailable'))
        # unknown from the start: refuse, write nothing, unless the owner overrides
        unknown = {'used_kib': None, 'total_kib': None, 'raw': 'odd'}
        code, recs = self.with_swaps([unknown])
        self.assertEqual((code, recs), (2, []))
        code, recs = self.with_swaps([None])
        self.assertEqual((code, recs), (2, []))
        code, recs = self.with_swaps([unknown], allow_unknown_swap=True)
        self.assertEqual(code, 0)
        self.assertFalse(recs[0]['params']['swap_monitored'])

    def test_total_wall_budget_is_a_hard_stop_between_cells(self):
        clock = iter(range(0, 10 ** 6, 400))
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, 'r.jsonl')
            calls = []
            code = run.ladder(self.args(out, stages='A', classes='exact,narrow,wide', total_wall_s=700),
                              lambda a, w: calls.append(1) or fake_cell('exact', 100, 0.001),
                              now=lambda: next(clock))
            self.assertEqual(code, 1)
            self.assertLess(len(calls), 3)
            self.assertIn('wall budget', self.read(out)[-1]['stopped'])

    def test_selftest_passes(self):
        self.assertEqual(run.selftest(None), 0)


class Analysis(unittest.TestCase):
    def test_fit_and_stats(self):
        f = analyze.fit([1, 2, 3, 4], [3, 5, 7, 9])
        self.assertAlmostEqual(f['slope'], 2)
        self.assertAlmostEqual(f['intercept'], 1)
        self.assertAlmostEqual(f['r2'], 1)
        self.assertIsNone(analyze.fit([1], [1]))
        s = analyze.stats([1.0, 2.0, 3.0])
        self.assertEqual((s['min'], s['median'], s['max']), (1.0, 2.0, 3.0))

    def test_summary_cross_host_and_expected(self):
        with tempfile.TemporaryDirectory() as d:
            paths = []
            for label, factor in (('fast', 1.0), ('slow', 30.0)):
                p = os.path.join(d, label + '.jsonl')
                paths.append(p)
                with open(p, 'w') as f:
                    env = perf.capture_env(label)
                    f.write(json.dumps({'type': 'env', 'env': env, 'params': {}}) + '\n')
                    for m in (100, 1000):
                        c = fake_cell('mixed', m, 0.001 * m * factor)
                        c['input'].update({'digest': 'i%d' % m, 'build_cpu_s': 0, 'uncertain_masked': m // 2})
                        c['evals'] = [dict(e, digest='r%d' % m, cpu_s=e['wall_s']) for e in c['evals']]
                        c['aggregates'] = [dict(g, digest='a%d' % m) for g in c['aggregates']]
                        f.write(json.dumps(dict(c, stage='x')) + '\n')
            s = analyze.summary(paths)
            self.assertEqual({c['digests_identical'] for c in s['cross_host']}, {True})
            self.assertAlmostEqual(s['cross_host'][0]['cpu_ratio_second_over_first'], 30.0)
            self.assertAlmostEqual(s['scaling']['fast|mixed']['loglog_exponents_between_stages'][0], 1.0, places=6)
            self.assertAlmostEqual(s['extrapolation']['fast|mixed']['condition_74796_cpu_s_from_top_stage'], 74.796)
            self.assertIn('Cross-host', analyze.markdown(s))
            self.assertEqual(s['bookkeeping'], [])
            with open(paths[0]) as f:
                lines = f.read().splitlines()
            env = json.loads(lines[0])
            env['env']['label'] = 'fixed'
            env['params']['frame'] = '307200'
            q = os.path.join(d, 'fixed.jsonl')
            with open(q, 'w') as f:
                f.write(json.dumps(env) + '\n' + '\n'.join(l.replace('"label"', '"label"') for l in lines[1:2]) + '\n')
            fixed = analyze.summary([q])
            self.assertEqual(len(fixed['bookkeeping']), 1)
            self.assertEqual(fixed['extrapolation'], {})
            self.assertIn('Fixed-frame', analyze.markdown(fixed))
            pins = analyze.expected(paths[:1])
            self.assertEqual(pins['cells'][perf.cell_key('mixed', 100, 400, 1)]['input_digest'], 'i100')


if __name__ == '__main__':
    unittest.main()
