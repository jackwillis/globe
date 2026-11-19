# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Synthetic virtual evidence: real admit/bounds/hash/parser paths, no geography.

Only bounded_read is replaced. Do not patch admission, tile validation, hashes,
manifest checks or the streaming loop. Generate tile bytes on demand; retain at
most two payloads and one 4800-entry manifest, never an archive/frame of bounds.
"""
import gzip
import json
import struct
import unittest
from collections import Counter
from functools import lru_cache
from pathlib import Path
from unittest.mock import patch
import adapter
from adapter import BoundsStream, CoarseAdapter, Paths, PIXELS, digest
from test_interval import tile


def encoded(value):
    return json.dumps(value, separators=(',', ':')).encode()


class SyntheticEvidence:
    def __init__(self):
        self.paths = Paths(*(Path('/synthetic')/s for s in ('fixtures', 'prefix', 'tail', 'audit')))
        feature = struct.pack('<IHIBI4d', 0, 65535, 1, 0, 2, 0., 0., 1., 0.)
        geometry = b'PEAAAB1\0'+struct.pack('<I', 1)+feature
        mask = bytes([1])*74796+bytes(PIXELS-74796)
        self.reads = Counter()
        self.corrupt = set()
        self.manifest = [{'index': i, 'sha256': digest(self.payload(i))} for i in range(4800)]
        runtime = {'synthetic': 'no floating mask construction'}
        audit = encoded({'runtime': runtime, 'mask_source_sha256': {'synthetic-mask': 'fixture'}})
        source = adapter.BASE/'synthetic-source'
        self.files = {source: b'synthetic reviewed source',
                      self.paths.fixtures/'view5-r220-relief.geometry': geometry,
                      self.paths.fixtures/'view5-r220-relief-background.rgb565': bytes(PIXELS*2),
                      self.paths.audit/'view5-r220-relief-mask.u8': mask,
                      self.paths.audit/'masked-audit.json': audit,
                      self.paths.tail/'combined-manifest.json': encoded(self.manifest)}
        self.pins = {'adapter': adapter.VERSION, 'protocol': 'synthetic-protocol',
                     'fixture': 'synthetic-frame', 'geometry_id': 0,
                     'sources': {'synthetic-source': digest(self.files[source])},
                     'evidence': {'tail': {'combined-manifest.json': digest(encoded(self.manifest))},
                                  'audit': {'masked-audit.json': digest(audit)}},
                     'inputs': {name: digest(self.files[self.paths.fixtures/name]) for name in
                                ('view5-r220-relief.geometry', 'view5-r220-relief-background.rgb565')},
                     'source_order_sha256': digest(feature),
                     'mask': {'sha256': digest(mask), 'runtime': runtime,
                              'source_sha256': {'synthetic-mask': 'fixture'}}}

    @staticmethod
    @lru_cache(maxsize=2)
    def payload(i):
        record = tile()
        record.update(tile_index=i, tile=[i % 80, i // 80])
        for k, pixel in enumerate(record['pixels']):
            pixel['pixel'] = [8*(i % 80)+k % 8, 8*(i // 80)+k // 8]
        return gzip.compress(encoded(record), mtime=0)

    def read(self, path, cap):
        path = Path(path)
        if path == Path(adapter.__file__).with_name('provenance.json'):
            raw = encoded(self.pins)
        elif path.parent.name == 'tiles':
            i = int(path.name[:4])
            # Independent expected storage split. An off-by-one routes to a
            # missing synthetic path rather than quietly finding a duplicate.
            expected = self.paths.prefix if i <= 4137 else self.paths.tail
            if path.parent.parent != expected:
                raise ValueError('wrong prefix/tail storage role')
            self.reads[(expected.name, i)] += 1
            raw = self.payload(i)
            if i in self.corrupt:
                changed = json.loads(gzip.decompress(raw))
                changed['pixels'][0]['lower_numerator'][0] = 1
                changed['pixels'][0]['upper_numerator'][0] = 1
                raw = gzip.compress(encoded(changed), mtime=0)
        else:
            raw = self.files[path]
        if len(raw) > cap:
            raise ValueError('synthetic file cap')
        return raw


class IntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.evidence = SyntheticEvidence()

    def setUp(self):
        self.evidence.corrupt.clear()
        self.evidence.reads.clear()
        self.reader = patch.object(adapter, 'bounded_read', self.evidence.read)
        self.reader.start()
        self.addCleanup(self.reader.stop)
        self.subject = CoarseAdapter(self.evidence.paths)

    def test_admit_success(self):
        result = self.subject.admit()
        self.assertEqual(result['structural_admission'], 'passed')
        self.assertFalse(result['geometric_enclosure_reproved'])
        self.assertEqual(result['pixels'], 307200)
        self.assertEqual(result['mask_count'], 74796)
        self.assertEqual(result['masked_midpoint_radius_bound'], '0')
        self.assertEqual(result['outside_mask_width_numerator'], 0)
        self.assertEqual(result['decision_resolved'], 'not evaluated')

    def test_admit_and_full_stream_completion_boundary(self):
        # bounds() executes complete admit(), then a second, verified pass.
        # Capture its actual admit result without replacing that implementation.
        with patch.object(self.subject, 'admit', wraps=self.subject.admit) as admission:
            stream = self.subject.bounds()
            with self.assertRaises(ValueError):
                stream.require_complete()
            count = 0
            for row in stream:
                if count == 0:
                    self.assertEqual(row[0], 0)
                    self.assertEqual(admission.call_count, 1)
                    with self.assertRaises(ValueError):
                        stream.require_complete()
                if count == 64*4138:
                    self.assertEqual(row[0], 261584)  # first pixel of tile(58,51)
                count += 1
            receipt = stream.require_complete()
            self.assertEqual(count, 307200)
            self.assertEqual(receipt.pixels, count)
            self.assertEqual(receipt.manifest_sha256, self.evidence.pins['evidence']['tail']['combined-manifest.json'])
            self.assertEqual(self.evidence.reads[('prefix', 4137)], 2)
            self.assertEqual(self.evidence.reads[('tail', 4138)], 2)
            self.assertEqual(sum(v for (role, _), v in self.evidence.reads.items() if role == 'prefix'), 2*4138)
            self.assertEqual(sum(v for (role, _), v in self.evidence.reads.items() if role == 'tail'), 2*662)
            self.assertTrue(all(v == 2 for v in self.evidence.reads.values()))
            stream.close()
            self.assertEqual(stream.require_complete(), receipt)

    def test_second_pass_mutation_and_partial_stream_have_no_receipt(self):
        stream = self.subject.bounds()
        self.assertEqual(next(stream)[0], 0)  # first-pass admission succeeded
        # Change the NEXT tile, after admission. Keep manifest and pins unchanged.
        self.evidence.corrupt.add(1)
        for _ in range(63):
            next(stream)
        with self.assertRaisesRegex(ValueError, 'provenance/hash mismatch'):
            next(stream)
        with self.assertRaises(ValueError):
            stream.require_complete()
        with self.assertRaises(ValueError):
            next(stream)
        stream.close()
        with self.assertRaises(ValueError):
            stream.require_complete()

    def test_malformed_and_incomplete_mask_even_if_hash_matches(self):
        path = self.evidence.paths.audit/'view5-r220-relief-mask.u8'
        original = self.evidence.files[path]
        for mask in (original[:-1], bytes([2])+original[1:], bytes(PIXELS)):
            with self.subTest(mask_length=len(mask)):
                # A matching new hash must not replace dimension/value/count checks.
                with patch.dict(self.evidence.files, {path: mask}):
                    self.subject.pins['mask']['sha256'] = digest(mask)
                    with self.assertRaisesRegex(ValueError, 'frozen mask'):
                        self.subject.admit()
        self.subject.pins['mask']['sha256'] = digest(original)

    def test_source_and_manifest_mutations_fail_admission(self):
        for path in (adapter.BASE/'synthetic-source', self.evidence.paths.tail/'combined-manifest.json'):
            with patch.dict(self.evidence.files, {path: b'changed'}):
                with self.assertRaisesRegex(ValueError, 'provenance/hash mismatch'):
                    self.subject.admit()


class CompletionTests(unittest.TestCase):
    def test_partial_closed_short_and_failed_streams(self):
        stream = BoundsStream(iter(range(PIXELS)), 'synthetic')
        for _ in range(100):
            next(stream)
        with self.assertRaises(ValueError):
            stream.require_complete()
        stream.close()
        with self.assertRaises(ValueError):
            stream.require_complete()
        with self.assertRaises(ValueError):
            next(stream)
        short = BoundsStream(iter(range(2)), 'synthetic')
        next(short)
        next(short)
        with self.assertRaisesRegex(ValueError, 'incomplete stream'):
            next(short)
        with self.assertRaises(ValueError):
            short.require_complete()

    def test_receipt_requires_verified_exhaustion_not_just_last_row(self):
        stream = BoundsStream(iter(range(PIXELS)), 'synthetic')
        for _ in range(PIXELS):
            next(stream)
        with self.assertRaises(ValueError):
            stream.require_complete()
        with self.assertRaises(StopIteration):
            next(stream)
        self.assertEqual(stream.require_complete().pixels, PIXELS)


if __name__ == '__main__':
    unittest.main(verbosity=2)
