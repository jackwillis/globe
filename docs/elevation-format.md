# GELV v1 elevation grid

All integers are little-endian, unsigned unless indicated. No runtime compression or allocation.

| Offset | Bytes | Field |
| --- | --- | --- |
| 0 | 4 | ASCII GELV |
| 4 | 2 | Version 1 |
| 6 | 2 | Flags, zero |
| 8, 12 | 4 each | Width, height (1..32768; product at most 16,777,216) |
| 16, 20, 24, 28 | 4 each | Signed west, south, east, north extents in microdegrees |
| 32 | 2 | Lookup entry count, exactly 256 |
| 34 | 2 | Reserved, zero |
| 36 | 4 | IEEE CRC-32 of bytes 0..35 followed by bytes 40..EOF |
| 40 | 512 | 256 strictly increasing signed int16 meter values |
| 552 | width * height | One lookup index per cell, row-major south to north, west to east |

File length must be exactly 552 + width*height. Extents must increase within +/-180 longitude and
+/-90 latitude. Regional packs fit these fields; antimeridian-crossing regions must be split.
CRC covers header metadata as well as payload (excluding only its own field), unlike GATL's
body-only CRC. This prevents an accidental extent/shape change from remaining valid. Coordinates
refer to cell edges, not sample centers. Missing cells are not represented in v1; builders reject
missing input rather than silently paint an invented height. Values are meters relative to source
vertical datum; this edition uses EGM2008. A future incompatible datum/encoding needs a new version.

The initial 360x180 pack is 65,352 bytes. The lookup uses linear integer interpolation between
(index:meters) anchors 0:-11000, 64:-2000, 112:-200, 127:-1, 128:0, 192:1000, 240:5000, 255:9000.
This spends more entries near sea level and lowland, while spanning ocean trenches and mountains.
Each one-degree cell is the float64 arithmetic mean of its 60x60 float32 source cells. Quantization
chooses the nearest lookup height, lower index on ties, with endpoint clamping. This is a display
pack, not an analytical or navigational elevation product.

`tools/elevation-builder/build_elevation.py` verifies the source SHA-256 before reading metadata,
checks coordinate spacing/order and missing values, then reads bounded row blocks. h5py/numpy are
modern-builder dependencies only. --cell-degrees also supports source-aligned resolutions such as
0.5 and 0.25 degrees within the decoder cell budget. The runtime stays zero-copy and endian-explicit.
The standard-library encoder and independent Python/C++ fixture checks run without those packages.

Source/license/hash are in data/SOURCES.md. Reproduce with a modern Python containing h5py/numpy:

```
python3 tools/elevation-builder/build_elevation.py \
  <verified-source-cache>/etopo/ETOPO_2022_v1_60s_N90W180_surface.nc data/elevation-1deg.gelv
```

The fixture tests all truncation lengths, extra bytes, every byte mutation, header/extent/lookup
violations with repaired CRCs, dimensions/size bounds and zero-allocation parsing. Initial pack
SHA-256: 2ceb255dcdad98fe28beaf85f1a64122956af4a8388e95f094858d3162b085be.

## Lake water mask (GL-28, 2026-11-02)

The bundled build uses the pinned Natural Earth 50m lakes 5.0.0 ZIP as an additional build-time
input (`--lakes PATH`). A global cell is water when its center is inside the polygon by even-odd
ring filling. Rings are unwrapped at the antimeridian; islands remain holes; rows remain south-first.
This center rule is exact for its chosen samples and avoids the earlier 16x16 area approximation.
It selects 30 Great Lakes cells (the earlier >50%-area estimate selected 29).

GELV v1 has no separate water flag. Selected cells whose quantized height is nonnegative become
index 127, the existing -1 m shallow-water **color proxy**. Existing negative heights are retained.
Consequently this pack does not preserve the positive surface/bed elevation of marked inland lakes;
it must not be used as an accurate bathymetry product. Header, lookup table, flags and decoder are
unchanged. A future water/ice flag needs a separately reviewed version; none is introduced here.

All 412 source records were read: 135 selected cell centers worldwide, 132 changed nonnegative
cells. The five Great Lakes contribute 30 centers, 29 changed cells. An independent ray-crossing
check reproduces those 30 centers; a pinned fixture checks that the embedded pack keeps them water.
The source's Caspian omission and resolution limitations remain (see data/SOURCES.md).

Two complete source conversions produced identical 65,352-byte files, SHA-256
`8d0fd2e6777f24dc13ac47e0f73aa459e503c70116d66a61bb2b339b618766a6`.
Without `--lakes`, the builder still reproduces the historical raw ETOPO grid used by compression
experiments. The current bundled build command is:

```
python tools/elevation-builder/build_elevation.py ETOPO.nc elevation-1deg.gelv --lakes ne_50m_lakes.zip
```

The settled replay pin deliberately changes 27336cb8117e5e7f -> efa030af1a10f887 for the water mask
and shared-corner shades. Host and G4 agree. Hole omission, north/south reversal and wrong-water-index
mutants fail. A stale Python bytecode cache initially replayed the last same-length mutant after
source restoration; clearing only that generated cache and repeating all mutations resolved it.
