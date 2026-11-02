# Compression sizes, 2026-11-01

Modern Python prototype, not a target codec or a runtime format change. The pinned ETOPO source
was processed by the same verified builder at 0.25 degrees: 1440 x 720, 1,037,352 bytes, SHA-256
`b6d9c8a4ae4cf66c67d6c247057c15e23416f6341a734965b33e07cf859b2241`.
The optional raw pack is outside the repository. It exceeds the current whole-grid terrain cache
budget; this measurement does not make it an importable runtime level under that budget.

Sizes below are for the 1,036,800 **cell bytes**, excluding the 552-byte GELV metadata and any
future compression container. DEFLATE includes zlib's wrapper, level 9. The predictor clamps
left + up - upper-left to 0..255 and stores the index error modulo 256. South-first rows are kept.
The byte range coder uses an adaptive order-0 256-symbol model, initially uniform, halved at total
32768. Every candidate was decoded and compared byte for byte with its input; predictor reversal
also reproduces the original grid.

| Cell representation | Encoded bytes | Fraction of raw cells |
| --- | ---: | ---: |
| Raw | 1,036,800 | 100% |
| DEFLATE | 500,301 | 48.26% |
| Predictor + DEFLATE | 428,179 | 41.30% |
| Adaptive range coder | 804,716 | 77.62% |
| Predictor + adaptive range coder | 392,593 | 37.87% |

Host Python encode/decode times are in the raw JSON and are **not G4 timing estimates**. No target
implementation, memory-size report or G4 decode benchmark exists for these candidates. The smaller
range-coded result alone is insufficient to choose a runtime decoder. The embedded v1 data stays
raw until a separately versioned bounded container and target validation are designed.

Atlas point deltas use zigzag varints, resetting at each polyline and wrapping longitude deltas
to the shorter signed 16-bit difference. The 32,480 raw point bytes become 26,706 bytes (82.22%);
keeping existing tables/header/provenance would give 29,294 bytes versus 35,068. Independent
varint reconstruction reproduces every quantized point. This replaces the earlier approximate
25,800-byte estimate, and does not establish a new GATL layout.

Reproduce with `tools/compression_bench.py QUARTER.gelv data/atlas.gatl OUTPUT.json`.
Tests include empty/constant/random streams, all byte values, model rescaling, predictor reversal
and atlas roundtrip. Model-update, predictor-diagonal and zero-varint mutants are rejected.
