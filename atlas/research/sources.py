#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Pinned HTTPS acquisition and a small strict reader for the audited NE SHP/DBF subset."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import urllib.request
import zipfile

ATLAS = Path(__file__).resolve().parents[1]

def locked_sources():
    return json.loads((ATLAS / 'sources.lock.json').read_text())['sources']

def verified_path(source, cache):
    path = Path(cache) / source['path']
    raw = path.read_bytes()
    if len(raw) != source['bytes'] or hashlib.sha256(raw).hexdigest() != source['sha256']:
        raise ValueError(f"{source['id']}: source checksum/length mismatch")
    return path

def acquire(source, cache):
    path = Path(cache) / source['path']
    if path.exists():
        return verified_path(source, cache)
    path.parent.mkdir(parents=True, exist_ok=True)
    if not source['url'].startswith('https://naturalearth.s3.amazonaws.com/'):
        raise ValueError('unexpected source host')
    req = urllib.request.Request(source['url'], headers={'User-Agent': 'globe-atlas-research/0.1'})
    with urllib.request.urlopen(req, timeout=120) as response:
        raw = response.read(source['bytes'] + 1)
    if len(raw) != source['bytes'] or hashlib.sha256(raw).hexdigest() != source['sha256']:
        raise ValueError(f"{source['id']}: downloaded source checksum/length mismatch")
    temporary = path.with_suffix('.partial')
    temporary.write_bytes(raw)
    temporary.replace(path)
    return path

def dbf_records(raw):
    if len(raw) < 33:
        raise ValueError('short DBF')
    count = struct.unpack_from('<I', raw, 4)[0]
    header, size = struct.unpack_from('<HH', raw, 8)
    if header < 33 or (header - 33) % 32 or header + count * size > len(raw):
        raise ValueError('DBF bounds')
    fields, offset = [], 1
    for i in range(32, header - 1, 32):
        name = raw[i:i+11].rstrip(b'\0').decode('ascii').upper()
        length = raw[i+16]
        fields.append((name, offset, length))
        offset += length
    if offset != size or raw[header-1] != 13:
        raise ValueError('DBF schema')
    rows = []
    for i in range(count):
        row = raw[header+i*size:header+(i+1)*size]
        if row[0] != 32:
            raise ValueError('deleted DBF record')
        rows.append({name: row[start:start+length].decode('utf-8').strip(' \0')
                     for name, start, length in fields})
    return rows

def shp_records(raw, coordinate_tolerance=1e-9):
    if len(raw) < 100 or struct.unpack_from('>I', raw)[0] != 9994:
        raise ValueError('SHP header')
    if struct.unpack_from('>I', raw, 24)[0] * 2 != len(raw):
        raise ValueError('SHP file size')
    version, shape = struct.unpack_from('<II', raw, 28)
    if version != 1000 or shape not in (1, 3, 5):
        raise ValueError('unsupported SHP shape')
    records, offset = [], 100
    while offset < len(raw):
        if offset + 8 > len(raw):
            raise ValueError('SHP record header')
        number, words = struct.unpack_from('>II', raw, offset)
        offset += 8
        record = raw[offset:offset+2*words]
        offset += 2*words
        if number != len(records)+1 or len(record) != 2*words or len(record) < 4:
            raise ValueError('SHP record bounds')
        if struct.unpack_from('<I', record)[0] == 0 and len(record) == 4:
            records.append([])  # Null geometry stays aligned with its DBF row.
            continue
        if struct.unpack_from('<I', record)[0] != shape:
            raise ValueError('SHP unexpected record type')
        if shape == 1:
            if len(record) != 20:
                raise ValueError('SHP point size')
            parts = [[struct.unpack_from('<dd', record, 4)]]
        else:
            if len(record) < 44:
                raise ValueError('SHP short geometry')
            n, p = struct.unpack_from('<II', record, 36)
            if not n or len(record) != 44 + 4*n + 16*p:
                raise ValueError('SHP geometry size')
            starts = list(struct.unpack_from('<'+'I'*n, record, 44)) + [p]
            if starts[0] != 0 or any(a >= b for a, b in zip(starts, starts[1:])):
                raise ValueError('SHP part offsets')
            coords = [struct.unpack_from('<dd', record, 44+4*n+16*i) for i in range(p)]
            parts = [coords[a:b] for a, b in zip(starts, starts[1:])]
        for part in parts:
            for lon, lat in part:
                if not math.isfinite(lon) or not math.isfinite(lat) or abs(lon) > 180+coordinate_tolerance or abs(lat) > 90+coordinate_tolerance:
                    raise ValueError('SHP coordinate')
        records.append(parts)
    return shape, records

def read_source(source, cache):
    with zipfile.ZipFile(verified_path(source, cache)) as archive:
        base = source['id']
        crs = archive.read(base + '.prj').decode('ascii').strip()
        encoding = archive.read(base + '.cpg').decode('ascii').strip().upper()
        if not crs.startswith('GEOGCS["GCS_WGS_1984",') or 'PRIMEM["Greenwich",0' not in crs or 'UNIT["Degree",0.017453292519943' not in crs:
            raise ValueError('expected WGS84 geographic degrees')
        if encoding != 'UTF-8':
            raise ValueError('expected UTF-8 DBF encoding')
        rows = dbf_records(archive.read(base + '.dbf'))
        shape, geometries = shp_records(archive.read(base + '.shp'),
            coordinate_tolerance=1e-6 if base == 'ne_110m_coastline' else 1e-9)
        version = archive.read(base + '.VERSION.txt').decode().strip()
    if version != source['version'] or len(rows) != len(geometries):
        raise ValueError('source version/record mismatch')
    return shape, list(zip(rows, geometries))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache', type=Path)
    parser.add_argument('--download', action='store_true', help='explicitly acquire missing locked files')
    args = parser.parse_args()
    for source in locked_sources():
        (acquire if args.download else verified_path)(source, args.cache)
        print(source['id'], 'verified')
