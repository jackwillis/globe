# Compression experiment (2026-11-01)

Script: `tools/compression-experiment/run.py` (Python + numpy; h5py only to derive the 0.5 and
0.25 degree grids from the ETOPO 2022 60 arc-second ice-surface NetCDF, which was readable via
h5py 3.16 installed in a throwaway venv). Everything in the tables below is **measured** by that
script; sections marked *inference* are not.

Method notes (measured setup):
- GELV sizes are cell-index payload only. The 552-byte header is excluded; its 512-byte lookup
  table is a fixed function of 8 anchors, so a compressed edition could regenerate it.
- The 0.5 and 0.25 degree grids use `build_elevation.py`'s lookup table and quantization
  (nearest entry, lower index on ties) applied to float64 cell means. The derived 1 degree grid
  was asserted byte-identical to `data/elevation-1deg.gelv`.
- Deflate is raw deflate level 9 (no zlib wrapper, 6 bytes less); lzma is LZMA2 FORMAT_RAW, preset
  9 extreme. Per-stream overhead is therefore minimal.
- Predictors work on the 8-bit lookup index with wrap-around residuals. Missing top/left
  neighbours copy the other neighbour; the corner predicts 128. Tiles reset these rules.
- Adaptive arithmetic coder: LZMA-style binary range coder, 11-bit probabilities (shift 5), 8-bit
  binary tree per context, 6 contexts from the bucketed gradient |left - up|. Round-trip decode
  verified for the 1 degree `avg` stream.
- Tiled: independent 16x16 degree tiles (ragged at the east/north edges since 360 and 180 are not
  multiples of 16), 276 tiles, each coded with a fresh model and counted with a 4-byte offset.
  Overhead is relative to the same scheme on the whole grid.

## Atlas (35068 bytes file, 467 polylines; polyline table raw 1868, varint form 941)

| scheme | point data bytes | + deflate | + lzma |
|---|---:|---:|---:|
| raw u16 pairs, no delta (current) | 32480 | 31113 | 27500 |
| delta+zigzag varint, interleaved | 26736 | 22963 | 20072 |
| delta+zigzag varint, lon plane then lat plane | 26736 | 22914 | 20036 |

## Elevation 1.0 degree (360x180, 64800 cells)

| scheme | whole grid bytes | ratio | 16 deg tiles bytes (incl. 4 B/tile index) | ratio | tiling overhead |
|---|---:|---:|---:|---:|---:|
| raw indices | 64800 | 1.00 | 64800 | 1.00 | 0 |
| deflate 9, no predictor | 41238 | 1.57 | 48205 | 1.34 | +16.9% (276 tiles) |
| lzma, no predictor | 34414 | 1.88 | 49391 | 1.31 | +43.5% (276 tiles) |
| left + deflate | 34282 | 1.89 | 41119 | 1.58 | +19.9% (276 tiles) |
| up + deflate | 36716 | 1.76 | 44682 | 1.45 | +21.7% (276 tiles) |
| avg + deflate | 34128 | 1.90 | 41427 | 1.56 | +21.4% (276 tiles) |
| paeth + deflate | 33895 | 1.91 | 41039 | 1.58 | +21.1% (276 tiles) |
| avg + lzma | 30872 | 2.10 | 47835 | 1.35 | +54.9% (276 tiles) |
| paeth + lzma | 30890 | 2.10 | 48276 | 1.34 | +56.3% (276 tiles) |
| left + adaptive arith (6 ctx) | 29862 | 2.17 | 50144 | 1.29 | +67.9% (276 tiles) |
| up + adaptive arith (6 ctx) | 32022 | 2.02 | 51157 | 1.27 | +59.8% (276 tiles) |
| avg + adaptive arith (6 ctx) | 29661 | 2.18 | 49699 | 1.30 | +67.6% (276 tiles) |
| paeth + adaptive arith (6 ctx) | 29140 | 2.22 | 50454 | 1.28 | +73.1% (276 tiles) |

(arith round-trip decode of 1 degree avg stream: OK, 29661 bytes)

## Elevation 0.5 degree (720x360, 259200 cells)

| scheme | whole grid bytes | ratio | 16 deg tiles bytes (incl. 4 B/tile index) | ratio | tiling overhead |
|---|---:|---:|---:|---:|---:|
| raw indices | 259200 | 1.00 | 259200 | 1.00 | 0 |
| deflate 9, no predictor | 142689 | 1.82 | 150918 | 1.72 | +5.8% (276 tiles) |
| lzma, no predictor | 119077 | 2.18 | 144458 | 1.79 | +21.3% (276 tiles) |
| left + deflate | 117513 | 2.21 | 124368 | 2.08 | +5.8% (276 tiles) |
| up + deflate | 125015 | 2.07 | 135505 | 1.91 | +8.4% (276 tiles) |
| avg + deflate | 115974 | 2.23 | 124629 | 2.08 | +7.5% (276 tiles) |
| paeth + deflate | 116610 | 2.22 | 124406 | 2.08 | +6.7% (276 tiles) |
| avg + lzma | 104574 | 2.48 | 133215 | 1.95 | +27.4% (276 tiles) |
| paeth + lzma | 105365 | 2.46 | 134689 | 1.92 | +27.8% (276 tiles) |
| left + adaptive arith (6 ctx) | 101019 | 2.57 | 133000 | 1.95 | +31.7% (276 tiles) |
| up + adaptive arith (6 ctx) | 108312 | 2.39 | 138289 | 1.87 | +27.7% (276 tiles) |
| avg + adaptive arith (6 ctx) | 99691 | 2.60 | 131159 | 1.98 | +31.6% (276 tiles) |
| paeth + adaptive arith (6 ctx) | 98999 | 2.62 | 133517 | 1.94 | +34.9% (276 tiles) |

## Elevation 0.25 degree (1440x720, 1036800 cells)

| scheme | whole grid bytes | ratio | 16 deg tiles bytes (incl. 4 B/tile index) | ratio | tiling overhead |
|---|---:|---:|---:|---:|---:|
| raw indices | 1036800 | 1.00 | 1036800 | 1.00 | 0 |
| deflate 9, no predictor | 501129 | 2.07 | 501027 | 2.07 | -0.0% (276 tiles) |
| lzma, no predictor | 414116 | 2.50 | 451208 | 2.30 | +9.0% (276 tiles) |
| left + deflate | 408796 | 2.54 | 415169 | 2.50 | +1.6% (276 tiles) |
| up + deflate | 433105 | 2.39 | 449937 | 2.30 | +3.9% (276 tiles) |
| avg + deflate | 401287 | 2.58 | 413283 | 2.51 | +3.0% (276 tiles) |
| paeth + deflate | 407754 | 2.54 | 417639 | 2.48 | +2.4% (276 tiles) |
| avg + lzma | 359499 | 2.88 | 402571 | 2.58 | +12.0% (276 tiles) |
| paeth + lzma | 366339 | 2.83 | 409960 | 2.53 | +11.9% (276 tiles) |
| left + adaptive arith (6 ctx) | 348109 | 2.98 | 378978 | 2.74 | +8.9% (276 tiles) |
| up + adaptive arith (6 ctx) | 372997 | 2.78 | 400240 | 2.59 | +7.3% (276 tiles) |
| avg + adaptive arith (6 ctx) | 342041 | 3.03 | 373355 | 2.78 | +9.2% (276 tiles) |
| paeth + adaptive arith (6 ctx) | 343142 | 3.02 | 379054 | 2.74 | +10.5% (276 tiles) |

## Findings (measured unless stated)

- Atlas point data: delta+zigzag varint is 26,736 bytes (-17.7% vs raw 32,480), close to the
  estimate in atlas-format.md (about 25,800). Adding deflate gives about 22,900 and lzma about
  20,000 (raw u16 pairs: 31,113 deflate, 27,500 lzma). Whole-file estimate with varint+lzma
  (inference): about 20,072 + 941 table + about 940 header/provenance, roughly 22,000 of 35,068.
- Elevation: the 8-bit index grid is hard to compress. 1 degree: best is paeth + arithmetic at
  29,140 (2.22x); the best deflate variant (paeth) gets 33,895 (1.91x). Higher resolutions compress
  better (0.25 degree: 3.03x with avg + arithmetic) because neighbouring cells are more correlated.
- Tiling costs +17..+73% at 1 degree, +6..+35% at 0.5 degree and +2..+12% at 0.25 degree; adaptive
  models and LZ windows have little data to learn from in small tiles, and the arithmetic coder
  suffers most because it re-learns its probabilities per tile. Deflate with a left/avg
  predictor suffers least (about +20% at 1 degree, +3% at 0.25).

## Decoder complexity (inference, approximate)

| Scheme | Decoder size | Notes |
| --- | --- | --- |
| delta varint | about 15 lines C++ | no tables, trivially streaming |
| inflate | about 250-400 lines, or link zlib | dynamic Huffman; would have to be vendored or ported for OS 9 |
| lzma | about 600-1000 lines (xz-embedded/LzmaDec scale), 10-30 KB code | needs a dictionary buffer (hundreds of KB for large grids) |
| predictor (left/up/avg) | about 5-10 lines extra | paeth about 12 lines |
| adaptive binary range coder | about 40-60 lines, state 6 x 256 x u16 = 3 KB | no dictionary; sequential, a few hundred cycles per cell |
| tiling | about 20 lines plus a tile index | enables random access and bounded memory |

## Recommendation for the floppy edition (inference)

The 1 degree pack is 65 KB raw, so ratios of 1.9-2.2x save at most about 35 KB. If that matters,
**avg predictor + adaptive range coder, untiled** (29,661 bytes, 2.18x measured; paeth 29,140) is
the best size for the code: about 60 lines of decoder, no dictionary, no vendored library, versus
250+ lines for inflate reaching 1.90x. For the atlas, delta varint alone (-5.7 KB, about 15 lines)
is the pragmatic first step; the same range coder with a byte model might approach lzma at far
lower decoder cost, but that was not measured. Tiling is only worthwhile if random access or low
memory is required (it costs 17-73% at 1 degree), or at 0.25 degree where overhead is about 2-12%;
a pre-trained initial probability table per tile would likely cut the arithmetic tiling cost (not
measured). Decode speed on the G4/OS 9 target was not measured.
