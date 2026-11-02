#!/usr/bin/env python3
"""Convert Natural Earth PolyLine shapefile ZIP using only Python's stdlib."""
import hashlib
import math
import struct
import sys
import zipfile
from pathlib import Path


def prepare(source, destination, member="ne_110m_coastline.shp"):
    with zipfile.ZipFile(source) as archive:
        raw = archive.read(member)
    if struct.unpack_from('>I', raw)[0] != 9994 or struct.unpack_from('<II', raw, 28) != (1000, 3):
        raise ValueError('expected PolyLine shapefile')
    lines = []
    offset = 100
    while offset < len(raw):
        _, words = struct.unpack_from('>II', raw, offset)
        offset += 8
        record = raw[offset:offset + words * 2]
        offset += words * 2
        if len(record) != words * 2 or struct.unpack_from('<I', record)[0] != 3:
            raise ValueError('invalid PolyLine record')
        parts, points = struct.unpack_from('<II', record, 36)
        indices = list(struct.unpack_from('<' + 'I' * parts, record, 44)) + [points]
        coords = [struct.unpack_from('<dd', record, 44 + 4 * parts + 16 * i) for i in range(points)]
        for start, end in zip(indices, indices[1:]):
            line = []
            for lon, lat in coords[start:end]:
                if not math.isfinite(lon + lat) or not -180.001 <= lon <= 180.001 or not -90 <= lat <= 90:
                    raise ValueError('invalid coordinate')
                lon = max(-180, min(180, lon))
                # Subdivide along shortest longitude difference, including dateline.
                if line:
                    old_lon, old_lat = line[-1]
                    delta = (lon - old_lon + 180) % 360 - 180
                    steps = max(1, math.ceil(max(abs(delta), abs(lat - old_lat)) / 2))
                    for j in range(1, steps):
                        line.append((((old_lon + delta * j / steps + 180) % 360) - 180,
                                     old_lat + (lat - old_lat) * j / steps))
                line.append((lon, lat))
            if len(line) >= 2:
                lines.append(line)
    with open(destination, 'wb') as out:
        out.write(b'PLNTMAP\0' + struct.pack('>III', 1, len(lines), sum(map(len, lines))))
        for line in lines:
            out.write(struct.pack('>I', len(line)))
            for lon, lat in line:
                # Unsigned offset microdegrees avoid signed conversion assumptions in C.
                out.write(struct.pack('>II', round((lon + 180) * 1e6), round((lat + 90) * 1e6)))
    print(f'{len(lines)} polylines, {sum(map(len, lines))} points')
    print('source SHA-256:', hashlib.sha256(Path(source).read_bytes()).hexdigest())


if __name__ == '__main__':
    prepare(*sys.argv[1:])
