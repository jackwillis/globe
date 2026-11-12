#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Generate a deterministic manifest for committed Poodle Earth release artifacts.

Only summarizes locally verified files; never contacts the network or publishes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re


def cmake_version(path: Path) -> str:
    match = re.search(
        r"project\(globe\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\b",
        path.read_text(encoding="utf-8"),
    )
    if not match:
        raise ValueError("missing project(globe VERSION ...) in CMakeLists.txt")
    return match.group(1)


def digest(path: Path) -> str:
    checksum = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            checksum.update(chunk)
    return checksum.hexdigest()


def is_image_digest_pinned(image: str) -> bool:
    return bool(re.fullmatch(r".+@sha256:[0-9a-f]{64}", image))


def make_manifest(
    *,
    revision: str,
    epoch: int,
    version: str,
    retro68_image: str,
    rust_image: str,
    paths: list[Path],
) -> dict:
    if not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise ValueError("revision must be a full Git SHA-1")
    if epoch < 0:
        raise ValueError("negative source date epoch")
    if not paths or len({path.name for path in paths}) != len(paths):
        raise ValueError("artifact list is empty or has duplicate filenames")
    artifacts = []
    for path in sorted(paths, key=lambda value: value.name):
        if not path.is_file():
            raise ValueError(f"missing artifact: {path}")
        artifacts.append({
            "filename": path.name,
            "bytes": path.stat().st_size,
            "sha256": digest(path),
        })
    return {
        "schema": 1,
        "project": "Poodle Earth",
        "version": version,
        "source_revision": revision,
        "source_date_epoch": epoch,
        "toolchain_images": {
            "retro68": retro68_image,
            "rust": rust_image,
        },
        "toolchain_images_digest_pinned": all(
            is_image_digest_pinned(value) for value in (retro68_image, rust_image)
        ),
        "notes": [
            "Source archive is created from the committed revision.",
            "The HFS disk image carries volume timestamps, so it is not byte-reproducible; the application forks inside it are.",
            "Toolchain versions are not proven reproducible unless pinned and verified.",
            "This manifest records artifact hashes, not OS 9 execution or acceptance.",
        ],
        "artifacts": artifacts,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--revision", required=True)
    parser.add_argument("--epoch", type=int, required=True)
    parser.add_argument("--retro68-image", required=True)
    parser.add_argument("--rust-image", required=True)
    parser.add_argument("--cmake", type=Path, default=Path("CMakeLists.txt"))
    parser.add_argument("artifacts", nargs="+", type=Path)
    args = parser.parse_args()
    manifest = make_manifest(
        revision=args.revision,
        epoch=args.epoch,
        version=cmake_version(args.cmake),
        retro68_image=args.retro68_image,
        rust_image=args.rust_image,
        paths=args.artifacts,
    )
    args.output.write_text(
        json.dumps(manifest, indent=2, ensure_ascii=True, sort_keys=True) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
