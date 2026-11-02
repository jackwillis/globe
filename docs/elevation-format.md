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
  ~/scratch/globe-data/etopo/ETOPO_2022_v1_60s_N90W180_surface.nc data/elevation-1deg.gelv
```

The fixture tests all truncation lengths, extra bytes, every byte mutation, header/extent/lookup
violations with repaired CRCs, dimensions/size bounds and zero-allocation parsing. Initial pack
SHA-256: 2ceb255dcdad98fe28beaf85f1a64122956af4a8388e95f094858d3162b085be.
