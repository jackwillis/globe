# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Independent synthetic controls for coupled quality-decision bounds.

No geographic inputs, candidates or references. Expected values come from
hand arithmetic or from direct lattice enumeration that never calls
scalar_range/cells, so a shared bug in the cell algebra cannot hide itself.
"""
import importlib.util
import itertools
import math
import random
import unittest
from fractions import Fraction as F
from pathlib import Path

from decision import (CHANNEL_MAX, FAIL, FLOOR, INCONCLUSIVE, PASS, QUANTIZED,
                      Range, condition, decide, denominator_state, expand, local_guard,
                      numerical_conjunction, quantize, scalar_range, style_decision,
                      with_geometry)

DEN = 255 * 62


# ---- independent oracle -------------------------------------------------------
def code_of(r, m):
    return math.floor(m*r + F(1, 2))


def x_of(code, m):
    return F((code << 3 | code >> 2) if m == 31 else (code << 2 | code >> 4), 255)


def oracle(lower, upper, weights, a, b, m):
    """Enumerate every lattice point (all kinks and tie boundaries lie on it).

    A tie belongs to the upper code; the lower code's left limit at a tie in
    (lower, upper] is recorded as an unattained value.
    """
    n = math.lcm(255, 2*m, lower.denominator, upper.denominator)
    lo, hi = lower*n, upper*n
    wa, wb, wq = weights
    values = []

    def f(r, q):
        return wa*abs(a-r) + wb*abs(b-r) + wq*abs(x_of(q, m)-r)
    for j in range(int(lo), int(hi)+1):
        r = F(j, n)
        q = code_of(r, m)
        values.append((f(r, q), True))
        if j > lo and (m*r + F(1, 2)).denominator == 1:
            values.append((f(r, q-1), False))
    low = min(v for v, _ in values)
    high = max(v for v, _ in values)
    return (low, any(t for v, t in values if v == low), high, any(t for v, t in values if v == high))


WEIGHTS = {'E_A': (1, 0, 0), 'E_B': (0, 1, 0), 'E_Q': (0, 0, 1), 'N': (1, -1, 0),
           'D': (1, 0, -1), 'H': (F(4, 5), -1, F(1, 5))}


def terms_of(weights, a, b):
    return [(F(w), t) for w, t in zip(weights, (a, b, QUANTIZED)) if w != 0]


def as_tuple(r):
    return (r.lower, r.lower_attained, r.upper, r.upper_attained)


def frame(lo, hi, a, b, den=DEN):
    """One-pixel frame, one included pixel; lo/hi integer RGB numerators."""
    return [(0, tuple(lo), tuple(hi), den)], b'\1', bytes(a), bytes(b)


def style(name, args, ids=range(16)):
    c = condition(name, 0, *args)
    return [with_geometry(c, i) for i in ids]


def exact_rows(n, den=1):
    return [(i, (0, 0, 0), (0, 0, 0), den) for i in range(n)]


def direct(lo, hi, den, a, b):
    """Per-channel direct formulas at ONE exact reference value (lo == hi)."""
    out = {'E_A': 0, 'E_B': 0, 'E_Q': 0}
    for c in range(3):
        r, m = F(lo[c], den), CHANNEL_MAX[c]
        q = x_of(code_of(r, m), m)
        out['E_A'] += abs(F(a[c], 255)-r)
        out['E_B'] += abs(F(b[c], 255)-r)
        out['E_Q'] += abs(q-r)
    return {k: v/3 for k, v in out.items()}


class ScalarRangeTests(unittest.TestCase):
    def test_lattice_oracle_with_attainment(self):
        rnd = random.Random(20261119)
        unattained = 0
        for _ in range(400):
            m = rnd.choice((31, 63))
            n = math.lcm(255, 2*m)   # interval ends lie on this grid, so the oracle grid is n
            start = rnd.randrange(0, n-1)
            if rnd.random() < .5:   # start just below a tie so the window crosses it
                start = max(0, (2*rnd.randrange(1, m)+1)*n//(2*m) - rnd.randrange(0, 4))
            width = rnd.choice((0, 1, 3, 20, 2*n//m))
            lower, upper = F(start, n), F(min(n, start+width), n)
            a, b = F(rnd.randrange(256), 255), F(rnd.randrange(256), 255)
            if rnd.random() < .4:
                a = F(round(float(lower)*255), 255)
            for name, weights in WEIGHTS.items():
                expected = oracle(lower, upper, tuple(F(w) for w in weights), a, b, m)
                actual = scalar_range(lower, upper, terms_of(weights, a, b), m)
                self.assertEqual(as_tuple(actual), expected, (name, m, lower, upper, a, b))
                unattained += not (expected[1] and expected[3])
        self.assertGreater(unattained, 20)   # the open-cell case is actually exercised

    def test_ties_and_expansion_by_hand(self):
        self.assertEqual([expand(c, 31) for c in (0, 1, 16, 31)], [0, 8, 132, 255])
        self.assertEqual([expand(c, 63) for c in (0, 1, 32, 63)], [0, 4, 130, 255])
        self.assertEqual((quantize(F(1, 62), 31), quantize(F(1, 62)-F(1, 10**9), 31)), (1, 0))
        self.assertEqual(quantize(F(1, 2), 31), 16)       # 31/2 + 1/2 = 16 exactly
        self.assertEqual(quantize(F(1, 2), 63), 32)
        self.assertEqual(quantize(F(1), 31), 31)
        # Exactly at the tie 1/62 the reference belongs to code 1 (upper): E_Q = |8/255 - 1/62|.
        tie = F(1, 62)
        at = scalar_range(tie, tie, [(F(1), QUANTIZED)], 31)
        self.assertEqual(as_tuple(at), (F(8, 255)-tie, True, F(8, 255)-tie, True))
        # Just below the tie the lower code applies: E_Q = R.
        below = scalar_range(tie-F(1, 10**6), tie-F(1, 10**6), [(F(1), QUANTIZED)], 31)
        self.assertEqual(below.lower, tie-F(1, 10**6))
        # Across [0, 1/62] code 0 owns [0, 1/62): sup E_Q = 1/62 is a limit, not attained,
        # because the attained tie point only reaches 8/255 - 1/62 < 1/62.
        span = scalar_range(F(0), tie, [(F(1), QUANTIZED)], 31)
        self.assertEqual(as_tuple(span), (F(0), True, tie, False))
        # A closed top cell: R = 1 has code M and expands to exactly 255/255.
        top = scalar_range(F(1), F(1), [(F(1), QUANTIZED)], 63)
        self.assertEqual(as_tuple(top), (F(0), True, F(0), True))
        self.assertEqual(scalar_range(F(1)-F(1, 126), F(1), [(F(1), QUANTIZED)], 63).lower, F(0))

    def test_invalid_scalar_inputs(self):
        q = [(F(1), QUANTIZED)]
        for args in [(F(1, 2), F(1, 4), q, 31), (F(-1, 10), F(1, 2), q, 31), (F(0), F(2), q, 31),
                     (0, F(1), q, 31), (F(0), F(1), q, None), (F(0), F(1), q, 255),
                     (F(0), F(1), [], 31), (F(0), F(1), [(1, F(0))], 31),
                     (F(0), F(1), [(F(1), F(2))], 31), (F(0), F(1), [(F(1), 'X')], 31)]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                scalar_range(*args)
        with self.assertRaises(ValueError):
            Range(F(1), True, F(0), True)
        with self.assertRaises(ValueError):
            Range(F(0), False, F(0), False)


class CoupledBoundsTests(unittest.TestCase):
    def test_identical_candidates_cancel_and_h_is_minus_d_over_five(self):
        rnd = random.Random(5)
        for _ in range(40):
            lo = [rnd.randrange(0, DEN//2) for _ in range(3)]
            hi = [x + rnd.randrange(0, DEN//2) for x in lo]
            a = bytes(rnd.randrange(256) for _ in range(3))
            c = condition('map', 0, *frame(lo, hi, a, a))
            self.assertEqual(as_tuple(c.ranges['N']), (0, True, 0, True))
            self.assertEqual(c.ranges['H'], c.ranges['D'].scale(F(-1, 5)))
            self.assertEqual(c.ranges['E_A'], c.ranges['E_B'])
        # Fully unknown reference: independent subtraction would be [-1,1] for N.
        wide = condition('map', 0, *frame([0]*3, [DEN]*3, [200]*3, [200]*3))
        self.assertEqual(as_tuple(wide.ranges['N']), (0, True, 0, True))

    def test_h_matches_per_channel_oracle_and_is_tighter_than_relaxation(self):
        lo, hi = [1965, 87, 3085], [2482, 655, 3365]
        a, b = [144, 130, 80], [180, 9, 113]
        c = condition('map', 0, *frame(lo, hi, a, b))
        expect = {}
        for name, weights in WEIGHTS.items():
            parts = [oracle(F(lo[k], DEN), F(hi[k], DEN), tuple(F(w) for w in weights),
                            F(a[k], 255), F(b[k], 255), CHANNEL_MAX[k]) for k in range(3)]
            expect[name] = (sum(p[0] for p in parts)/3, all(p[1] for p in parts),
                            sum(p[2] for p in parts)/3, all(p[3] for p in parts))
            self.assertEqual(as_tuple(c.ranges[name]), expect[name], name)
        n, d, h = c.ranges['N'], c.ranges['D'], c.ranges['H']
        relaxed = (n.lower - d.upper/5, n.upper - d.lower/5)
        self.assertGreaterEqual(h.lower, relaxed[0])
        self.assertLessEqual(h.upper, relaxed[1])
        self.assertTrue(h.lower > relaxed[0] and h.upper < relaxed[1])   # strictly tighter

    def test_false_midpoint_pass_and_fail_are_inconclusive(self):
        cases = [  # (lo, hi, A, B, midpoint verdict)
            ([1965, 87, 3085], [2482, 655, 3365], [144, 130, 80], [180, 9, 113], PASS),
            ([989, 1407, 4122], [1296, 1651, 4800], [10, 211, 27], [58, 174, 64], FAIL)]
        for lo, hi, a, b, midpoint_verdict in cases:
            args = frame(lo, hi, a, b)
            # Exact midpoint reference: numerators (L+U) over twice the denominator.
            point = tuple(x+y for x, y in zip(lo, hi))
            mid_rows = [(0, point, point, 2*DEN)]
            mid_condition = condition('map', 0, mid_rows, b'\1', bytes(a), bytes(b))
            d, h = mid_condition.ranges['D'], mid_condition.ranges['H']
            self.assertEqual((d.lower, h.lower), (d.upper, h.upper))
            self.assertGreater(d.lower, FLOOR)
            self.assertEqual(PASS if h.lower >= 0 else FAIL, midpoint_verdict)
            interval = style_decision(style('map', args))
            self.assertEqual(interval['denominator'], 'ELIGIBLE')
            self.assertEqual(interval['status'], INCONCLUSIVE)
            self.assertLess(interval['H'].lower, 0)
            self.assertGreater(interval['H'].upper, 0)
            # Both signs are realized by actual reference worlds (independent oracle).
            parts = [oracle(F(lo[k], DEN), F(hi[k], DEN), tuple(F(w) for w in WEIGHTS['H']),
                            F(a[k], 255), F(b[k], 255), CHANNEL_MAX[k]) for k in range(3)]
            self.assertEqual(sum(p[0] for p in parts)/3, interval['H'].lower)
            self.assertEqual(sum(p[2] for p in parts)/3, interval['H'].upper)


class StyleDecisionTests(unittest.TestCase):
    def test_h_threshold_equality_and_sign(self):
        # R = 0 exactly, A = 255: E_A = 1, E_Q = 0, D = 1. H = 4/5 - E_B.
        for b, status, h_sign in [(203, PASS, 1), (204, PASS, 0), (205, FAIL, -1)]:
            d = style_decision(style('map', frame([0]*3, [0]*3, [255]*3, [b]*3, 1)))
            self.assertEqual((d['denominator'], d['status']), ('ELIGIBLE', status), b)
            self.assertEqual((d['H'].lower > 0) - (d['H'].lower < 0), h_sign)
            self.assertEqual(d['D'].lower, 1)

    def test_h_upper_zero_is_not_a_failure_and_straddle_is_inconclusive(self):
        # A = 255, R below every tie so Q = 0: per channel H = 4/5 - b/255 + 2R/5. Red/blue
        # use b = 205 (H = -1/255 + 2R/5, zero exactly at R = 1/102 < first red tie 1/62);
        # green is exact with b = 204 (H = 0). Upper H is exactly 0 and attained, so some
        # worlds pass and the rest fail: not a provable failure.
        args = frame([0]*3, [5, 0, 5], [255]*3, [205, 204, 205], 510)
        d = style_decision(style('map', args))
        self.assertEqual(as_tuple(d['H']), (F(-2, 765), True, F(0), True))
        self.assertEqual((d['denominator'], d['status'], d['reason']),
                         ('ELIGIBLE', INCONCLUSIVE, 'H straddles 0'))
        # One numerator unit lower the supremum is negative and the failure is proved.
        d = style_decision(style('map', frame([0]*3, [4, 0, 4], [255]*3, [205, 204, 205], 510)))
        self.assertEqual((d['H'].upper < 0, d['status']), (True, FAIL))
        # Past the red tie (10/510 > 1/62) Q jumps to 8/255 and H straddles clearly.
        d = style_decision(style('map', frame([0]*3, [10, 0, 10], [255]*3, [205, 204, 205], 510)))
        self.assertEqual(d['status'], INCONCLUSIVE)

    def test_floor_boundary_equality_and_one_unit(self):
        # n = 512, R = 0: D = S / (765*512) with S = sum of A bytes; floor needs S = 3825.
        def build(s):
            a = bytearray(3*512)
            for i in range(0, s//255):
                a[3*i] = 255
            if s % 255:
                a[3*(s//255)] = s % 255
            b = bytearray(3*512)      # B worse than A everywhere A has signal and elsewhere
            for i in range(512):
                b[3*i:3*i+3] = bytes([255, 255, 255]) if i < 15 else bytes(3)
            return exact_rows(512), b'\1'*512, bytes(a), bytes(b)
        results = {}
        for s in (3824, 3825, 3826):
            args = build(s)
            d = style_decision(style('relief', args))
            results[s] = (d['denominator'], d['status'])
            self.assertEqual(d['D'].lower, F(s, 765*512))
        # H.upper < 0 in the eligible case: B is much worse than A, so FAIL is provable.
        self.assertEqual(results, {3824: ('FLOOR_LIMITED', INCONCLUSIVE),
                                   3825: ('FLOOR_LIMITED', INCONCLUSIVE),   # D == floor: undefined
                                   3826: ('ELIGIBLE', FAIL)})
        self.assertEqual(F(3825, 765*512), FLOOR)
        # Even a definitely negative H cannot FAIL a floor-limited denominator.
        d = style_decision(style('relief', build(3825)))
        self.assertLess(d['H'].upper, 0)
        self.assertIn('floor-limited', d['reason'])

    def test_crossing_denominator_is_inconclusive_even_when_h_is_resolved(self):
        rows, mask, a, b = exact_rows(512), b'\1'*512, bytearray(3*512), bytearray(3*512)
        for i in range(14):
            a[3*i] = 255
        a[3*14] = 1                      # known part of S: 14*255 + 1 = 3571
        for i in range(512):
            b[3*i:3*i+3] = bytes([255]*3) if i < 15 else bytes(3)
        # Pixel 20: A red = 255, reference in [0, 3/255] (Q = 0): contributes 255 - 2*R*255,
        # i.e. S in [3571 + 249, 3571 + 255] = [3820, 3826] around the floor value 3825.
        rows[20] = (20, (0, 0, 0), (3, 0, 0), 255)
        a[3*20] = 255
        c = condition('map', 0, rows, mask, bytes(a), bytes(b))
        self.assertEqual((c.ranges['D'].lower, c.ranges['D'].upper),
                         (F(3820, 765*512), F(3826, 765*512)))
        self.assertTrue(c.ranges['D'].lower < FLOOR < c.ranges['D'].upper)
        self.assertLess(c.ranges['H'].upper, 0)       # H alone would fail B; D blocks the verdict
        d = style_decision([with_geometry(c, i) for i in range(16)])
        self.assertEqual((d['denominator'], d['status']), ('CROSSING', INCONCLUSIVE))

    def test_negative_or_zero_denominator_is_never_a_ratio(self):
        # A already equals the reference: D = 0 - 0 (floor-limited), gap undefined.
        d = style_decision(style('map', frame([0]*3, [0]*3, [0]*3, [10]*3, 1)))
        self.assertEqual((d['denominator'], d['status'], d['G_conservative']),
                         ('FLOOR_LIMITED', INCONCLUSIVE, None))
        # A farther from R than Q is at worst: still no division.
        d = style_decision(style('map', frame([0]*3, [0]*3, [0]*3, [0]*3, 1)))
        self.assertIsNone(d['G_conservative'])
        self.assertEqual(denominator_state(Range(F(-1, 2), True, F(1, 200), True)), 'FLOOR_LIMITED')

    def test_denominator_state_boundaries(self):
        for lower, upper, expected in [
                (FLOOR, FLOOR, 'FLOOR_LIMITED'), (F(0), FLOOR, 'FLOOR_LIMITED'),
                (FLOOR, FLOOR+F(1, 10**9), 'CROSSING'),      # lower == floor is not eligible
                (FLOOR-F(1, 10**9), FLOOR+F(1, 10**9), 'CROSSING'),
                (FLOOR+F(1, 10**9), F(1, 2), 'ELIGIBLE')]:
            self.assertEqual(denominator_state(Range(lower, True, upper, True)), expected)

    def test_identical_candidates_with_uncertain_reference_fail_provably(self):
        # B = A closes no gap: H = -D/5 < 0 when D is eligible, for a wide reference too.
        args = frame([0]*3, [DEN//20]*3, [255]*3, [255]*3)
        d = style_decision(style('map', args))
        self.assertEqual(d['N'].lower, 0)
        self.assertEqual(d['N'].upper, 0)
        self.assertEqual((d['denominator'], d['status']), ('ELIGIBLE', FAIL))
        self.assertEqual(d['H'], d['D'].scale(F(-1, 5)))

    def test_ratio_of_means_not_mean_of_ratios(self):
        rnd = random.Random(11)
        conds, expected = [], []
        for i in range(16):
            lo = [rnd.randrange(0, 40) for _ in range(3)]
            a = [rnd.randrange(60, 256) for _ in range(3)]
            b = [rnd.randrange(0, 256) for _ in range(3)]
            conds.append(condition('relief', i, *frame(lo, lo, a, b, 255)))
            v = direct(lo, lo, 255, a, b)
            expected.append((v['E_A']-v['E_B'], v['E_A']-v['E_Q']))
        n = sum(x for x, _ in expected)/16
        d = sum(y for _, y in expected)/16
        result = style_decision(conds)
        self.assertEqual((result['N'].lower, result['N'].upper), (n, n))
        self.assertEqual((result['D'].lower, result['D'].upper), (d, d))
        self.assertEqual((result['H'].lower, result['H'].upper), (n-d/5, n-d/5))
        self.assertEqual(result['G_conservative'], {'lower': n/d, 'upper': n/d})
        self.assertNotEqual(n/d, sum(x/y for x, y in expected)/16)   # not the mean of ratios
        self.assertEqual(result['status'], PASS if n-d/5 >= 0 else FAIL)

    def test_map_and_relief_are_never_pooled(self):
        good = style('map', frame([0]*3, [0]*3, [255]*3, [0]*3, 1))
        bad = style('relief', frame([0]*3, [0]*3, [255]*3, [255]*3, 1))
        result = decide({'map': good, 'relief': bad})
        self.assertEqual((result['points']['map']['status'], result['points']['relief']['status']),
                         (PASS, FAIL))
        self.assertEqual(result['point_conjunction'], FAIL)
        self.assertEqual(decide({'map': good})['points']['map']['status'], PASS)
        self.assertEqual(result['points']['map']['H'], decide({'map': good})['points']['map']['H'])
        with self.assertRaises(ValueError):
            style_decision(good[:8] + bad[8:])
        with self.assertRaises(ValueError):
            decide({'map': bad})       # a relief-tagged frame cannot stand in for map

    def test_partial_conjunction_is_never_a_complete_pass(self):
        fr = frame([0]*3, [0]*3, [255]*3, [0]*3, 1)
        good, also = style('map', fr), style('relief', fr)
        full = decide({'map': good, 'relief': also})
        self.assertTrue(full['complete'])
        self.assertEqual((full['point_conjunction'], full['guard_conjunction']), (PASS, PASS))
        self.assertEqual(sum(len(g) for g in full['guards'].values()), 32)
        part = decide({'map': good})
        self.assertFalse(part['complete'])
        self.assertEqual(part['points']['map']['status'], PASS)      # still inspectable
        self.assertEqual((part['point_conjunction'], part['guard_conjunction']),
                         (INCONCLUSIVE, INCONCLUSIVE))
        bad = style('relief', frame([0]*3, [0]*3, [255]*3, [255]*3, 1))
        self.assertEqual(decide({'map': good, 'relief': bad})['point_conjunction'], FAIL)
        with self.assertRaises(ValueError):    # 15 geometries cannot be a style
            decide({'map': good[:15], 'relief': also})

    def test_three_valued_conjunction(self):
        self.assertEqual(numerical_conjunction([PASS, PASS]), PASS)
        self.assertEqual(numerical_conjunction([PASS, INCONCLUSIVE]), INCONCLUSIVE)
        self.assertEqual(numerical_conjunction([INCONCLUSIVE, FAIL, PASS]), FAIL)
        with self.assertRaises(ValueError):
            numerical_conjunction([])
        with self.assertRaises(ValueError):
            numerical_conjunction(['pass'])

    def test_invalid_style_inputs(self):
        ok = style('map', frame([0]*3, [0]*3, [255]*3, [0]*3, 1))
        other = style('relief', frame([0]*3, [0]*3, [255]*3, [0]*3, 1))
        for bad in (ok[:15], ok + ok[:1], [ok[0]]*16, ok[:15] + [other[15]], tuple(ok), ok[:15] + [1]):
            with self.subTest(n=len(bad)), self.assertRaises(ValueError):
                style_decision(bad)
        for args in [('mapp', 0), ('map', 16), ('map', -1), ('map', True), ('map', 1.0)]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                condition(*args, *frame([0]*3, [0]*3, [0]*3, [0]*3, 1))


class LocalGuardTests(unittest.TestCase):
    def test_floor_branch_equality_and_boundaries(self):
        # A == R everywhere: E_A = 0, so the 2/1024 term governs. n = 512: E_B = K/(765*512).
        def build(k):
            b = bytearray(3*512)
            for i in range(k // 255):
                b[3*i] = 255
            if k % 255:
                b[3*(k//255)] = k % 255
            return exact_rows(512), b'\1'*512, bytes(3*512), bytes(b)
        out = {}
        for k in (764, 765, 766):
            g = local_guard(condition('map', 0, *build(k)))
            out[k] = (g['branch'], g['status'], g['J']['upper'])
        self.assertEqual(out[764][:2], ('floor', PASS))
        self.assertEqual(out[765], ('floor', PASS, F(0)))          # J == 0 passes
        self.assertEqual(out[766][:2], ('floor', FAIL))
        self.assertEqual(out[766][2], F(1, 765*512))

    def test_ratio_branch_equality_and_boundaries(self):
        # R = 0, A = 100: E_A = 100/255 >> 20/1024, so the 10% term governs; J = (b - 110)/255.
        out = {}
        for b in (109, 110, 111):
            g = local_guard(condition('map', 0, *frame([0]*3, [0]*3, [100]*3, [b]*3, 1)))
            out[b] = (g['branch'], g['status'], g['J']['lower'])
        self.assertEqual(out, {109: ('ratio', PASS, F(-1, 255)), 110: ('ratio', PASS, F(0)),
                               111: ('ratio', FAIL, F(1, 255))})

    def test_lower_bound_equal_to_zero_is_not_a_violation(self):
        # R in [0, 3/255], A = 100, B = 110: per channel J = 0.1*R >= 0 with J = 0 at R = 0.
        # Zero passes, so J.lower == 0 with J.upper > 0 is unresolved, not a violation.
        g = local_guard(condition('map', 0, *frame([0]*3, [3]*3, [100]*3, [110]*3, 255)))
        self.assertEqual((g['branch'], g['J']['lower'], g['status']), ('ratio', F(0), INCONCLUSIVE))
        self.assertEqual(g['J']['upper'], F(3, 2550))

    def test_mixed_branch_bounds_against_world_enumeration(self):
        rnd = random.Random(99)
        relaxed = mixed = 0
        for _ in range(120):
            lo = [rnd.randrange(0, 8) for _ in range(3)]
            hi = [x + rnd.randrange(0, 4) for x in lo]
            a = [rnd.randrange(0, 12) for _ in range(3)]
            b = [rnd.randrange(0, 40) for _ in range(3)]
            g = local_guard(condition('map', 0, *frame(lo, hi, a, b, 255)))
            worlds = []
            for ref in itertools.product(*[range(l, h+1) for l, h in zip(lo, hi)]):
                ea = sum(abs(F(x, 255)-F(r, 255)) for x, r in zip(a, ref))/3
                eb = sum(abs(F(x, 255)-F(r, 255)) for x, r in zip(b, ref))/3
                worlds.append(eb - ea - max(ea/10, F(2, 1024)))
            self.assertEqual(g['J']['lower'], min(worlds), (lo, hi, a, b))     # exact infimum
            self.assertGreaterEqual(g['J']['upper'], max(worlds))              # sound supremum
            if g['upper_exact']:
                self.assertEqual(g['J']['upper'], max(worlds))
            mixed += g['branch'] == 'mixed'
            relaxed += g['J']['upper'] > max(worlds)   # none found in the author's searches
        self.assertGreater(mixed, 10)
        self.assertGreaterEqual(relaxed, 0)   # looseness of the labelled relaxation is unproven

    def test_identical_candidates_pass_even_with_unknown_reference(self):
        g = local_guard(condition('map', 0, *frame([0]*3, [DEN]*3, [77]*3, [77]*3)))
        self.assertEqual(g['status'], PASS)
        self.assertLessEqual(g['J']['upper'], -F(2, 1024))

    def test_status_when_unresolved(self):
        # B can be either side of the allowance: neither proved nor violated.
        wide = local_guard(condition('map', 0, *frame([0]*3, [255]*3, [10]*3, [200]*3, 255)))
        self.assertEqual(wide['status'], INCONCLUSIVE)


class CrossCheckTests(unittest.TestCase):
    def test_agrees_with_fixed_mask_for_reference_only_quantities(self):
        path = Path(__file__).resolve().parent.parent / 'aa-interval-v1' / 'metrics.py'
        if not path.exists():
            self.skipTest('aa-interval-v1 not in this tree')
        spec = importlib.util.spec_from_file_location('interval_metrics', path)
        metrics = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(metrics)
        rnd = random.Random(21)
        for _ in range(20):
            size = rnd.randrange(2, 7)
            mask = bytes(rnd.randrange(2) for _ in range(size)) or b'\1'
            if not any(mask):
                mask = b'\1' + mask[1:]
            rows = []
            for i in range(size):
                lo = [rnd.randrange(0, 200) for _ in range(3)]
                rows.append((i, tuple(lo), tuple(min(255, x + rnd.randrange(0, 80)) for x in lo), 255))
            a = bytes(rnd.randrange(256) for _ in range(3*size))
            b = bytes(rnd.randrange(256) for _ in range(3*size))
            expected = metrics.fixed_mask(rows, mask, a, b)
            ours = condition('map', 0, rows, mask, a, b)
            for ours_key, theirs in (('E_A', 'mae_a'), ('E_B', 'mae_b'), ('N', 'a_minus_b')):
                self.assertEqual((ours.ranges[ours_key].lower, ours.ranges[ours_key].upper),
                                 (expected[theirs].lower, expected[theirs].upper))
            self.assertTrue(all(ours.ranges[k].lower_attained and ours.ranges[k].upper_attained
                                for k in ('E_A', 'E_B', 'N')))

    def test_invalid_frames_are_rejected(self):
        row = (0, (0,)*3, (1,)*3, 1)
        for args in [([], b'\1', bytes(3), bytes(3)), ([row, row], b'\1', bytes(3), bytes(3)),
                     ([row], b'\0', bytes(3), bytes(3)), ([row], b'\2', bytes(3), bytes(3)),
                     ([row], b'\1', bytes(2), bytes(3)), ([row], bytearray(b'\1'), bytes(3), bytes(3)),
                     ([(0, (0,)*3, (1,)*3, 0)], b'\1', bytes(3), bytes(3)),
                     ([(0, (1,)*3, (0,)*3, 1)], b'\1', bytes(3), bytes(3)),
                     ([(0, (0,)*3, (2,)*3, 1)], b'\1', bytes(3), bytes(3)),
                     ([(0, (False,)*3, (1,)*3, 1)], b'\1', bytes(3), bytes(3)),
                     ([(0, (0,)*3, (1,)*3, True)], b'\1', bytes(3), bytes(3)),
                     ([(1, (0,)*3, (1,)*3, 1)], b'\1', bytes(3), bytes(3)),
                     ([(0, (0,)*3, (1,)*3, 1), (1, (0,)*3, (1,)*3, 1)], b'\1', bytes(3), bytes(3)),
                     # duplicate plus omission with equal count must still fail
                     ([(0, (0,)*3, (1,)*3, 1), (0, (0,)*3, (1,)*3, 1)], b'\1\1', bytes(6), bytes(6))]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                condition('map', 0, *args)

    def test_unmasked_rows_are_validated_but_not_scored(self):
        rows = [(0, (0,)*3, (0,)*3, 1), (1, (0,)*3, (1,)*3, 1)]
        c = condition('map', 0, rows, b'\1\0', bytes([255]*3 + [0]*3), bytes(6))
        self.assertEqual(c.mask_count, 1)
        self.assertEqual(c.ranges['E_A'], Range(F(1), True, F(1), True))
        with self.assertRaises(ValueError):
            condition('map', 0, [rows[0], (1, (1,)*3, (0,)*3, 1)], b'\1\0', bytes(6), bytes(6))


if __name__ == '__main__':
    unittest.main(verbosity=2)
