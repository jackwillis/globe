#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Build the bundled GATL atlas from Natural Earth shapefile ZIPs (stdlib only).

    build_atlas.py COASTLINE.zip BORDERS.zip OUT.gatl

The source ZIPs are verified against the SHA-256 values below; a mismatch is an error unless
--allow-hash-mismatch is given (the provenance text then records the actual hashes). See
docs/atlas-format.md for the output format.
"""
import argparse
import hashlib
import math
import struct
import sys
import zlib
import zipfile

MAGIC = b"GATL"
VERSION = 1
LAYER_COASTLINE = 1
LAYER_BORDER = 2
MAX_POLYLINES = 65536
MAX_POINTS_PER_POLYLINE = 65535
MAX_POINTS = 1000000
MAX_TEXT = 4096
STEP = 360.0 / 65536.0

# name, expected ZIP SHA-256, shapefile member, URL, layer
SOURCES = {
    "coastline": {
        "title": "Natural Earth 1:110m coastline",
        "sha256": "664449b39070027e882abb295974d182afec18ca21107273d17e9e8bf6f64817",
        "member": "ne_110m_coastline",
        "url": "https://naturalearth.s3.amazonaws.com/110m_physical/ne_110m_coastline.zip",
        "layer": LAYER_COASTLINE,
    },
    "borders": {
        "title": "Natural Earth 1:110m admin-0 boundary lines (land)",
        "sha256": "bb2954981570498a4b4344f8e83fa54caa5858e15e800094112407d2bd2f940d",
        "member": "ne_110m_admin_0_boundary_lines_land",
        "url": "https://naturalearth.s3.amazonaws.com/110m_cultural/"
        "ne_110m_admin_0_boundary_lines_land.zip",
        "layer": LAYER_BORDER,
    },
}


def quantize(lon, lat):
    """Returns (lon_q, lat_q). Longitude +180 wraps to 0: the same point as -180 on a sphere."""
    qlon = math.floor((lon + 180.0) / STEP + 0.5) % 65536
    qlat = math.floor((lat + 90.0) / STEP + 0.5)
    return qlon, min(max(qlat, 0), 32768)


def read_polylines(shp):
    """Parts of every PolyLine record as lists of (lon, lat), original vertices, in file order."""
    if len(shp) < 100 or struct.unpack_from(">I", shp)[0] != 9994:
        raise ValueError("not a shapefile")
    if struct.unpack_from("<II", shp, 28) != (1000, 3):
        raise ValueError("expected PolyLine shapefile")
    lines = []
    offset = 100
    while offset < len(shp):
        if offset + 8 > len(shp):
            raise ValueError("truncated record header")
        _, words = struct.unpack_from(">II", shp, offset)
        offset += 8
        record = shp[offset : offset + words * 2]
        offset += words * 2
        if len(record) != words * 2 or len(record) < 44 or struct.unpack_from("<I", record)[0] != 3:
            raise ValueError("invalid PolyLine record")
        parts, points = struct.unpack_from("<II", record, 36)
        if len(record) < 44 + 4 * parts + 16 * points:
            raise ValueError("truncated PolyLine record")
        starts = list(struct.unpack_from("<" + "I" * parts, record, 44)) + [points]
        coords = [
            struct.unpack_from("<dd", record, 44 + 4 * parts + 16 * i) for i in range(points)
        ]
        for a, b in zip(starts, starts[1:]):
            if not 0 <= a <= b <= points:
                raise ValueError("invalid part index")
            part = coords[a:b]
            for lon, lat in part:
                if not (math.isfinite(lon) and math.isfinite(lat)):
                    raise ValueError("non-finite coordinate")
                if not -180.001 <= lon <= 180.001 or not -90.0 <= lat <= 90.0:
                    raise ValueError("coordinate out of range")
            lines.append([(max(-180.0, min(180.0, lon)), lat) for lon, lat in part])
    return lines


def quantized_polyline(line):
    """Quantize and collapse consecutive duplicates; None if fewer than two points remain."""
    out = []
    for lon, lat in line:
        q = quantize(lon, lat)
        if not out or out[-1] != q:
            out.append(q)
    return out if len(out) >= 2 else None


def encode(layers, text):
    """layers: list of (layer_id, [[(lon_q, lat_q), ...], ...]); text: provenance str -> bytes."""
    table = bytearray()
    points = bytearray()
    polyline_count = 0
    point_count = 0
    for layer, lines in layers:
        for line in lines:
            if len(line) < 2 or len(line) > MAX_POINTS_PER_POLYLINE:
                raise ValueError("polyline point count out of range")
            table += struct.pack("<BBH", layer, 0, len(line))
            for qlon, qlat in line:
                points += struct.pack("<HH", qlon, qlat)
            polyline_count += 1
            point_count += len(line)
    if polyline_count > MAX_POLYLINES or point_count > MAX_POINTS:
        raise ValueError("atlas too large")
    raw = text.encode("utf-8")
    if len(raw) > MAX_TEXT or b"\0" in raw:
        raise ValueError("invalid provenance text")
    body = bytes(table) + bytes(points) + raw
    header = struct.pack(
        "<4sHHIIII", MAGIC, VERSION, 0, polyline_count, point_count, len(raw), zlib.crc32(body)
    )
    return header + body


def provenance(infos):
    parts = ["Globe atlas, format GATL 1; quantized to 360/65536 degrees."]
    for name in ("coastline", "borders"):
        info = infos[name]
        parts.append(
            f"{name}: {SOURCES[name]['title']} {info['version']}, {SOURCES[name]['url']}, "
            f"zip sha256 {info['sha256']}."
        )
    parts.append(
        "License: public domain (https://www.naturalearthdata.com/about/terms-of-use/). "
        "Natural Earth draws de facto boundaries; borders are a cartographic reference layer, "
        "not a statement about sovereignty."
    )
    return " ".join(parts)


def load_source(name, path, allow_mismatch):
    with open(path, "rb") as f:
        data = f.read()
    digest = hashlib.sha256(data).hexdigest()
    expected = SOURCES[name]["sha256"]
    if digest != expected and not allow_mismatch:
        raise SystemExit(f"{name}: sha256 {digest} != expected {expected} (--allow-hash-mismatch)")
    with zipfile.ZipFile(path) as archive:
        shp = archive.read(SOURCES[name]["member"] + ".shp")
        version = archive.read(SOURCES[name]["member"] + ".VERSION.txt").decode("ascii").strip()
    return shp, {"sha256": digest, "version": version}


def build(coast_zip, borders_zip, allow_mismatch=False):
    layers = []
    infos = {}
    for name, path in (("coastline", coast_zip), ("borders", borders_zip)):
        shp, infos[name] = load_source(name, path, allow_mismatch)
        lines = [quantized_polyline(line) for line in read_polylines(shp)]
        layers.append((SOURCES[name]["layer"], [line for line in lines if line]))
    return encode(layers, provenance(infos)), layers


def main(argv):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("coastline_zip")
    ap.add_argument("borders_zip")
    ap.add_argument("output")
    ap.add_argument("--allow-hash-mismatch", action="store_true")
    args = ap.parse_args(argv)
    blob, layers = build(args.coastline_zip, args.borders_zip, args.allow_hash_mismatch)
    with open(args.output, "wb") as out:
        out.write(blob)
    for (layer, lines), label in zip(layers, ("coastline", "borders")):
        print(f"{label}: {len(lines)} polylines, {sum(len(line) for line in lines)} points")
    print(f"{args.output}: {len(blob)} bytes")


if __name__ == "__main__":
    main(sys.argv[1:])
