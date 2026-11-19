#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Stage byte-identical optional atlas files for package-layout experiments.

This is a distribution *staging* tool, not a runtime pack format, an edition
manifest specification, or a package-manager backend. Nothing is installed.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import tempfile


PREFIX = PurePosixPath("share/poodle-earth")
SCHEMA = "distribution-stage-probe-1"


def edition_name(name):
    if not re.fullmatch(r"[a-z][a-z0-9._-]{0,63}", name) or name in {".", ".."}:
        raise ValueError("edition must be a short lowercase package-safe identifier")
    return name


def input_path(raw):
    if not raw or "\\" in raw or ":" in raw:
        raise ValueError("invalid relative input path")
    path = PurePosixPath(raw)
    if path.is_absolute() or any(part in {"", ".", ".."} for part in raw.split("/")):
        raise ValueError("input path must be a normalized relative path")
    return path


def source_file(root, name):
    path = input_path(name)
    source = root.joinpath(*path.parts)
    if not source.is_file() or source.is_symlink() or not source.resolve().is_relative_to(root):
        raise ValueError(f"missing, symlinked, or escaping source file: {name}")
    return path, source


def hash_file(path):
    digest = hashlib.sha256()
    length = 0
    with path.open("rb") as stream:
        while True:
            chunk = stream.read(1024 * 1024)
            if not chunk:
                break
            length += len(chunk)
            digest.update(chunk)
    return length, digest.hexdigest()


def manifest_path(edition):
    return PREFIX / "staging" / f"{edition}.json"


def stage(source_root, output, edition, packs, notices=()):
    edition = edition_name(edition)
    source_root = Path(source_root).resolve(strict=True)
    output = Path(output)
    if output.exists() or output.is_symlink():
        raise ValueError("output must not already exist (no overwrite or stale files)")
    if not packs:
        raise ValueError("at least one pack file is required")
    names = [("pack", name) for name in packs] + [("notice", name) for name in notices]
    if len({(kind, name) for kind, name in names}) != len(names):
        raise ValueError("duplicate input")
    prepared = [(kind, *source_file(source_root, name)) for kind, name in names]
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="poodle-stage-", dir=output.parent) as temp:
        root = Path(temp) / "payload"
        root.mkdir()
        entries = []
        for kind, relative, source in sorted(prepared, key=lambda entry: (entry[0], str(entry[1]))):
            section = PREFIX / "packs" / edition if kind == "pack" else (
                PurePosixPath("share/doc") / f"poodle-earth-data-{edition}"
            )
            destination = section / relative
            target = root.joinpath(*destination.parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                raise ValueError(f"duplicate staged destination: {destination}")
            with source.open("rb") as inp, target.open("wb") as out:
                while True:
                    chunk = inp.read(1024 * 1024)
                    if not chunk:
                        break
                    out.write(chunk)
            original = hash_file(source)
            copied = hash_file(target)
            if original != copied:
                raise ValueError(f"source changed while staging: {relative}")
            entries.append({
                "kind": kind,
                "path": str(destination),
                "bytes": copied[0],
                "sha256": copied[1],
            })
        manifest = {
            "schema": SCHEMA,
            "edition": edition,
            "note": "Experimental distribution staging metadata; not a runtime format",
            "files": sorted(entries, key=lambda entry: entry["path"]),
        }
        descriptor = root.joinpath(*manifest_path(edition).parts)
        descriptor.parent.mkdir(parents=True, exist_ok=True)
        descriptor.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        verify(root, edition)
        if output.exists():
            raise ValueError("output appeared while staging")
        root.rename(output)
    return manifest


def verify(output, edition):
    edition = edition_name(edition)
    output = Path(output)
    descriptor = output.joinpath(*manifest_path(edition).parts)
    if descriptor.is_symlink() or not descriptor.is_file():
        raise ValueError("missing distribution staging manifest")
    manifest = json.loads(descriptor.read_text(encoding="utf-8"))
    if manifest.get("schema") != SCHEMA or manifest.get("edition") != edition:
        raise ValueError("incorrect staging manifest edition/schema")
    records = manifest.get("files")
    if not isinstance(records, list) or not records:
        raise ValueError("empty or invalid file inventory")
    expected = {manifest_path(edition)}
    for record in records:
        rel = input_path(record["path"])
        if rel in expected:
            raise ValueError("duplicate manifest path")
        expected.add(rel)
        candidate = output.joinpath(*rel.parts)
        if candidate.is_symlink() or not candidate.is_file():
            raise ValueError(f"missing or symlinked staged file: {rel}")
        if record.get("kind") == "pack":
            required_prefix = PREFIX / "packs" / edition
        elif record.get("kind") == "notice":
            required_prefix = PurePosixPath("share/doc") / f"poodle-earth-data-{edition}"
        else:
            raise ValueError("unknown staged file kind")
        if rel == required_prefix or required_prefix not in rel.parents:
            raise ValueError(f"file outside its edition directory: {rel}")
        if hash_file(candidate) != (record.get("bytes"), record.get("sha256")):
            raise ValueError(f"staged file checksum/size mismatch: {rel}")
    actual = {PurePosixPath(path.relative_to(output).as_posix()) for path in output.rglob("*") if path.is_file()}
    if any(path.is_symlink() for path in output.rglob("*")) or actual != expected:
        raise ValueError("unexpected, missing, or symlinked stage entries")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    create = commands.add_parser("stage", help="create fresh architecture-neutral staging tree")
    create.add_argument("--source-root", type=Path, required=True)
    create.add_argument("--output", type=Path, required=True)
    create.add_argument("--edition", required=True)
    create.add_argument("--pack", action="append", required=True)
    create.add_argument("--notice", action="append", default=[])
    check = commands.add_parser("verify", help="check byte integrity and file ownership")
    check.add_argument("--output", type=Path, required=True)
    check.add_argument("--edition", required=True)
    args = parser.parse_args()
    try:
        if args.command == "stage":
            result = stage(args.source_root, args.output, args.edition, args.pack, args.notice)
        else:
            result = verify(args.output, args.edition)
    except (ValueError, OSError, KeyError, TypeError, json.JSONDecodeError) as error:
        parser.exit(1, f"data staging: {error}\n")
    print(f"Verified {len(result['files'])} files for {result['edition']}: {args.output}")


if __name__ == "__main__":
    main()
