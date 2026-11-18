#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Native-size diagnostics and fixture hashes; no quality metrics or timings."""
import argparse
import hashlib
import html
import json
import random
import struct
import zlib
from pathlib import Path
from reference import read_geometry


def png565(path, width=640, height=480):
    data = path.read_bytes()
    if len(data) != width * height * 2:
        raise ValueError("frame size")
    scan = bytearray()
    values = struct.unpack("<" + "H" * (width * height), data)
    for y in range(height):
        scan.append(0)
        for value in values[y * width:(y + 1) * width]:
            r, g, b = (value >> 11) & 31, (value >> 5) & 63, value & 31
            scan.extend(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)))
    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
            + chunk(b"IDAT", zlib.compress(scan)) + chunk(b"IEND", b""))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("repo", type=Path)
    args = parser.parse_args()
    root = args.directory
    rows = [json.loads(s) for s in (root / "verification.jsonl").read_text().splitlines()]
    if len(rows) != 32 or not all(r["A_replay_equal"] for r in rows):
        raise ValueError("fixture verification incomplete")
    rng = random.Random(20261118)
    identity = {}
    page = ["<!doctype html><meta charset=utf-8><title>PE-AA-01 blind native-size diagnostics</title>",
            "<h1>Native-size diagnostics</h1><p>View at 100% browser zoom. Labels conceal arm identity. "
            "Record observations before opening PRIVATE-identity.json. No visual verdict or scored benchmark has run.</p>"]
    geometry_counts = []
    for row in rows:
        stem = row["fixture"]
        parsed = read_geometry(root / (stem + ".geometry"))
        assert len(parsed) == row["features"]
        assert sum(len(c) for _, _, c in parsed) == row["chains"]
        geometry_counts.append({"fixture": stem, "parse_verified": True})
        arms = ["A", "B"]
        rng.shuffle(arms)
        for label, arm in zip(["X", "Y"], arms):
            name = stem + "-" + label + ".png"
            (root / name).write_bytes(png565(root / (stem + "-" + arm + ".rgb565")))
            identity[name] = arm
            page.append(f"<h2>{html.escape(stem)} — {label}</h2><img width=640 height=480 src='{name}' alt='{label}'>")
    (root / "PRIVATE-identity.json").write_text(json.dumps(identity, indent=2) + "\n")
    (root / "review.html").write_text("\n".join(page) + "\n")
    hashes = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(root.iterdir()) if p.is_file() and p.name != "manifest.json"}
    sources = {p.relative_to(args.repo).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
               for p in sorted((args.repo / "tools/aa-experiment").iterdir()) if p.is_file()}
    manifest = {"schema": 1, "baseline": "00be059502db996661fdd7a9f22d366bcef87ed8",
                "fixtures": rows, "geometry_parse": geometry_counts,
                "files_sha256": hashes, "sources_sha256": sources,
                "image_label_seed": 20261118, "quality_scores_run": False, "performance_measurements_run": False}
    (root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print("PASS: 32 bounded serialized fixtures, 64 native 640x480 PNGs, blind review page and hashes")


if __name__ == "__main__":
    main()
