# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Read-only, pinned legacy coarse certificate adapter. No integrator imports.

Admission verifies encoding and evidence identity; enclosure soundness remains
conditional on the pinned reviewed producer and its documented assumptions.
"""
import gzip
import hashlib
import io
import json
import math
import re
import struct
from dataclasses import dataclass
from fractions import Fraction as F
from pathlib import Path

VERSION = 'PE-AA-01-coarse-certificate-adapter-v1'
DEN = 255 * 4**16
PIXELS = 640 * 480
MAX_JSON = 256 * 1024
BASE = Path(__file__).resolve().parents[2]


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def bounded_read(path, limit):
    path = Path(path).absolute()
    require(path.resolve() == path, 'symlink evidence paths unsupported')
    with path.open('rb') as stream:
        data = stream.read(limit+1)
    require(len(data) <= limit, 'file size bound')
    return data


def digest(data):
    return hashlib.sha256(data).hexdigest()


def strict_json(raw):
    def integer(text):
        require(re.fullmatch(r'0|-?[1-9][0-9]{0,19}', text) is not None, 'canonical bounded integer')
        return int(text)
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, 'duplicate JSON key')
            result[key] = value
        return result
    def floating(text):
        value = float(text)
        require(math.isfinite(value), 'nonfinite JSON float')
        return value
    def invalid(_):
        raise ValueError('nonfinite JSON constant')
    try:
        return json.loads(raw, parse_int=integer, parse_float=floating, object_pairs_hook=pairs, parse_constant=invalid)
    except (RecursionError, UnicodeError) as error:
        raise ValueError('invalid bounded JSON') from error


def uint(value, high, name):
    require(type(value) is int and 0 <= value <= high, name)
    return value


def vector(value, count, high, name):
    require(type(value) is list and len(value) == count, name)
    for v in value:
        uint(v, high, name)


def geometry_identity(raw):
    """Independent envelope/domain check; ordered raw feature bytes bind source order."""
    require(raw[:8] == b'PEAAAB1\0', 'geometry magic')
    pos = 8
    def take(fmt):
        nonlocal pos
        size = struct.calcsize(fmt)
        require(pos+size <= len(raw), 'truncated geometry')
        values = struct.unpack_from(fmt, raw, pos)
        pos += size
        return values
    count, = take('<I')
    require(count <= 8192, 'feature bound')
    chains = vertices = 0
    ordered = bytearray()
    for _ in range(count):
        start = pos
        _, _, n = take('<IHI')
        chains += n
        require(chains <= 32768, 'chain bound')
        for _ in range(n):
            closed, points = take('<BI')
            vertices += points
            require(closed <= 1 and points >= 2 and vertices <= 262144, 'point/closure bound')
            for _ in range(points):
                for v in take('<dd'):
                    require(math.isfinite(v) and abs(v) <= 10**6, 'finite coordinate domain')
                    require(v.as_integer_ratio()[1] <= 2**128, 'dyadic domain')
        ordered.extend(raw[start:pos])
    require(pos == len(raw), 'trailing geometry')
    return digest(ordered)


def validate_tile(record, index):
    require(type(record) is dict, 'tile object')
    uint(record.get('tile_index'), 4799, 'tile index')
    vector(record.get('tile'), 2, 79, 'tile coordinates')
    require(record['tile_index'] == index and record['tile'] == [index % 80, index // 80], 'tile identity')
    require(record.get('complete') is True, 'incomplete tile is unknown')
    require(type(record.get('target_den')) is int and record['target_den'] == 2560, 'operational radius')
    require(type(record.get('target_attained')) is bool, 'precision flag')
    require(record.get('max_depth_limit') == 16 and type(record['max_depth_limit']) is int, 'depth limit')
    require(record.get('fair_pixel_cell_quota') == 100000, 'cell quota')
    require(type(record.get('coordinate_scale_exponent')) is int and 16 <= record['coordinate_scale_exponent'] <= 128, 'dyadic scale')
    pp = record.get('pixels')
    require(type(pp) is list and len(pp) == 64, 'tile coverage')
    all_precise = True
    for k, p in enumerate(pp):
        require(type(p) is dict, 'pixel object')
        vector(p.get('pixel'), 2, 639, 'pixel coordinates')
        require(p['pixel'] == [index % 80 * 8 + k % 8, index // 80 * 8 + k // 8], 'pixel identity')
        require(type(p.get('denominator')) is int and p['denominator'] == DEN, 'positive canonical denominator')
        lo, hi = p.get('lower_numerator'), p.get('upper_numerator')
        vector(lo, 3, DEN, 'lower RGB integer')
        vector(hi, 3, DEN, 'upper RGB integer')
        require(all(a <= b for a, b in zip(lo, hi)), 'reversed bound')
        precise = max(b-a for a, b in zip(lo, hi))*2560 <= 2*DEN
        require(type(p.get('precision_reached')) is bool and p['precision_reached'] == precise, 'precision status')
        require(p.get('state') in ('precision_reached', 'cell_budget', 'time_budget', 'depth_budget'), 'unknown/failure state')
        require((p['state'] == 'precision_reached') == precise, 'stop state')
        uint(p.get('cell_visits'), 100000, 'cell visits')
        require(p['cell_visits'] >= 1 and type(p.get('pixel_cell_quota')) is int and p['pixel_cell_quota'] == 100000, 'pixel quota')
        uint(p.get('deepest'), 16, 'depth')
        require(type(p.get('area_denominator')) is int and p['area_denominator'] == 4**16, 'area denominator')
        uint(p.get('unresolved_area_numerator'), 4**16, 'unknown area')
        codes = p.get('quantized_codes')
        require(type(codes) is list and len(codes) == 3, 'code channels')
        for pair, a, b, m in zip(codes, lo, hi, (31, 63, 31)):
            vector(pair, 2, m, 'code range')
            require(pair == [(2*m*a+DEN)//(2*DEN), (2*m*b+DEN)//(2*DEN)], 'half-up code range')
        all_precise &= precise
    require(record['target_attained'] == all_precise, 'tile precision status')
    return pp


@dataclass(frozen=True)
class Paths:
    fixtures: Path
    prefix: Path
    tail: Path
    audit: Path


class CoarseAdapter:
    """Streaming adapter restricted to one frozen fixture. No candidate paths accepted."""
    def __init__(self, paths):
        self.paths = paths
        self.pins = strict_json(bounded_read(Path(__file__).with_name('provenance.json'), 128*1024))
        require(self.pins['adapter'] == VERSION, 'adapter version')

    def allowed_reads(self):
        p = self.paths
        files = [Path(__file__).with_name('provenance.json')]
        files += [BASE / name for name in self.pins['sources']]
        files += [getattr(p, role) / name for role, entries in self.pins['evidence'].items() for name in entries]
        files += [p.fixtures / name for name in self.pins['inputs']]
        files += [p.audit / 'view5-r220-relief-mask.u8']
        files += [(p.prefix if i < 4138 else p.tail) / 'tiles' / f'{i:04d}.json.gz' for i in range(4800)]
        return {str(f.absolute()) for f in files}

    def checked(self, path, expected, cap):
        raw = bounded_read(path, cap)
        require(digest(raw) == expected, 'provenance/hash mismatch: '+str(path))
        return raw

    def admit(self):
        """Return admission/mask reconciliation only. No geographic quality metrics."""
        for name, sha in self.pins['sources'].items():
            self.checked(BASE / name, sha, 1024*1024)
        for role, entries in self.pins['evidence'].items():
            for name, sha in entries.items():
                self.checked(getattr(self.paths, role) / name, sha, 4*1024*1024)
        inputs = {name: self.checked(self.paths.fixtures / name, sha, 16*1024*1024)
                  for name, sha in self.pins['inputs'].items()}
        geo = inputs['view5-r220-relief.geometry']
        require(geometry_identity(geo) == self.pins['source_order_sha256'], 'source-order hash')
        require(len(inputs['view5-r220-relief-background.rgb565']) == PIXELS*2, 'background dimensions')
        mask = self.checked(self.paths.audit / 'view5-r220-relief-mask.u8', self.pins['mask']['sha256'], PIXELS)
        require(len(mask) == PIXELS and all(v in (0, 1) for v in mask) and sum(mask) == 74796, 'frozen mask')
        audit = strict_json(self.checked(self.paths.audit / 'masked-audit.json',
                                         self.pins['evidence']['audit']['masked-audit.json'], 256*1024))
        require(audit['runtime'] == self.pins['mask']['runtime'] and audit['mask_source_sha256'] == self.pins['mask']['source_sha256'], 'mask provenance')
        manifest = strict_json(self.checked(self.paths.tail / 'combined-manifest.json',
                                            self.pins['evidence']['tail']['combined-manifest.json'], 4*1024*1024))
        require(type(manifest) is list and len(manifest) == 4800, 'manifest coverage')
        masked_width = full_width = precise_count = ambiguous = 0
        seen = bytearray(PIXELS)
        for i, row in enumerate(manifest):
            require(type(row) is dict and type(row.get('index')) is int and row['index'] == i, 'manifest tile order/uniqueness')
            require(type(row.get('sha256')) is str and re.fullmatch('[0-9a-f]{64}', row['sha256']), 'payload hash')
            root = self.paths.prefix if i < 4138 else self.paths.tail
            raw = self.checked(root / 'tiles' / f'{i:04d}.json.gz', row['sha256'], MAX_JSON)
            # Bounded decompression, not gzip.decompress on untrusted expansion.
            with gzip.GzipFile(fileobj=io.BytesIO(raw)) as source:
                decoded = source.read(MAX_JSON+1)
            require(len(decoded) <= MAX_JSON, 'expanded tile size bound')
            pp = validate_tile(strict_json(decoded), i)
            for pixel in pp:
                x, y = pixel['pixel']
                index = y*640+x
                require(not seen[index], 'duplicate frame pixel')
                seen[index] = 1
                width = sum(b-a for a, b in zip(pixel['lower_numerator'], pixel['upper_numerator']))
                full_width += width
                masked_width += width*mask[index]
                precise_count += pixel['precision_reached']
                ambiguous += any(a != b for a, b in pixel['quantized_codes'])
        require(all(seen), 'partial certificate; missing pixels unknown')
        return {'adapter': VERSION, 'protocol': self.pins['protocol'],
                'fixture': self.pins['fixture'], 'geometry_id': self.pins['geometry_id'],
                'style': 'relief', 'dimensions': [640, 480], 'tiles': 4800,
                'pixels': PIXELS, 'structural_admission': 'passed',
                'geometric_enclosure_reproved': False, 'precise_at_1_over_2560': precise_count == PIXELS,
                'precise_pixels': precise_count, 'decision_resolved': 'not evaluated',
                'validity_scope': 'conditional on pinned reviewed producer and numerical assumptions',
                'mask_sha256': digest(mask), 'mask_count': sum(mask),
                'masked_midpoint_radius_bound': str(F(masked_width, 6*sum(mask)*DEN)),
                'full_midpoint_radius_bound': str(F(full_width, 6*PIXELS*DEN)),
                'outside_mask_width_numerator': full_width-masked_width,
                'quantization_ambiguous_pixels': ambiguous,
                'candidate_inputs_read': 0, 'geographic_quality_scores_computed': 0}

    def mask(self):
        """Pinned authoritative bytes for a future in-memory fixed_mask consumer."""
        return self.checked(self.paths.audit / 'view5-r220-relief-mask.u8',
                            self.pins['mask']['sha256'], PIXELS)

    def bounds(self):
        """Return a bounded stream with an explicit exhaustion receipt.

        A future consumer must call require_complete() after consuming the stream.
        A partial, closed or failed stream cannot issue a receipt.
        """
        return BoundsStream(self._bounds(), self.pins['evidence']['tail']['combined-manifest.json'])

    def _bounds(self):
        """Future in-memory consumer bridge; full admission precedes first yield.

        Not invoked by admit.py. Recheck every payload on the second streaming
        pass so a changed file cannot inherit earlier admission. No score may be
        used unless the consumer exhausts this iterator successfully.
        """
        self.admit()
        manifest_raw = self.checked(self.paths.tail / 'combined-manifest.json',
                                    self.pins['evidence']['tail']['combined-manifest.json'], 4*1024*1024)
        manifest = strict_json(manifest_raw)
        for i, row in enumerate(manifest):
            root = self.paths.prefix if i < 4138 else self.paths.tail
            raw = self.checked(root / 'tiles' / f'{i:04d}.json.gz', row['sha256'], MAX_JSON)
            with gzip.GzipFile(fileobj=io.BytesIO(raw)) as source:
                decoded = source.read(MAX_JSON+1)
            require(len(decoded) <= MAX_JSON, 'expanded tile size bound')
            for p in validate_tile(strict_json(decoded), i):
                x, y = p['pixel']
                yield y*640+x, tuple(p['lower_numerator']), tuple(p['upper_numerator']), p['denominator']


@dataclass(frozen=True)
class CompletionReceipt:
    adapter: str
    manifest_sha256: str
    pixels: int


class BoundsStream:
    """Single-pass rows; constant-size completion state, no frame buffering.

    Completion is recorded only after the underlying verified generator reaches
    StopIteration with exactly PIXELS rows. Even the last yielded row is not a
    receipt: the consumer must request exhaustion, as a normal for-loop does.
    This API contract is not a security boundary against a hostile Python caller.
    """
    def __init__(self, rows, manifest_sha256):
        self._rows = iter(rows)
        self._manifest_sha256 = manifest_sha256
        self._count = 0
        self._failed = False
        self._receipt = None

    def __iter__(self):
        return self

    def __next__(self):
        require(not self._failed, 'stream failed or closed')
        if self._receipt is not None:
            raise StopIteration
        try:
            row = next(self._rows)
            self._count += 1
            require(self._count <= PIXELS, 'excess stream rows')
            return row
        except StopIteration:
            if self._count != PIXELS:
                self._failed = True
                raise ValueError('incomplete stream; no completion receipt')
            self._receipt = CompletionReceipt(VERSION, self._manifest_sha256, self._count)
            raise
        except BaseException:
            self._failed = True
            raise

    def close(self):
        if self._receipt is None:
            self._failed = True
        close = getattr(self._rows, 'close', None)
        if close is not None:
            close()

    def require_complete(self):
        require(self._receipt is not None, 'verified full-frame completion not established')
        return self._receipt
