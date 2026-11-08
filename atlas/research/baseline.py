#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Measure committed production pack payloads. No source acquisition or runtime changes."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

ROOT = Path(__file__).resolve().parents[2]

def inventory(root=ROOT):
    result = []
    for path in sorted((root / 'data').iterdir()):
        if path.suffix not in {'.gatl', '.gpol', '.gplc', '.gelv', '.ggrd'}:
            continue
        raw = path.read_bytes()
        magic = raw[:4].decode('ascii')
        entry = dict(path=str(path.relative_to(root)), format=magic,
                     packed_bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
        if magic == 'GATL':
            lines, points, text = struct.unpack_from('<III', raw, 8)
            layers = {}
            for i in range(lines):
                layer, _, count = struct.unpack_from('<BBH', raw, 24 + i * 4)
                bucket = layers.setdefault(str(layer), {'parts': 0, 'points': 0})
                bucket['parts'] += 1
                bucket['points'] += count
            assert len(raw) == 24 + 4 * lines + 4 * points + text
            assert zlib.crc32(raw[24:]) == struct.unpack_from('<I', raw, 20)[0]
            entry.update(records=lines, points=points, layers=layers,
                         provenance=raw[-text:].decode('utf-8'), decoded_additional_bytes=0)
        elif magic == 'GPOL':
            count, rings, points, text = struct.unpack_from('<IIII', raw, 8)
            assert len(raw) == 32 + 16 * count + 16 * rings + 8 * points + text
            assert zlib.crc32(raw[32:], zlib.crc32(raw[:28])) == struct.unpack_from('<I', raw, 28)[0]
            entry.update(records=count, rings=rings, points=points,
                         decoded_additional_bytes=0, scene_vector_payload_bytes=24 * points)
        elif magic == 'GPLC':
            count, text = struct.unpack_from('<II', raw, 8)
            assert len(raw) == 24 + 24 * count + text
            assert zlib.crc32(raw[24:], zlib.crc32(raw[:20])) == struct.unpack_from('<I', raw, 20)[0]
            entry.update(records=count, decoded_additional_bytes=0,
                         scene_vector_payload_bytes=24 * count)
        elif magic == 'GELV':
            w, h = struct.unpack_from('<II', raw, 8)
            assert len(raw) == 552 + w * h
            assert zlib.crc32(raw[40:], zlib.crc32(raw[:36])) == struct.unpack_from('<I', raw, 36)[0]
            corners, cells = (w + 1) * (h + 1), w * h
            entry.update(width=w, height=h, cells=cells, decoded_additional_bytes=0,
                         terrain_payload_estimate_bytes=49 * corners + 6 * cells + 3072,
                         terrain_estimate_assumptions='sizeof(Vec3)=24, CellBands=4, Rgb565=2; excludes object/allocator overhead')
        elif magic == 'GGRD':
            # Header fields are specified by the existing codec; decoding is tested by CTest.
            entry.update(width=360, height=180, decoded_additional_bytes=64800,
                         dimensions_basis='typed landclasses v1 contract; C++ decoder validates')
        provenance = {
            'GATL': {'sources': ['Natural Earth 110m coastline 4.1.0', 'Natural Earth 110m land boundaries 5.1.0'],
                     'license': 'public domain', 'command': 'python3 tools/atlas-builder/build_atlas.py ne_110m_coastline.zip ne_110m_admin_0_boundary_lines_land.zip OUT.gatl',
                     'selection': 'all source parts; quantize to 360/65536 degree, collapse adjacent duplicate coordinates',
                     'decoder': 'strict CRC/body and bounds; zero-copy; Scene derives unit vectors, caps and chunks'},
            'GPOL': {'sources': ['Natural Earth 110m admin0 countries 5.1.1'], 'license': 'public domain',
                     'command': 'python3 tools/country-builder/build_countries.py ne_110m_admin_0_countries.zip OUT.gpol',
                     'selection': 'all source country records; multipart rings/holes, microdegree coordinates; ASCII ADMIN names',
                     'decoder': 'strict bounds/CRC; zero-copy; CountryScene derives unit vectors'},
            'GPLC': {'sources': ['Wikidata retained response 2026-11-02'], 'license': 'CC0-1.0 structured data',
                     'command': 'python3 tools/places-builder/build_places.py --large data/wikidata-cities-large-20261102.json OUT.gplc',
                     'selection': 'largest population >=1000000; maximum 30 per country; lower QID tie; direct P31 query limits; Q3139 excluded',
                     'decoder': 'strict bounds/CRC/QID order/ASCII; zero-copy; PlaceScene derives unit vectors'},
            'GELV': {'sources': ['NOAA ETOPO 2022 v1 60s ice surface', 'Natural Earth 50m lakes 5.0.0'],
                     'license': 'CC0-1.0 NOAA and public domain Natural Earth',
                     'command': 'python3 tools/elevation-builder/build_elevation.py ETOPO.nc OUT.gelv --lakes ne_50m_lakes.zip --cell-degrees ' + ('0.5' if path.name == 'elevation-0.5deg.gelv' else '1'),
                     'selection': 'float64 mean of source-aligned cells; 256-height quantizer; lake-center nonnegative heights replaced by -1m water-color proxy',
                     'decoder': 'strict bounds/header+body CRC/LUT/extents; zero-copy; TerrainScene allocates geometry and workspaces'},
            'GGRD': {'sources': ['RESOLVE Ecoregions 2017'], 'license': 'CC-BY-4.0; data/LANDCLASS-NOTICE.txt required',
                     'command': 'python3 tools/landclass-builder/build_landclasses.py Ecoregions2017.zip OUT.ggrd',
                     'selection': 'cell-center even-odd fill; smallest ECO_ID overlap wins; 14 biomes collapsed to four classes plus unknown',
                     'decoder': 'strict stored/decoded CRCs, bounded grid decode; typed contract 360x180 and classes 0..4'}
        }[magic]
        entry.update(provenance)
        entry['source_hash_ledger'] = 'data/SOURCES.md and source hash constants in the cited builder; not reacquired for NOAA/RESOLVE in this audit'
        entry['persistent_payload_lower_bound_bytes'] = len(raw) + entry.get('scene_vector_payload_bytes', 0) + entry.get('terrain_payload_estimate_bytes', 0) + entry['decoded_additional_bytes']
        entry['temporary_load_basis'] = 'Embedded views allocate no input copy; file imports retain a full input buffer. Scene allocations coexist with source bytes; allocator/object overhead excluded.'
        result.append(entry)
    return {'schema': 1, 'basis': 'pack bytes measured; scene/cache payload arithmetic from source, not allocation profiling',
            'packs': result}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    output = json.dumps(inventory(), indent=2, ensure_ascii=False) + '\n'
    if args.output:
        args.output.write_text(output)
    else:
        print(output, end='')
