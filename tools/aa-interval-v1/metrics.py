# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Exact fixed-mask loss bounds. Memory inputs only; no candidate file reader/CLI.

Extrema are exact for the Cartesian product of scalar reference intervals. They
are conservative for a geometry whose channels/pixels may be correlated.
"""
from dataclasses import dataclass
from fractions import Fraction as F

VERSION = 'PE-AA-01-fixed-mask-interval-v1'
MAX_PIXELS = 640 * 480


@dataclass(frozen=True)
class Interval:
    lower: F
    upper: F

    def __post_init__(self):
        if type(self.lower) is not F or type(self.upper) is not F or self.lower > self.upper:
            raise ValueError('ordered exact Fraction endpoints required')

    def json(self):
        return {'lower': str(self.lower), 'upper': str(self.upper)}


def scalar_losses(lower, upper, a, b):
    """Return MAE A, MAE B, and shared-reference |A-R|-|B-R| bounds."""
    if any(type(v) is not F or not 0 <= v <= 1 for v in (lower, upper, a, b)) or lower > upper:
        raise ValueError('normalized rational inputs required')
    # Piecewise linear losses: endpoints and all interior candidate kinks suffice.
    points = {lower, upper}
    points.update(v for v in (a, b) if lower <= v <= upper)
    ea = [abs(a-r) for r in points]
    eb = [abs(b-r) for r in points]
    delta = [abs(a-r)-abs(b-r) for r in points]
    return tuple(Interval(min(v), max(v)) for v in (ea, eb, delta))


def fixed_mask(rows, mask, candidate_a, candidate_b):
    """Rows: (row-major pixel index, RGB lower ints, upper ints, denominator).

    Mask is immutable uint8 0/1; candidates are expanded encoded RGB uint8 bytes,
    one RGB triple per pixel, normalized exactly by 255. Every frame pixel must
    appear exactly once even if excluded. No empty-mask zero score convention.
    """
    if type(mask) is not bytes or not 0 < len(mask) <= MAX_PIXELS or any(v > 1 for v in mask):
        raise ValueError('bounded literal 0/1 mask required')
    n = sum(mask)
    if not n:
        raise ValueError('empty mask is undefined')
    if any(type(c) is not bytes or len(c) != 3*len(mask) for c in (candidate_a, candidate_b)):
        raise ValueError('expanded RGB byte length')
    seen = bytearray(len(mask))
    totals = [[F(0), F(0)] for _ in range(3)]
    count = 0
    for index, lo, hi, den in rows:
        if type(index) is not int or not 0 <= index < len(mask) or seen[index]:
            raise ValueError('duplicate/out-of-frame pixel')
        seen[index] = 1
        count += 1
        if type(den) is not int or not 1 <= den <= 2**63-1:
            raise ValueError('bounded positive denominator')
        if len(lo) != 3 or len(hi) != 3 or any(type(v) is not int for v in (*lo, *hi)):
            raise ValueError('RGB integer bounds')
        if not all(0 <= x <= y <= den for x, y in zip(lo, hi)):
            raise ValueError('ordered normalized bounds')
        if not mask[index]:
            continue
        for channel in range(3):
            bounds = scalar_losses(F(lo[channel], den), F(hi[channel], den),
                                   F(candidate_a[3*index+channel], 255),
                                   F(candidate_b[3*index+channel], 255))
            for total, bound in zip(totals, bounds):
                total[0] += bound.lower
                total[1] += bound.upper
    if count != len(mask):
        raise ValueError('incomplete frame; missing pixels are unknown')
    return dict(zip(('mae_a', 'mae_b', 'a_minus_b'),
                    (Interval(a/(3*n), b/(3*n)) for a, b in totals)))


def at_least(value, threshold):
    """Generic synthetic predicate; equality passes, no epsilon/midpoint verdict."""
    if type(threshold) is not F:
        raise ValueError('exact threshold required')
    if value.lower >= threshold:
        return 'proved'
    if value.upper < threshold:
        return 'disproved'
    return 'inconclusive'


def round_codes(lower, upper, maximum):
    """Exact half-up Q code range, including boundary ties; no quantized loss yet."""
    if type(maximum) is not int or maximum not in (31, 63):
        raise ValueError('RGB565 channel maximum')
    if any(type(v) is not F or not 0 <= v <= 1 for v in (lower, upper)) or lower > upper:
        raise ValueError('normalized rational bounds')
    def q(x):
        y = maximum*x + F(1, 2)
        return y.numerator // y.denominator
    return q(lower), q(upper)
