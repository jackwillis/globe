# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Exact coupled reference / once-quantized RGB565 quality-decision bounds.

Memory inputs only: no candidate file reader, no CLI, no geographic scores.
Every endpoint is a Fraction. Each scalar reference channel R lies in [L, U];
the once-quantized control Q(R) = expand(floor(M*R + 1/2))/255 is a function of
that SAME R, so every quantity involving Q is optimized over R, never over
independent R and Q. Extrema are exact for the Cartesian product of the scalar
channel intervals and conservative for geometry-induced cross-pixel/channel
dependence, which is relaxed exactly as in aa-interval-v1.
"""
from dataclasses import dataclass, replace
from fractions import Fraction as F

VERSION = 'PE-AA-01-coupled-decision-v1'
MAX_PIXELS = 640 * 480
CHANNEL_MAX = (31, 63, 31)
FLOOR = F(10, 1024)
TARGET = F(1, 5)
LOCAL_RATIO = F(1, 10)
LOCAL_FLOOR = F(2, 1024)
GEOMETRIES = 16
STYLES = ('map', 'relief')
PASS, FAIL, INCONCLUSIVE = 'PASS', 'FAIL', 'INCONCLUSIVE'
QUANTIZED = 'Q'


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


@dataclass(frozen=True)
class Range:
    """Infimum and supremum of a value set, and whether each is attained.

    The set may have gaps (Q jumps at ties); only its hull endpoints matter.
    """
    lower: F
    lower_attained: bool
    upper: F
    upper_attained: bool

    def __post_init__(self):
        require(type(self.lower) is F and type(self.upper) is F and self.lower <= self.upper,
                'ordered exact Fraction endpoints required')
        require(type(self.lower_attained) is bool and type(self.upper_attained) is bool,
                'attainment flags')
        require(self.lower < self.upper or (self.lower_attained and self.upper_attained),
                'a single value is attained')

    def __add__(self, other):
        return Range(self.lower + other.lower, self.lower_attained and other.lower_attained,
                     self.upper + other.upper, self.upper_attained and other.upper_attained)

    def scale(self, k):
        require(type(k) is F, 'exact scale')
        if k > 0:
            return Range(self.lower*k, self.lower_attained, self.upper*k, self.upper_attained)
        if k < 0:
            return Range(self.upper*k, self.upper_attained, self.lower*k, self.lower_attained)
        return Range(F(0), True, F(0), True)

    def json(self):
        return {'lower': str(self.lower), 'lower_attained': self.lower_attained,
                'upper': str(self.upper), 'upper_attained': self.upper_attained}


ZERO = Range(F(0), True, F(0), True)


def expand(code, maximum):
    """Frozen RGB565 bit replication of a 5-bit (31) or 6-bit (63) code."""
    require(type(code) is int and 0 <= code <= maximum, 'code range')
    if maximum == 31:
        return (code << 3) | (code >> 2)
    if maximum == 63:
        return (code << 2) | (code >> 4)
    raise ValueError('RGB565 channel maximum')


def quantize(value, maximum):
    """Exact half-up code: ties go to the upper code."""
    y = maximum*value + F(1, 2)
    return y.numerator // y.denominator


def cells(lower, upper, maximum):
    """Reference sub-intervals sharing one code: (code, lo, hi, hi_closed).

    Code q owns [(2q-1)/2M, (2q+1)/2M): closed below, OPEN above, so an exact tie
    belongs to the upper code. lo is always attained; hi only if hi_closed.
    """
    out = []
    for q in range(quantize(lower, maximum), quantize(upper, maximum) + 1):
        left, right = F(2*q-1, 2*maximum), F(2*q+1, 2*maximum)
        lo = max(lower, left)
        hi, closed = (upper, True) if upper < right else (right, False)
        if lo < hi or (lo == hi and closed):
            out.append((q, lo, hi, closed))
    return out


def scalar_range(lower, upper, terms, maximum=None):
    """Extrema of sum w*|t - R| over R in [lower, upper].

    A target t is a Fraction or QUANTIZED (Q(R), coupled to the same R).
    """
    require(all(type(v) is F for v in (lower, upper)) and 0 <= lower <= upper <= 1,
            'normalized rational interval')
    require(type(terms) is list and terms, 'terms')
    uses_q = any(type(t) is str for _, t in terms)
    for w, t in terms:
        require(type(w) is F and (t == QUANTIZED if type(t) is str else type(t) is F and 0 <= t <= 1),
                'term weight/target')
    if uses_q:
        require(maximum in (31, 63), 'RGB565 channel maximum')
        pieces = cells(lower, upper, maximum)
    else:
        pieces = [(None, lower, upper, True)]
    low = high = None
    for q, lo, hi, closed in pieces:
        targets = [(w, F(expand(q, maximum), 255) if type(t) is str else t) for w, t in terms]
        points = {lo, hi}
        points.update(t for _, t in targets if lo < t < hi)
        for p in points:
            value = sum(w*abs(t-p) for w, t in targets)
            attained = closed or p != hi
            if low is None or value < low[0]:
                low = [value, attained]
            elif value == low[0]:
                low[1] = low[1] or attained
            if high is None or value > high[0]:
                high = [value, attained]
            elif value == high[0]:
                high[1] = high[1] or attained
    return Range(low[0], low[1], high[0], high[1])


# Per-channel quantities. F1 = E_B - 11/10 E_A is the first branch of J.
QUANTITIES = {
    'E_A': lambda a, b: [(F(1), a)],
    'E_B': lambda a, b: [(F(1), b)],
    'E_Q': lambda a, b: [(F(1), QUANTIZED)],
    'N': lambda a, b: [(F(1), a), (F(-1), b)],
    'D': lambda a, b: [(F(1), a), (F(-1), QUANTIZED)],
    'H': lambda a, b: [(1-TARGET, a), (F(-1), b), (TARGET, QUANTIZED)],
    'F1': lambda a, b: [(-(1+LOCAL_RATIO), a), (F(1), b)],
}


@dataclass(frozen=True)
class Condition:
    """One geometry/style fixed-mask result: normalized MAE-type ranges."""
    style: str
    geometry_id: int
    mask_count: int
    ranges: dict


def condition(style, geometry_id, rows, mask, candidate_a, candidate_b):
    """Rows: (row-major pixel index, RGB lower ints, upper ints, denominator).

    Same row/mask/candidate contract as aa-interval-v1 fixed_mask: literal 0/1
    mask, expanded encoded RGB bytes normalized by 255, every frame pixel exactly
    once even when excluded, empty mask undefined. Quantities are divided by 3n.
    """
    require(style in STYLES, 'style')
    require(type(geometry_id) is int and 0 <= geometry_id < GEOMETRIES, 'geometry id')
    require(type(mask) is bytes and 0 < len(mask) <= MAX_PIXELS and all(v <= 1 for v in mask),
            'bounded literal 0/1 mask required')
    n = sum(mask)
    require(n > 0, 'empty mask is undefined')
    require(all(type(c) is bytes and len(c) == 3*len(mask) for c in (candidate_a, candidate_b)),
            'expanded RGB byte length')
    seen = bytearray(len(mask))
    total = {name: ZERO for name in QUANTITIES}
    count = 0
    for index, lo, hi, den in rows:
        require(type(index) is int and 0 <= index < len(mask) and not seen[index],
                'duplicate/out-of-frame pixel')
        seen[index] = 1
        count += 1
        require(type(den) is int and 1 <= den <= 2**63-1, 'bounded positive denominator')
        require(len(lo) == 3 and len(hi) == 3 and all(type(v) is int for v in (*lo, *hi)),
                'RGB integer bounds')
        require(all(0 <= x <= y <= den for x, y in zip(lo, hi)), 'ordered normalized bounds')
        if not mask[index]:
            continue
        for c in range(3):
            a, b = F(candidate_a[3*index+c], 255), F(candidate_b[3*index+c], 255)
            for name, terms in QUANTITIES.items():
                total[name] += scalar_range(F(lo[c], den), F(hi[c], den), terms(a, b), CHANNEL_MAX[c])
    require(count == len(mask), 'incomplete frame; missing pixels are unknown')
    scale = F(1, 3*n)
    return Condition(style, geometry_id, n, {k: v.scale(scale) for k, v in total.items()})


def local_guard(cond):
    """J = E_B - E_A - max(E_A/10, 2/1024) for one condition (A's primary mask).

    J = min(F1, F2 - 2/1024) with F1 = E_B - 11/10 E_A and F2 = E_B - E_A = -N, so BOTH
    branches of the original maximum stay. inf J = min of the branch infima is exact.
    sup J is exact when one branch provably governs (E_A on one side of 20/1024);
    otherwise it is the conservative min of the branch suprema, labelled relaxed.
    PASS: J.upper <= 0 (equality passes). FAIL: J.lower > 0. Else INCONCLUSIVE.
    """
    require(type(cond) is Condition, 'condition')
    e_a, n, f1 = cond.ranges['E_A'], cond.ranges['N'], cond.ranges['F1']
    kink = LOCAL_FLOOR / LOCAL_RATIO
    f2 = n.scale(F(-1))
    f2 = Range(f2.lower - LOCAL_FLOOR, f2.lower_attained, f2.upper - LOCAL_FLOOR, f2.upper_attained)
    if e_a.upper <= kink:
        branch, lower, upper, exact = 'floor', f2.lower, f2.upper, True
    elif e_a.lower >= kink:
        branch, lower, upper, exact = 'ratio', f1.lower, f1.upper, True
    else:
        branch, lower, upper, exact = 'mixed', min(f1.lower, f2.lower), min(f1.upper, f2.upper), False
    status, reason = ((PASS, 'J.upper<=0') if upper <= 0 else
                      (FAIL, 'J.lower>0') if lower > 0 else (INCONCLUSIVE, 'J straddles 0'))
    return {'style': cond.style, 'geometry_id': cond.geometry_id, 'status': status,
            'reason': reason, 'J': {'lower': lower, 'upper': upper}, 'branch': branch,
            'lower_exact': True, 'upper_exact': exact, 'E_A': e_a}


def denominator_state(d):
    """Strict floor: eligible only if D.lower > 10/1024; D.upper <= floor is limited."""
    if d.upper <= FLOOR:
        return 'FLOOR_LIMITED'
    return 'ELIGIBLE' if d.lower > FLOOR else 'CROSSING'


def style_decision(conditions):
    """Original point criterion for ONE style over 16 equally weighted geometry IDs.

    N, D, H are means of per-geometry coupled ranges (references are independent
    across geometries, so the Minkowski mean is exact; H = N - D/5 is computed per
    channel from one shared R, never as N-interval minus D-interval). G = N/D is
    the ratio of means, reported only as a conservative corner quotient with D>0.
    PASS: eligible D and H.lower >= 0. FAIL: eligible D and H.upper < 0.
    FLOOR_LIMITED or CROSSING denominators and straddling H are INCONCLUSIVE.
    """
    require(type(conditions) is list and len(conditions) == GEOMETRIES
            and all(type(c) is Condition for c in conditions), '16 conditions')
    require(len({c.style for c in conditions}) == 1, 'one style per criterion')
    require(sorted(c.geometry_id for c in conditions) == list(range(GEOMETRIES)),
            '16 distinct geometry ids')
    mean = {}
    for name in ('N', 'D', 'H'):
        total = ZERO
        for c in conditions:
            total += c.ranges[name]
        mean[name] = total.scale(F(1, GEOMETRIES))
    n, d, h = mean['N'], mean['D'], mean['H']
    state = denominator_state(d)
    gap = None
    if d.lower > 0:
        corners = [x/y for x in (n.lower, n.upper) for y in (d.lower, d.upper)]
        gap = {'lower': min(corners), 'upper': max(corners)}
    if state == 'FLOOR_LIMITED':
        status, reason = INCONCLUSIVE, 'D.upper<=floor: floor-limited, no percentage conclusion'
    elif state == 'CROSSING':
        status, reason = INCONCLUSIVE, 'D straddles the floor'
    elif h.lower >= 0:
        status, reason = PASS, 'D.lower>floor and H.lower>=0'
    elif h.upper < 0:
        status, reason = FAIL, 'D.lower>floor and H.upper<0'
    else:
        status, reason = INCONCLUSIVE, 'H straddles 0'
    return {'version': VERSION, 'style': conditions[0].style, 'geometries': GEOMETRIES,
            'N': n, 'D': d, 'H': h, 'G_conservative': gap, 'denominator': state,
            'status': status, 'reason': reason}


def numerical_conjunction(statuses):
    """Three-valued AND of required numerical criteria; not acceptance of B."""
    statuses = list(statuses)
    require(statuses and all(s in (PASS, FAIL, INCONCLUSIVE) for s in statuses), 'statuses')
    if FAIL in statuses:
        return FAIL
    return PASS if all(s == PASS for s in statuses) else INCONCLUSIVE


def decide(by_style):
    """Map and Relief remain separate; no pooling. Returns per-style decisions plus
    local guards for every supplied condition and the explicit conjunctions."""
    require(type(by_style) is dict and by_style and set(by_style) <= set(STYLES), 'styles')
    require(all(type(c) is list and all(type(x) is Condition and x.style == s for x in c)
                for s, c in by_style.items()), 'conditions must carry their own style')
    points = {s: style_decision(list(c)) for s, c in sorted(by_style.items())}
    guards = {s: [local_guard(x) for x in sorted(c, key=lambda x: x.geometry_id)]
              for s, c in sorted(by_style.items())}
    flat = [g['status'] for gs in guards.values() for g in gs]
    complete = (set(by_style) == set(STYLES) and len(flat) == len(STYLES) * GEOMETRIES
                and all(len(c) == GEOMETRIES for c in by_style.values()))

    def conjoin(statuses):
        # A partial set can fail the experiment but can never pass it.
        r = numerical_conjunction(statuses)
        return r if complete or r == FAIL else INCONCLUSIVE
    return {'points': points, 'guards': guards, 'complete': complete,
            'point_conjunction': conjoin(p['status'] for p in points.values()),
            'guard_conjunction': conjoin(flat)}


def with_geometry(cond, geometry_id):
    """Synthetic-control helper: reuse one computed frame as another geometry ID."""
    return replace(cond, geometry_id=geometry_id)
