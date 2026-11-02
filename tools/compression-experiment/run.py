#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Build-time experiment: how small can the atlas and elevation grids get? Prints markdown tables.

Needs numpy; the 0.5/0.25 degree grids additionally need h5py and the ETOPO source (otherwise
only the committed 1 degree pack is measured).  Sizes are payload bytes (GELV: cell indices only,
without the 552-byte header).  Deflate is raw (no zlib wrapper) and lzma is FORMAT_RAW, so
per-stream overhead is minimal and tiled totals are not penalised by container headers.

usage: run.py [--source ETOPO.nc] [--cache grids.npz] [--no-arith]
"""
import argparse, importlib.util, lzma, struct, zlib
from pathlib import Path
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('be', ROOT / 'tools/elevation-builder/build_elevation.py')
be = importlib.util.module_from_spec(spec); spec.loader.exec_module(be)

def deflate(b):
    c = zlib.compressobj(9, zlib.DEFLATED, -15, 9); return len(c.compress(b) + c.flush())
LZ = [{'id': lzma.FILTER_LZMA2, 'preset': 9 | lzma.PRESET_EXTREME}]
def xz(b): return len(lzma.compress(bytes(b), format=lzma.FORMAT_RAW, filters=LZ))

# ---------------------------------------------------------------- atlas
def read_atlas(path):
    d = Path(path).read_bytes()
    P, N, T = struct.unpack_from('<III', d, 8)
    table = [struct.unpack_from('<BxH', d, 24 + 4 * i) for i in range(P)]
    pts = struct.unpack_from('<%dH' % (2 * N), d, 24 + 4 * P)
    lines, k = [], 0
    for layer, n in table:
        lines.append((layer, [(pts[2 * (k + i)], pts[2 * (k + i) + 1]) for i in range(n)])); k += n
    return d, lines, 24 + 4 * P + 4 * N

def varint(u, out):
    while u >= 0x80: out.append(u & 0x7f | 0x80); u >>= 7
    out.append(u)
def zz(v): return v * 2 if v >= 0 else -v * 2 - 1

def atlas_report(path):
    d, lines, tab_end = read_atlas(path)
    raw_pts = d[24 + 4 * len(lines):tab_end]
    inter, lon, lat, counts = bytearray(), bytearray(), bytearray(), bytearray()
    for layer, pts in lines:
        counts.append(layer); varint(len(pts), counts)
        px = py = 0
        for x, y in pts:   # first point of each polyline is a delta from (0,0), i.e. absolute
            a, b = zz(x - px), zz(y - py)
            varint(a, inter); varint(b, inter); varint(a, lon); varint(b, lat); px, py = x, y
    rows = [('raw u16 pairs, no delta (current)', len(raw_pts), deflate(raw_pts), xz(raw_pts)),
            ('delta+zigzag varint, interleaved', len(inter), deflate(inter), xz(inter)),
            ('delta+zigzag varint, lon plane then lat plane', len(lon) + len(lat), deflate(lon + lat), xz(lon + lat))]
    return len(d), len(lines), len(counts), rows

# ---------------------------------------------------------------- elevation grids
def grids(source, cache):
    one = np.frombuffer((ROOT / 'data/elevation-1deg.gelv').read_bytes()[552:], np.uint8).reshape(180, 360)
    out = {1.0: one}
    if cache and Path(cache).exists():
        z = np.load(cache); return {float(k): z[k] for k in z.files}
    if not source: return out
    import h5py
    table = np.array(be.lookup(), dtype=np.float64)   # same quantization as build_elevation.quantize
    acc = {1.0: [], 0.5: [], 0.25: []}
    with h5py.File(source, 'r', rdcc_nbytes=64 << 20) as f:
        z = f['z']; assert z.shape == (10800, 21600)
        for y in range(180):               # one degree of rows (60) at a time, south first
            blk = z[y * 60:(y + 1) * 60, :]
            assert np.all(np.isfinite(blk)) and not np.any(blk == -99999)
            for deg, fy in ((1.0, 60), (0.5, 30), (0.25, 15)):
                w = round(360 / deg)
                m = blk.reshape(60 // fy, fy, w, 21600 // w).mean(axis=(1, 3), dtype=np.float64)
                hi = np.clip(np.searchsorted(table, m, 'left'), 1, 255)
                acc[deg].append(np.where(m - table[hi - 1] <= table[hi] - m, hi - 1, hi).astype(np.uint8))
    for deg in acc: out[deg] = np.concatenate(acc[deg]).reshape(-1, round(360 / deg))
    assert np.array_equal(out[1.0], one), 'derived 1 degree grid differs from data/elevation-1deg.gelv'
    if cache: np.savez(cache, **{str(k): v for k, v in out.items()})
    return out

def predict(a, mode):
    """Residual (mod 256) of each cell against a predictor from already-decoded neighbours.
    Missing neighbours at the top/left edge copy the other one; the corner predicts 128."""
    a = a.astype(np.int16)
    L = np.zeros_like(a); U = np.zeros_like(a); UL = np.zeros_like(a)
    L[:, 1:] = a[:, :-1]; L[1:, 0] = a[:-1, 0]; L[0, 0] = 128
    U[1:] = a[:-1]; U[0] = L[0]
    UL[1:, 1:] = a[:-1, :-1]; UL[0, :] = U[0, :]; UL[1:, 0] = L[1:, 0]
    if mode == 'left': p = L
    elif mode == 'up': p = U
    elif mode == 'avg': p = (L + U) >> 1
    else:  # paeth
        g = L + U - UL; dl, du, dul = abs(g - L), abs(g - U), abs(g - UL)
        p = np.where((dl <= du) & (dl <= dul), L, np.where(du <= dul, U, UL))
    return ((a - p) & 255).astype(np.uint8), L, U

# Adaptive binary range coder (LZMA style: 11-bit probabilities, shift 5, carry propagation).
# A symbol is coded MSB first with a binary tree of 255 probabilities per context; the context
# is the local gradient |left-up| bucketed into 6 classes (the decoder sees the same neighbours).
class Enc:
    def __init__(s): s.low = 0; s.rng = 0xFFFFFFFF; s.cache = 0; s.csize = 1; s.out = bytearray()
    def shift(s):
        if s.low < 0xFF000000 or s.low >= 1 << 32:
            c = s.low >> 32; t = s.cache
            while True:
                s.out.append((t + c) & 255); t = 255; s.csize -= 1
                if not s.csize: break
            s.cache = (s.low >> 24) & 255
        s.csize += 1; s.low = (s.low & 0x00FFFFFF) << 8
    def bit(s, pr, i, b):
        bound = (s.rng >> 11) * pr[i]
        if b: s.low += bound; s.rng -= bound; pr[i] -= pr[i] >> 5
        else: s.rng = bound; pr[i] += (2048 - pr[i]) >> 5
        while s.rng < 1 << 24: s.rng = (s.rng << 8) & 0xFFFFFFFF; s.shift()
    def finish(s):
        for _ in range(5): s.shift()
        return bytes(s.out[1:])           # first byte is always 0

class Dec:
    def __init__(s, b): s.b = b + b'\0' * 8; s.pos = 4; s.rng = 0xFFFFFFFF; s.code = int.from_bytes(s.b[:4], 'big')
    def bit(s, pr, i):
        bound = (s.rng >> 11) * pr[i]
        if s.code < bound: s.rng = bound; pr[i] += (2048 - pr[i]) >> 5; r = 0
        else: s.code -= bound; s.rng -= bound; pr[i] -= pr[i] >> 5; r = 1
        while s.rng < 1 << 24: s.rng <<= 8; s.code = (s.code << 8) | s.b[s.pos]; s.pos += 1
        return r

BUCKET = [0 if d == 0 else 1 if d == 1 else 2 if d < 4 else 3 if d < 8 else 4 if d < 24 else 5 for d in range(256)]
BUCKET_NP = np.array(BUCKET, np.uint8)

def arith_encode(a, mode):
    res, L, U = predict(a, mode)
    ctx = BUCKET_NP[np.abs(L - U)].ravel().tolist()
    pr = [[1024] * 256 for _ in range(6)]; e = Enc()
    for s, c in zip(res.ravel().tolist(), ctx):
        p = pr[c]; m = 1
        for k in (7, 6, 5, 4, 3, 2, 1, 0):
            b = (s >> k) & 1; e.bit(p, m, b); m = (m << 1) | b
    return e.finish()

def arith_decode_check(a, blob):          # round-trip proof for the avg predictor (decoder sketch)
    h, w = a.shape; d = Dec(blob); pr = [[1024] * 256 for _ in range(6)]
    out = np.zeros((h, w), np.int16)
    for y in range(h):
        for x in range(w):
            l = int(out[y, x - 1]) if x else (int(out[y - 1, 0]) if y else 128)
            u = int(out[y - 1, x]) if y else l
            p = pr[BUCKET[abs(l - u)]]; m = 1
            for _ in range(8): m = (m << 1) | d.bit(p, m)
            out[y, x] = ((m & 255) + ((l + u) >> 1)) & 255
    return np.array_equal(out, a)

def tiles(a, deg, td=16):
    t = round(td / deg); h, w = a.shape     # ragged at the east/north edges (360 and 180 are not multiples of 16)
    return [a[y:y + t, x:x + t] for y in range(0, h, t) for x in range(0, w, t)]

def measure(a, deg, do_arith):
    ts = tiles(a, deg); idx = 4 * len(ts)          # u32 offset per tile
    rows = [('raw indices', a.size, a.size, 0)]
    rows.append(('deflate 9, no predictor', deflate(a.tobytes()), sum(deflate(t.tobytes()) for t in ts) + idx, len(ts)))
    rows.append(('lzma, no predictor', xz(a.tobytes()), sum(xz(t.tobytes()) for t in ts) + idx, len(ts)))
    for mode in ('left', 'up', 'avg', 'paeth'):
        r = predict(a, mode)[0].tobytes(); rt = [predict(t, mode)[0].tobytes() for t in ts]
        rows.append((f'{mode} + deflate', deflate(r), sum(map(deflate, rt)) + idx, len(ts)))
    for mode in ('avg', 'paeth'):
        r = predict(a, mode)[0].tobytes(); rt = [predict(t, mode)[0].tobytes() for t in ts]
        rows.append((f'{mode} + lzma', xz(r), sum(map(xz, rt)) + idx, len(ts)))
    if do_arith:
        for mode in ('left', 'up', 'avg', 'paeth'):
            rows.append((f'{mode} + adaptive arith (6 ctx)', len(arith_encode(a, mode)),
                         sum(len(arith_encode(t, mode)) for t in ts) + idx, len(ts)))
    return rows

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--source'); ap.add_argument('--cache')
    ap.add_argument('--no-arith', action='store_true'); ap.add_argument('--atlas', default=str(ROOT / 'data/atlas.gatl'))
    args = ap.parse_args()
    total, nlines, tab, rows = atlas_report(args.atlas)
    print(f'## Atlas ({total} bytes file, {nlines} polylines; polyline table raw {4*nlines}, varint form {tab})\n')
    print('| scheme | point data bytes | + deflate | + lzma |\n|---|---:|---:|---:|')
    for n, a, b, c in rows: print(f'| {n} | {a} | {b} | {c} |')
    for deg, a in sorted(grids(args.source, args.cache).items(), reverse=True):
        print(f'\n## Elevation {deg} degree ({a.shape[1]}x{a.shape[0]}, {a.size} cells)\n')
        print('| scheme | whole grid bytes | ratio | 16 deg tiles bytes (incl. 4 B/tile index) | ratio | tiling overhead |\n|---|---:|---:|---:|---:|---:|')
        for n, w, t, nt in measure(a, deg, not args.no_arith):
            if nt == 0: print(f'| {n} | {w} | 1.00 | {w} | 1.00 | 0 |'); continue
            print(f'| {n} | {w} | {a.size/w:.2f} | {t} | {a.size/t:.2f} | {t/w-1:+.1%} ({nt} tiles) |')
        if deg == 1.0:
            blob = arith_encode(a, 'avg'); ok = arith_decode_check(a, blob)
            print(f'\n(arith round-trip decode of 1 degree avg stream: {"OK" if ok else "FAIL"}, {len(blob)} bytes)')
if __name__ == '__main__': main()
