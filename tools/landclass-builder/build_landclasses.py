#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Pinned RESOLVE 2017 biomes -> south-first one-degree GGRD class grid (CC BY 4.0 data)."""
import argparse
import hashlib
import importlib.util
import math
from pathlib import Path
import struct
import zipfile

SOURCE_SHA256 = 'be36d6209e443038d02e309f0447c6e7f2a62f5fe60c605ffe90d064952f2a60'
UNKNOWN, DESERT, FOREST, GRASS_SHRUB, TUNDRA = range(5)


def biome_class(number, name):
    if name == 'N/A':
        return UNKNOWN  # Rock/ice is deliberately not a Greenland ice flag (GL-40).
    if number in (1, 2, 3, 4, 5, 6, 14):
        return FOREST
    if number in (7, 8, 9, 10, 12):
        return GRASS_SHRUB
    if number == 11:
        return TUNDRA
    if number == 13:
        return DESERT
    raise ValueError('unknown biome')


def cell_centers(parts, width=360, height=180):
    """Even-odd filling, all rings of one polygon together; half-open edges, longitude wrap.

    Bucket edge intersections by sampled latitude: O(vertices + crossings), rather than
    rescanning millions of vertices for every latitude. Rings are unwrapped and hole-aligned.
    """
    intersections = [[] for _ in range(height)]
    anchor = None
    for part in parts:
        if len(part) < 4 or part[0] != part[-1]:
            raise ValueError('unclosed/short ring')
        ring = []
        previous = part[0][0]
        for lon, lat in part:
            if not math.isfinite(lon + lat) or not -180 - 1e-9 <= lon <= 180 + 1e-9 or not -90 - 1e-9 <= lat <= 90 + 1e-9:
                raise ValueError('coordinate')
            lon = min(180, max(-180, lon))
            lat = min(90, max(-90, lat))
            lon = previous + (lon - previous + 180) % 360 - 180
            ring.append((lon, lat))
            previous = lon
        middle = sum(x for x, _ in ring) / len(ring)
        if anchor is None:
            anchor = middle
        shift = 360 * round((anchor - middle) / 360)
        for (ax, ay), (bx, by) in zip(ring, ring[1:]):
            if ay == by:
                continue
            low, high = sorted((ay, by))
            first = max(0, math.ceil((low + 90) * height / 180 - 0.5))
            end = min(height, math.ceil((high + 90) * height / 180 - 0.5))
            for row in range(first, end):
                lat = -90 + (row + 0.5) * 180 / height
                intersections[row].append(ax + shift + (lat - ay) * (bx - ax) / (by - ay))
    selected = set()
    for row, crossings in enumerate(intersections):
        crossings.sort()
        if len(crossings) % 2:
            raise ValueError('odd intersections')
        for left, right in zip(crossings[::2], crossings[1::2]):
            first = math.ceil((left + 180) * width / 360 - 0.5)
            end = math.ceil((right + 180) * width / 360 - 0.5)
            if end - first > width:
                raise ValueError('polygon spans more than globe')
            for col in range(first, end):
                selected.add(row * width + col % width)
    return selected


def records(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as file:
        while block := file.read(1024 * 1024):
            digest.update(block)
    if digest.hexdigest() != SOURCE_SHA256:
        raise ValueError('source SHA-256 mismatch')
    with zipfile.ZipFile(path) as archive:
        if b'EPSG","4326' not in archive.read('Ecoregions2017.prj'):
            raise ValueError('expected WGS84 geographic CRS')
        dbf = archive.read('Ecoregions2017.dbf')
        count = struct.unpack_from('<I', dbf, 4)[0]
        header, row_size = struct.unpack_from('<HH', dbf, 8)
        fields = {}
        offset = 1
        for at in range(32, header - 1, 32):
            name = dbf[at:at + 11].rstrip(b'\0').decode('ascii')
            size = dbf[at + 16]
            fields[name] = (offset, size)
            offset += size
        if count != 847 or offset != row_size or header + count * row_size > len(dbf):
            raise ValueError('DBF schema/record count')
        def field(row, name):
            offset, size = fields[name]
            return row[offset:offset + size].decode('utf-8').strip(' \0')
        with archive.open('Ecoregions2017.shp') as shp:
            header_shp = shp.read(100)
            if struct.unpack_from('>I', header_shp)[0] != 9994 or struct.unpack_from('<II', header_shp, 28) != (1000, 5):
                raise ValueError('Polygon shapefile expected')
            for index in range(count):
                row = dbf[header + index * row_size:header + (index + 1) * row_size]
                if row[0] != 32 or field(row, 'LICENSE') != 'CC-BY 4.0':
                    raise ValueError('deleted row or unexpected license')
                prefix = shp.read(8)
                if len(prefix) != 8:
                    raise ValueError('missing shape record')
                size = struct.unpack_from('>I', prefix, 4)[0] * 2
                data = shp.read(size)
                if len(data) != size or size < 44 or struct.unpack_from('<I', data)[0] != 5:
                    raise ValueError('polygon record')
                parts, points = struct.unpack_from('<II', data, 36)
                if not parts or len(data) != 44 + parts * 4 + points * 16:
                    raise ValueError('polygon length')
                starts = list(struct.unpack_from('<' + 'I' * parts, data, 44)) + [points]
                if starts[0] or any(not 0 <= a < b <= points for a, b in zip(starts, starts[1:])):
                    raise ValueError('ring bounds')
                kind = biome_class(int(float(field(row, 'BIOME_NUM'))), field(row, 'BIOME_NAME'))
                if kind == UNKNOWN:
                    continue
                coords = list(struct.iter_unpack('<dd', data[44 + 4 * parts:]))
                yield int(field(row, 'ECO_ID')), kind, [coords[a:b] for a, b in zip(starts, starts[1:])]
            if shp.read(1):
                raise ValueError('trailing shapes')


def build(source, output):
    cells = bytearray(360 * 180)
    # Polygon order must not decide boundary conflicts: smallest ECO_ID wins.
    owners = [2**31] * len(cells)
    count = overlaps = 0
    for eco_id, kind, parts in records(source):
        count += 1
        for index in cell_centers(parts):
            overlaps += owners[index] != 2**31
            if eco_id < owners[index]:
                owners[index] = eco_id
                cells[index] = kind
    spec = importlib.util.spec_from_file_location('grid_codec', Path(__file__).parents[1] / 'grid-codec/grid_codec.py')
    codec = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(codec)
    output = Path(output)
    output.write_bytes(codec.encode_grid(360, 180, cells))
    print('classified records', count, 'overlapping centers', overlaps, 'class counts', [cells.count(i) for i in range(5)])
    print('output', output.stat().st_size, 'SHA256', hashlib.sha256(output.read_bytes()).hexdigest())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source')
    parser.add_argument('output')
    args = parser.parse_args()
    build(args.source, args.output)
