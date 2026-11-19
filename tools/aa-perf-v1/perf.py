# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""PE-AA-01 numerical-instrument performance pilot: workloads and measurement.

Synthetic inputs only, standard library only, Python 3.7+ syntax. Imports the reviewed
aa-decision-v1 module unchanged and times it. Inputs come from an integer-only
SplitMix64 stream, so every host and Python version builds byte-identical workloads;
stable SHA-256 digests of the inputs and of the exact results prove it.
"""
import gc
import hashlib
import json
import os
import platform
import re
import subprocess
import sys
import time
from dataclasses import replace
from fractions import Fraction

sys.dont_write_bytecode = True  # never leave __pycache__ on the host under test
HERE = os.path.dirname(os.path.abspath(__file__))
DECISION_DIR = os.path.join(os.path.dirname(HERE), 'aa-decision-v1')
sys.path.insert(0, DECISION_DIR)
import decision  # noqa: E402  (reviewed module, executed unmodified)

try:
    import resource
except ImportError:  # not Unix
    resource = None
try:
    import signal
except ImportError:
    signal = None

VERSION = 'PE-AA-01-perf-v1'
DEN = 255 * 4 ** 16                  # canonical certificate denominator (aa-interval-v1)
UNIT = DEN // 255                    # one 8-bit step
R_NARROW = DEN // 2560               # 1/2560, the certified operating radius
R_WIDE = DEN // 64
FULL_FRAME, FULL_MASK = 640 * 480, 74796
STAGES = {'A': 100, 'B': 1000, 'C': 5000, 'D': 10000}
CLASSES = ('exact', 'narrow', 'wide', 'crossing', 'mixed')
MIXED_WEIGHTS = ('exact',) * 3 + ('narrow',) * 3 + ('wide',) * 2 + ('crossing',) * 2
MASK64 = (1 << 64) - 1
GEOMETRIES, STYLES = decision.GEOMETRIES, decision.STYLES


class CellLimit(Exception):
    """Raised inside a cell when its wall-clock or memory budget is exceeded."""


class SplitMix64:
    def __init__(self, seed):
        self.s = seed & MASK64

    def next(self):
        self.s = (self.s + 0x9E3779B97F4A7C15) & MASK64
        z = self.s
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & MASK64
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & MASK64
        return z ^ (z >> 31)

    def below(self, n):
        """Unbiased integer in [0, n)."""
        limit = (1 << 64) - ((1 << 64) % n)
        while True:
            v = self.next()
            if v < limit:
                return v % n


def derive_seed(cls, masked, frame, seed):
    text = '%s|%s|%d|%d|%d' % (VERSION, cls, masked, frame, seed)
    return int(hashlib.sha256(text.encode('ascii')).hexdigest()[:16], 16)


def auto_frame(masked):
    """Frame size keeping the certified mask density (74796 of 307200)."""
    return min(FULL_FRAME, max(masked, -(-masked * FULL_FRAME // FULL_MASK)))


def cell_key(cls, masked, frame, seed):
    return '%s|%d|%d|%d' % (cls, masked, frame, seed)


def _interval(rng, kind, ch):
    m = decision.CHANNEL_MAX[ch]
    if kind == 'exact':
        if rng.below(4) == 0:  # a quarter sit exactly on an expanded RGB565 level
            v = decision.expand(rng.below(m + 1), m) * UNIT
        else:
            v = rng.below(DEN + 1)
        return v, v
    if kind == 'crossing':     # strictly straddles one half-up quantizer tie
        q = rng.below(m)
        t = ((2 * q + 1) * DEN) // (2 * m)
        return t - 1 - rng.below(R_NARROW), t + 1 + rng.below(R_NARROW)
    radius = R_NARROW if kind == 'narrow' else R_WIDE
    c = rng.below(DEN + 1)
    return max(0, c - 1 - rng.below(radius)), min(DEN, c + 1 + rng.below(radius))


def _clamp(v):
    return 0 if v < 0 else 255 if v > 255 else v


def build_workload(cls, masked, frame, seed):
    """Deterministic rows, mask and candidates. Unmasked rows are shared exact zeros."""
    if cls not in CLASSES:
        raise ValueError('unknown class')
    if not (0 < masked and masked * 4 <= frame * 3 and frame <= decision.MAX_PIXELS):
        raise ValueError('need 0 < masked <= 3/4 frame <= 307200')
    rng = SplitMix64(derive_seed(cls, masked, frame, seed))
    mask = bytearray(frame)
    placed = 0
    while placed < masked:
        i = rng.below(frame)
        if not mask[i]:
            mask[i] = 1
            placed += 1
    a, b = bytearray(3 * frame), bytearray(3 * frame)
    idle = ((0, 0, 0), (0, 0, 0))
    rows, kinds = [], {k: 0 for k in CLASSES[:4]}
    uncertain = 0
    for i in range(frame):
        if not mask[i]:
            rows.append((i, idle[0], idle[1], DEN))
            continue
        kind = MIXED_WEIGHTS[rng.below(len(MIXED_WEIGHTS))] if cls == 'mixed' else cls
        kinds[kind] += 1
        lo, hi = [], []
        for ch in range(3):
            x, y = _interval(rng, kind, ch)
            lo.append(x)
            hi.append(y)
        uncertain += any(x < y for x, y in zip(lo, hi))
        for ch in range(3):
            base = _clamp((lo[ch] + hi[ch]) * 255 // (2 * DEN))
            va = _clamp(base + rng.below(17) - 8)
            vb = va if rng.below(10) == 0 else _clamp(va + rng.below(25) - 12)
            a[3 * i + ch], b[3 * i + ch] = va, vb
        rows.append((i, tuple(lo), tuple(hi), DEN))
    stats = {'frame_pixels': frame, 'masked': masked, 'uncertain_masked': uncertain,
             'exact_masked': masked - uncertain, 'kinds': kinds}
    return rows, bytes(mask), bytes(a), bytes(b), stats


def input_digest(cls, masked, frame, seed, rows, mask, a, b):
    h = hashlib.sha256()
    h.update(('%s %s %d %d %d\n' % (VERSION, cls, masked, frame, seed)).encode('ascii'))
    for blob in (mask, a, b):
        h.update(blob)
    for start in range(0, len(rows), 1024):
        h.update(''.join('%d %d %d %d %d %d %d %d\n' % ((i,) + lo + hi + (den,))
                         for i, lo, hi, den in rows[start:start + 1024]).encode('ascii'))
    return h.hexdigest()


def canon(obj):
    if isinstance(obj, decision.Range):
        return obj.json()
    if isinstance(obj, Fraction):
        return str(obj)
    if isinstance(obj, dict):
        return {str(k): canon(v) for k, v in sorted(obj.items(), key=lambda kv: str(kv[0]))}
    if isinstance(obj, (list, tuple)):
        return [canon(v) for v in obj]
    return obj


def result_digest(obj):
    text = json.dumps(canon(obj), sort_keys=True, separators=(',', ':'))
    return hashlib.sha256(text.encode('ascii')).hexdigest()


def condition_result(cond):
    return {'style': cond.style, 'mask_count': cond.mask_count, 'ranges': cond.ranges}


def aggregate(cond):
    """decide() over 16 geometry IDs x 2 styles reusing one evaluated condition.

    Times only the aggregation arithmetic; it is not 32 independent evaluations.
    """
    return decision.decide({s: [replace(cond, style=s, geometry_id=g) for g in range(GEOMETRIES)]
                            for s in STYLES})


# ---- operating-system instrumentation, every piece optional -------------------------

def peak_rss_kib():
    """Peak resident set in KiB, or None. Linux and NetBSD report ru_maxrss in KiB."""
    if resource is None:
        return None
    try:
        peak = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    except (OSError, ValueError, AttributeError):
        return None
    return peak // 1024 if sys.platform == 'darwin' else peak


def rusage_snapshot():
    if resource is None:
        return None
    try:
        r = resource.getrusage(resource.RUSAGE_SELF)
    except (OSError, ValueError, AttributeError):
        return None
    return {k: getattr(r, k) for k in ('ru_utime', 'ru_stime', 'ru_minflt', 'ru_majflt', 'ru_nswap',
                                       'ru_inblock', 'ru_oublock', 'ru_nvcsw', 'ru_nivcsw')
            if hasattr(r, k)}


def rusage_delta(before, after):
    if before is None or after is None:
        return None
    return {k: after[k] - before[k] for k in after if k in before}


def loadavg():
    try:
        return list(os.getloadavg())
    except (OSError, AttributeError):
        return None


def _run_text(argv):
    try:
        done = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, timeout=10)
        return done.stdout.decode('ascii', 'replace').strip() if done.returncode == 0 else None
    except (OSError, subprocess.SubprocessError, ValueError):
        return None


SWAP_KIB = {'kb': 1, 'blocks': 0.5}   # swapctl -s prints 512-byte blocks unless asked for KB


def parse_meminfo_swap(text):
    """Swap sizes in KiB from Linux /proc/meminfo text, or None."""
    fields = dict(line.split(':', 1) for line in text.splitlines() if ':' in line)
    try:
        total, free = (int(fields[k].split()[0]) for k in ('SwapTotal', 'SwapFree'))
    except (KeyError, ValueError, IndexError):
        return None
    return {'total_kib': total, 'used_kib': total - free}


def parse_swapctl_summary(text):
    """Swap sizes in KiB from `swapctl -s` text, or None if the format is not recognized.

    NetBSD: "total: 1048576 blocks allocated, 0 blocks used, 1048576 blocks available"
    (512-byte blocks). Also accepts the KB form. Odd block counts round up.
    """
    m = re.search(r'total:\s*(\d+)\s*(KB|blocks)\s+allocated,\s*(\d+)\s*(KB|blocks)\s+used', text or '', re.I)
    if not m:
        return None

    def kib(count, unit):
        return -(-int(count) * 1 // 2) if unit.lower() == 'blocks' else int(count)
    return {'total_kib': kib(m.group(1), m.group(2)), 'used_kib': kib(m.group(3), m.group(4))}


def swap_info():
    """Swap in use (KiB) where discoverable; used_kib is None when it cannot be read.

    Linux /proc/meminfo first, else `swapctl -s` (found on PATH or in /sbin, /usr/sbin).
    """
    try:
        with open('/proc/meminfo') as f:
            parsed = parse_meminfo_swap(f.read())
        if parsed:
            return dict(parsed, source='/proc/meminfo')
    except OSError:
        pass
    for exe in ('swapctl', '/sbin/swapctl', '/usr/sbin/swapctl'):
        text = _run_text([exe, '-s'])
        if text:
            parsed = parse_swapctl_summary(text) or {'total_kib': None, 'used_kib': None}
            return dict(parsed, source='swapctl -s', raw=text)
    return None


def physical_memory_kib():
    try:
        return os.sysconf('SC_PHYS_PAGES') * os.sysconf('SC_PAGE_SIZE') // 1024
    except (ValueError, OSError, AttributeError):
        return None


def cpu_model():
    try:
        with open('/proc/cpuinfo') as f:
            for line in f:
                if line.startswith('model name') or line.startswith('cpu\t'):
                    return line.split(':', 1)[1].strip()
    except OSError:
        pass
    return _run_text(['sysctl', '-n', 'hw.model'])


def clock_info():
    out = {}
    for name in ('perf_counter', 'process_time', 'monotonic'):
        try:
            i = time.get_clock_info(name)
            out[name] = {'implementation': i.implementation, 'resolution': i.resolution}
        except (ValueError, AttributeError):
            out[name] = None
    return out


def file_sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        h.update(f.read())
    return h.hexdigest()


def capture_env(label):
    u = platform.uname()
    return {'label': label, 'harness': VERSION, 'python': sys.version.replace('\n', ' '),
            'python_version': list(sys.version_info[:3]), 'implementation': sys.implementation.name,
            'byteorder': sys.byteorder, 'maxsize': sys.maxsize, 'int_info': str(sys.int_info),
            'system': u.system, 'release': u.release, 'machine': u.machine,
            'platform': platform.platform(), 'cpu_model': cpu_model(), 'cpu_count': os.cpu_count(),
            'physical_memory_kib': physical_memory_kib(), 'swap': swap_info(), 'loadavg': loadavg(),
            'gc_enabled': gc.isenabled(), 'clocks': clock_info(),
            'rss_unit_assumed_kib': sys.platform != 'darwin',
            'decision_sha256': file_sha256(os.path.join(DECISION_DIR, 'decision.py')),
            'perf_sha256': file_sha256(os.path.join(HERE, 'perf.py')),
            'decision_version': decision.VERSION}


class Guard:
    """1 Hz timer that aborts a cell over wall or resident-memory budget (Unix only)."""

    def __init__(self, wall_s, mem_kib):
        self.deadline = time.monotonic() + wall_s if wall_s else None
        self.mem_kib = mem_kib
        self.active = False

    def _tick(self, signum, frame):
        if self.deadline is not None and time.monotonic() > self.deadline:
            raise CellLimit('wall_limit')
        peak = peak_rss_kib()
        if self.mem_kib and peak is not None and peak > self.mem_kib:
            raise CellLimit('rss_limit')

    def __enter__(self):
        if signal is not None and hasattr(signal, 'setitimer') and hasattr(signal, 'SIGALRM'):
            signal.signal(signal.SIGALRM, self._tick)
            signal.setitimer(signal.ITIMER_REAL, 1.0, 1.0)
            self.active = True
        return self

    def __exit__(self, *exc):
        if self.active:
            signal.setitimer(signal.ITIMER_REAL, 0)
            self.active = False
        return False


class Stopwatch:
    def __enter__(self):
        gc.collect()
        self.w, self.c = time.perf_counter(), time.process_time()
        return self

    def __exit__(self, *exc):
        self.wall, self.cpu = time.perf_counter() - self.w, time.process_time() - self.c
        return False


def run_cell(cls, masked, frame, seed, reps, wall_s=None, mem_kib=None, expected=None):
    """Build one workload and evaluate it `reps` times. Always returns a record."""
    rec = {'type': 'cell', 'class': cls, 'masked': masked, 'frame': frame, 'seed': seed,
           'reps_requested': reps, 'status': 'ok', 'evals': [], 'aggregates': [],
           'loadavg_before': loadavg(), 'baseline_rss_kib': peak_rss_kib()}
    ru0 = rusage_snapshot()
    try:
        with Guard(wall_s, mem_kib):
            with Stopwatch() as build:
                rows, mask, a, b, stats = build_workload(cls, masked, frame, seed)
            rec['input'] = dict(stats, build_wall_s=build.wall, build_cpu_s=build.cpu)
            rec['rss_after_build_kib'] = peak_rss_kib()
            with Stopwatch() as dig:
                rec['input']['digest'] = input_digest(cls, masked, frame, seed, rows, mask, a, b)
            rec['input']['digest_wall_s'] = dig.wall
            for rep in range(reps):
                with Stopwatch() as ev:
                    cond = decision.condition('map', 0, rows, mask, a, b)
                with Stopwatch() as ag:
                    outcome = aggregate(cond)
                rec['evals'].append({'rep': rep, 'wall_s': ev.wall, 'cpu_s': ev.cpu,
                                     'digest': result_digest(condition_result(cond))})
                rec['aggregates'].append({'rep': rep, 'wall_s': ag.wall, 'cpu_s': ag.cpu,
                                          'digest': result_digest(outcome),
                                          'complete': outcome['complete'],
                                          'point_conjunction': outcome['point_conjunction'],
                                          'guard_conjunction': outcome['guard_conjunction']})
            rec['rss_after_eval_kib'] = peak_rss_kib()
    except CellLimit as e:
        rec['status'] = str(e)
    except MemoryError:
        rec['status'] = 'memory_error'
    except Exception as e:  # recorded, never swallowed silently
        rec['status'] = 'error'
        rec['error'] = '%s: %s' % (type(e).__name__, e)
    rec['peak_rss_kib'] = peak_rss_kib()
    rec['rusage_delta'] = rusage_delta(ru0, rusage_snapshot())
    rec['loadavg_after'] = loadavg()
    check_digests(rec, expected)
    return rec


def check_digests(rec, expected):
    """Compare against the pinned Linux digests when this cell has an entry."""
    rec['digest_check'] = 'unpinned'
    pin = (expected or {}).get('cells', {}).get(cell_key(rec['class'], rec['masked'], rec['frame'], rec['seed']))
    if pin is None or rec['status'] != 'ok':
        return
    seen = ({rec['input']['digest']} == {pin['input_digest']}
            and {e['digest'] for e in rec['evals']} == {pin['result_digest']}
            and {g['digest'] for g in rec['aggregates']} == {pin['aggregate_digest']})
    rec['digest_check'] = 'match' if seen else 'MISMATCH'
    if not seen:
        rec['status'] = 'digest_mismatch'
