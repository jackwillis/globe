#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Release manifest tests, standard library only."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("release_manifest", ROOT / "tools" / "release_manifest.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ManifestTest(unittest.TestCase):
    def test_cmake_version(self):
        self.assertRegex(module.cmake_version(ROOT / "CMakeLists.txt"), r"^\d+\.\d+\.\d+$")

    def test_manifest_hashes_and_order(self):
        with tempfile.TemporaryDirectory() as tmp:
            folder = Path(tmp)
            b = folder / "b.bin"
            a = folder / "a.sit"
            b.write_bytes(b"test")
            a.write_bytes(b"archive")
            kwargs = dict(
                revision="a" * 40, epoch=1700000000, version="0.0.1",
                retro68_image="example@sha256:123", rust_image="other:slim",
                paths=[b, a],
            )
            manifest = module.make_manifest(**kwargs)
            self.assertEqual([x["filename"] for x in manifest["artifacts"]], ["a.sit", "b.bin"])
            self.assertFalse(manifest["toolchain_images_digest_pinned"])
            self.assertEqual(manifest, module.make_manifest(**kwargs))
            b.write_bytes(b"changed")
            self.assertNotEqual(
                manifest["artifacts"][1]["sha256"], module.make_manifest(**kwargs)["artifacts"][1]["sha256"]
            )

    def test_reject_duplicates_and_missing(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "missing.bin"
            args = dict(
                revision="a" * 40, epoch=1, version="0.0.1",
                retro68_image="r", rust_image="s",
            )
            with self.assertRaises(ValueError):
                module.make_manifest(paths=[path], **args)
            path.write_bytes(b"x")
            with self.assertRaises(ValueError):
                module.make_manifest(paths=[path, path], **args)
            with self.assertRaises(ValueError):
                module.make_manifest(paths=[path], **{**args, "revision": "bad"})


if __name__ == "__main__":
    unittest.main()
