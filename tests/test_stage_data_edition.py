#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Tests for architecture-independent atlas payload staging (standard library only)."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("stage_data_edition", ROOT / "tools/stage_data_edition.py")
stage_tool = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stage_tool)


class StageDataEditionTest(unittest.TestCase):
    def make_source(self, root):
        source = root / "source"
        source.mkdir()
        (source / "atlas.gatl").write_bytes(b"GATL\x01\x00 test pack bytes")
        (source / "states").mkdir()
        (source / "states" / "wisconsin.gpol").write_bytes(b"GPOL\x01\x00 states")
        (source / "DATA-CREDITS.txt").write_text("Example provenance\n", encoding="utf-8")
        return source

    def test_reproducible_and_exact_bytes(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = self.make_source(root)
            packs = ["states/wisconsin.gpol", "atlas.gatl"]
            first = root / "first"
            second = root / "second"
            kwargs = {"edition": "world-probe", "packs": packs, "notices": ["DATA-CREDITS.txt"]}
            a = stage_tool.stage(source, first, **kwargs)
            b = stage_tool.stage(source, second, **kwargs)
            self.assertEqual(a, b)
            self.assertEqual(stage_tool.verify(first, "world-probe"), a)
            self.assertEqual(stage_tool.verify(second, "world-probe"), b)
            self.assertEqual(
                (first / "share/poodle-earth/packs/world-probe/atlas.gatl").read_bytes(),
                (source / "atlas.gatl").read_bytes(),
            )
            self.assertEqual(
                (first / "share/doc/poodle-earth-data-world-probe/DATA-CREDITS.txt").read_bytes(),
                (source / "DATA-CREDITS.txt").read_bytes(),
            )
            self.assertEqual(
                (first / "share/poodle-earth/staging/world-probe.json").read_bytes(),
                (second / "share/poodle-earth/staging/world-probe.json").read_bytes(),
            )
            self.assertEqual(len(a["files"]), 3)
            with self.assertRaisesRegex(ValueError, "overwrite"):
                stage_tool.stage(source, first, **kwargs)

    def test_reject_invalid_names_and_paths(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = self.make_source(root)
            for invalid in ("../escape", "/absolute", "a//b", "a/./b", "C:/windows", "a\\b"):
                with self.subTest(path=invalid), self.assertRaises(ValueError):
                    stage_tool.stage(source, root / "bad", "world-probe", [invalid])
            for invalid in ("../world", "a/b", "WORLD", "..", ""):
                with self.subTest(edition=invalid), self.assertRaises(ValueError):
                    stage_tool.stage(source, root / "bad", invalid, ["atlas.gatl"])
            with self.assertRaisesRegex(ValueError, "duplicate"):
                stage_tool.stage(source, root / "bad", "world-probe", ["atlas.gatl", "atlas.gatl"])
            with self.assertRaisesRegex(ValueError, "at least one"):
                stage_tool.stage(source, root / "bad", "world-probe", [])

    def test_reject_symlinked_source(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = self.make_source(root)
            (source / "alias.gatl").symlink_to(source / "atlas.gatl")
            with self.assertRaisesRegex(ValueError, "symlinked"):
                stage_tool.stage(source, root / "out", "world-probe", ["alias.gatl"])

    def test_detect_corrupt_missing_or_unowned_files(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = self.make_source(root)
            out = root / "out"
            stage_tool.stage(source, out, "world-probe", ["atlas.gatl"])
            file = out / "share/poodle-earth/packs/world-probe/atlas.gatl"
            original = file.read_bytes()
            file.write_bytes(b"tampered")
            with self.assertRaisesRegex(ValueError, "checksum/size"):
                stage_tool.verify(out, "world-probe")
            file.write_bytes(original)
            self.assertEqual(len(stage_tool.verify(out, "world-probe")["files"]), 1)
            extra = out / "share/poodle-earth/packs/world-probe/extra.gatl"
            extra.write_bytes(b"unknown")
            with self.assertRaisesRegex(ValueError, "unexpected"):
                stage_tool.verify(out, "world-probe")
            extra.unlink()
            file.unlink()
            with self.assertRaisesRegex(ValueError, "missing"):
                stage_tool.verify(out, "world-probe")

    def test_reject_manifest_traversal(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = self.make_source(root)
            out = root / "out"
            stage_tool.stage(source, out, "world-probe", ["atlas.gatl"])
            path = out / "share/poodle-earth/staging/world-probe.json"
            data = json.loads(path.read_text())
            data["files"][0]["path"] = "../outside"
            path.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaises(ValueError):
                stage_tool.verify(out, "world-probe")


if __name__ == "__main__":
    unittest.main()
