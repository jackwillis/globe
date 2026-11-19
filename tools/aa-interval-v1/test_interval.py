# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Independent analytical/exhaustive synthetic controls. No geographic inputs."""
import copy
import json
import struct
import tempfile
import unittest
from fractions import Fraction as F
from pathlib import Path
from adapter import (DEN, CoarseAdapter, Paths, digest, geometry_identity,
                     strict_json, validate_tile)
from metrics import Interval, at_least, fixed_mask, round_codes, scalar_losses


def tile():
    # Entirely synthetic black reference, created without importing producer code.
    return {'tile_index': 0, 'tile': [0, 0], 'complete': True, 'target_den': 2560,
            'target_attained': True, 'max_depth_limit': 16,
            'fair_pixel_cell_quota': 100000, 'coordinate_scale_exponent': 46,
            'pixels': [{'pixel': [k % 8, k // 8], 'lower_numerator': [0, 0, 0],
                        'upper_numerator': [0, 0, 0], 'denominator': DEN,
                        'precision_reached': True, 'state': 'precision_reached',
                        'cell_visits': 1, 'pixel_cell_quota': 100000, 'deepest': 0,
                        'area_denominator': 4**16, 'unresolved_area_numerator': 0,
                        'quantized_codes': [[0, 0], [0, 0], [0, 0]]} for k in range(64)]}


class MetricsTests(unittest.TestCase):
    def test_exhaustive_independent_scalar_oracle(self):
        # On this lattice all candidate kinks are in the enumerated worlds.
        # Expected losses come from direct integer distances, no interval helper.
        for lo in range(9):
            for hi in range(lo, 9):
                for a in range(9):
                    for b in range(9):
                        expected = ([abs(a-r) for r in range(lo, hi+1)],
                                    [abs(b-r) for r in range(lo, hi+1)],
                                    [abs(a-r)-abs(b-r) for r in range(lo, hi+1)])
                        actual = scalar_losses(F(lo, 8), F(hi, 8), F(a, 8), F(b, 8))
                        self.assertEqual(actual, tuple(Interval(F(min(v), 8), F(max(v), 8)) for v in expected))

    def test_identical_candidates_cancel_uncertainty_exactly(self):
        for a in range(256):
            ea, eb, delta = scalar_losses(F(0), F(1), F(a, 255), F(a, 255))
            self.assertEqual(ea, eb)
            self.assertEqual(delta, Interval(F(0), F(0)))
        # Independent subtraction would be [-1,1]; shared reference is exactly 0.
        value = fixed_mask([(0, (0,)*3, (1,)*3, 1)], b'\1', bytes(3), bytes(3))
        self.assertEqual(value['a_minus_b'], Interval(F(0), F(0)))

    def test_analytical_mask_and_normalization(self):
        rows = [(0, (0,)*3, (0,)*3, 1), (1, (1,)*3, (1,)*3, 2),
                (2, (0,)*3, (1,)*3, 1)]
        a, b = bytes([0]*3+[255]*6), bytes(9)
        result = fixed_mask(rows, b'\1\1\0', a, b)
        self.assertEqual(result['mae_a'], Interval(F(1, 4), F(1, 4)))
        self.assertEqual(result['mae_b'], Interval(F(1, 4), F(1, 4)))
        self.assertEqual(result['a_minus_b'], Interval(F(0), F(0)))
        # RGB channels have different exact errors; all channels normalized by255.
        result = fixed_mask([(0, (0,)*3, (0,)*3, 1)], b'\1', bytes([255, 128, 0]), bytes(3))
        self.assertEqual(result['mae_a'].lower, F(383, 765))

    def test_equal_count_duplicate_and_omitted_pixels(self):
        rows = [(0, (0,)*3, (0,)*3, 1), (0, (0,)*3, (0,)*3, 1)]
        # Two rows for a two-pixel frame; count alone would conceal missing ID1.
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            fixed_mask(rows, b'\1\1', bytes(6), bytes(6))

    def test_tight_shared_difference_and_antisymmetry(self):
        for lo, hi in [(F(0), F(1)), (F(1, 4), F(3, 4)), (F(1, 3), F(2, 3))]:
            ab = scalar_losses(lo, hi, F(1, 4), F(3, 4))[2]
            ba = scalar_losses(lo, hi, F(3, 4), F(1, 4))[2]
            self.assertEqual(ab, Interval(-ba.upper, -ba.lower))
        self.assertEqual(scalar_losses(F(0), F(1), F(1, 4), F(3, 4))[2], Interval(F(-1, 2), F(1, 2)))
        # Both candidates below R: shared difference is constant despite wide R.
        self.assertEqual(scalar_losses(F(1, 2), F(1), F(0), F(1, 4))[2], Interval(F(1, 4), F(1, 4)))

    def test_heterogeneous_cartesian_worlds(self):
        # Two included RGB pixels, channel-varying uncertainty. Exhaust all 64
        # endpoint worlds where candidates are outside each scalar interval.
        import itertools
        lo, hi = (1, 2, 3), (2, 3, 4)
        rows = [(0, lo, hi, 8), (1, hi, (3, 4, 5), 8)]
        a, b = bytes(6), bytes([255]*6)
        actual = fixed_mask(rows, b'\1\1', a, b)
        worlds = []
        for bits in itertools.product((0, 1), repeat=6):
            ref = [F((rows[i//3][2] if bit else rows[i//3][1])[i % 3], 8) for i, bit in enumerate(bits)]
            worlds.append((sum(ref)/6, sum(1-r for r in ref)/6, sum(2*r-1 for r in ref)/6))
        for c, key in enumerate(('mae_a', 'mae_b', 'a_minus_b')):
            self.assertEqual(actual[key], Interval(min(w[c] for w in worlds), max(w[c] for w in worlds)))

    def test_threshold_crossing_and_equality(self):
        uncertain = scalar_losses(F(0), F(1), F(0), F(1))[2]
        self.assertEqual(at_least(uncertain, F(0)), 'inconclusive')
        self.assertEqual(at_least(Interval(F(0), F(0)), F(0)), 'proved')
        self.assertEqual(at_least(Interval(F(-1), F(-1, 1000000)), F(0)), 'disproved')
        self.assertEqual(at_least(Interval(F(-1, 10**30), F(1, 10**30)), F(0)), 'inconclusive')

    def test_quantization_ties_independently(self):
        # Independent nearest-code oracle: choose upper code at equal distance.
        for maximum in (31, 63):
            for code in range(maximum):
                tie = F(2*code+1, 2*maximum)
                for x in (tie-F(1, 10**12), tie, tie+F(1, 10**12)):
                    expected = min(range(maximum+1), key=lambda k: (abs(F(k, maximum)-x), -k))
                    self.assertEqual(round_codes(x, x, maximum), (expected, expected))
                self.assertEqual(round_codes(tie-F(1, 10**12), tie, maximum), (code, code+1))
            self.assertEqual(round_codes(F(0), F(1), maximum), (0, maximum))

    def test_invalid_inputs_and_missing_pixels(self):
        row = (0, (0,)*3, (1,)*3, 1)
        invalid = [([], b'\1', bytes(3), bytes(3)), ([row, row], b'\1', bytes(3), bytes(3)),
                   ([row], b'\0', bytes(3), bytes(3)), ([row], b'\2', bytes(3), bytes(3)),
                   ([row], b'\1', bytes(2), bytes(3)),
                   ([(0, (0,)*3, (1,)*3, 0)], b'\1', bytes(3), bytes(3)),
                   ([(0, (1,)*3, (0,)*3, 1)], b'\1', bytes(3), bytes(3)),
                   ([(0, (False,)*3, (1,)*3, 1)], b'\1', bytes(3), bytes(3))]
        for args in invalid:
            with self.subTest(args=args), self.assertRaises(ValueError):
                fixed_mask(*args)


class AdmissionTests(unittest.TestCase):
    def test_synthetic_certificate_to_metric_bridge(self):
        pp = validate_tile(tile(), 0)
        rows = [(p['pixel'][1]*8+p['pixel'][0], tuple(p['lower_numerator']),
                 tuple(p['upper_numerator']), p['denominator']) for p in pp]
        result = fixed_mask(rows, bytes([1]*64), bytes([255]*192), bytes(192))
        self.assertEqual(result['mae_a'], Interval(F(1), F(1)))
        self.assertEqual(result['mae_b'], Interval(F(0), F(0)))
        self.assertEqual(result['a_minus_b'], Interval(F(1), F(1)))

    def test_malformed_certificate_mutations(self):
        mutations = [lambda r: r.update(complete=False), lambda r: r.update(tile_index=True),
                     lambda r: r.update(tile=[1, 0]), lambda r: r['pixels'].pop(),
                     lambda r: r['pixels'].__setitem__(1, copy.deepcopy(r['pixels'][0])),
                     lambda r: r['pixels'][0].update(denominator=0),
                     lambda r: r['pixels'][0].update(denominator=-1),
                     lambda r: r['pixels'][0].update(denominator=True),
                     lambda r: r['pixels'][0].update(lower_numerator=[True, 0, 0]),
                     lambda r: r['pixels'][0].update(lower_numerator=[1, 0, 0]),
                     lambda r: r['pixels'][0].update(upper_numerator=[DEN+1, 0, 0]),
                     lambda r: r['pixels'][0].update(state='unknown'),
                     lambda r: r['pixels'][0].update(precision_reached=False),
                     lambda r: r['pixels'][0].update(deepest=17),
                     lambda r: r['pixels'][0].update(quantized_codes=[[0, 1], [0, 0], [0, 0]]),
                     lambda r: r['pixels'][0].update(cell_visits=100001),
                     lambda r: r.update(target_den=10240),
                     lambda r: r['pixels'][0].update(area_denominator=0)]
        for mutate in mutations:
            r = tile()
            mutate(r)
            with self.subTest(mutation=mutate), self.assertRaises(ValueError):
                validate_tile(r, 0)

    def test_noncanonical_positive_denominators(self):
        for den in (1, DEN+1, 2*DEN):
            record = tile()
            record['pixels'][0]['denominator'] = den
            with self.subTest(den=den), self.assertRaisesRegex(ValueError, 'canonical denominator'):
                validate_tile(record, 0)

    def test_interior_quantizer_boundaries_in_admission(self):
        # On either side of the FIRST interior rounding boundary. Independent
        # nearest-code enumeration chooses the larger code at equal distance.
        for channel, maximum in enumerate((31, 63, 31)):
            below = DEN//(2*maximum)
            for value in (below, below+1):
                record = tile()
                pixel = record['pixels'][0]
                pixel['lower_numerator'][channel] = value
                pixel['upper_numerator'][channel] = value
                expected = min(range(maximum+1), key=lambda k: (abs(F(k, maximum)-F(value, DEN)), -k))
                pixel['quantized_codes'][channel] = [expected, expected]
                self.assertEqual(len(validate_tile(record, 0)), 64)
                pixel['quantized_codes'][channel] = [1-expected, 1-expected]
                with self.assertRaisesRegex(ValueError, 'half-up code range'):
                    validate_tile(record, 0)

    def test_precision_equality_and_one_integer_over(self):
        width = DEN//1280
        self.assertEqual(F(width, DEN), F(1, 1280))
        for w, precise in ((width, True), (width+1, False)):
            record = tile()
            pixel = record['pixels'][0]
            pixel.update(upper_numerator=[w]*3, precision_reached=precise,
                         state='precision_reached' if precise else 'cell_budget')
            record['target_attained'] = precise
            self.assertEqual(len(validate_tile(record, 0)), 64)
            pixel['precision_reached'] = not precise
            with self.assertRaisesRegex(ValueError, 'precision status'):
                validate_tile(record, 0)

    def test_valid_wide_is_not_invalid(self):
        r = tile()
        p = r['pixels'][0]
        p.update(upper_numerator=[DEN]*3, precision_reached=False, state='cell_budget',
                 quantized_codes=[[0, 31], [0, 63], [0, 31]])
        r['target_attained'] = False
        self.assertEqual(len(validate_tile(r, 0)), 64)

    def test_strict_integer_and_duplicate_key_encoding(self):
        for text in ('{"x":1,"x":2}', '{"x":-0}', '{"x":999999999999999999999}', '{"x":NaN}', '{"x":Infinity}', '{"x":1e999}'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                strict_json(text)
        r = tile()
        r['pixels'][0]['denominator'] = float(DEN)
        with self.assertRaises(ValueError):
            validate_tile(strict_json(json.dumps(r)), 0)

    def test_independent_geometry_envelope_and_order(self):
        def feature(identity, x=0.):
            return struct.pack('<IHIBI4d', identity, 65535, 1, 0, 2, x, 0., 1., 0.)
        raw = b'PEAAAB1\0'+struct.pack('<I', 2)+feature(0)+feature(1)
        self.assertEqual(geometry_identity(raw), digest(feature(0)+feature(1)))
        self.assertNotEqual(geometry_identity(raw), geometry_identity(raw[:12]+feature(1)+feature(0)))
        for bad in (raw[:-1], raw+b'\0', b'wrong', b'PEAAAB1\0'+struct.pack('<I', 1)+feature(0, float('nan')),
                    b'PEAAAB1\0'+struct.pack('<I', 1)+feature(0, 2.**-129)):
            with self.assertRaises(ValueError):
                geometry_identity(bad)

    def test_read_guard_denies_candidates_and_writes(self):
        import subprocess
        import sys
        # The child installs the same irrevocable audit hook as the admission CLI.
        # No candidate is opened: even a nonexistent candidate path must be denied.
        source = """
from admit import install_read_guard
install_read_guard(set(), set())
for name, mode in [('/tmp/synthetic-A.rgb565', 'rb'), ('/tmp/synthetic-mask', 'wb')]:
    try:
        open(name, mode)
    except PermissionError:
        pass
    else:
        raise RuntimeError('guard failed')
"""
        result = subprocess.run([sys.executable, '-B', '-c', source],
                                cwd=Path(__file__).resolve().parent, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr.decode())

    def test_hash_and_resource_bounds_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp)/'synthetic'
            p.write_bytes(b'abc')
            a = CoarseAdapter(Paths(*(Path(temp),)*4))
            self.assertEqual(a.checked(p, digest(b'abc'), 3), b'abc')
            with self.assertRaises(ValueError):
                a.checked(p, digest(b'abd'), 3)
            with self.assertRaises(ValueError):
                a.checked(p, digest(b'abc'), 2)
            symlink = Path(temp)/'link'
            symlink.symlink_to(p)
            with self.assertRaises(ValueError):
                a.checked(symlink, digest(b'abc'), 3)


if __name__ == '__main__':
    unittest.main(verbosity=2)
