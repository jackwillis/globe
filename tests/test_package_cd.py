#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Exercise the hybrid writer and verifier with synthetic Macintosh forks."""
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from macbinary_rename import crc16_xmodem
from package_cd import APP, hybrid_partition, package, text_macbinary


@unittest.skipUnless(
    all(shutil.which(tool) for tool in ("faketime", "genisoimage", "hmount", "hls", "hcopy", "humount")),
    "requires the Linux CD toolchain",
)
class PackageCDTest(unittest.TestCase):
    def test_roundtrip_and_rejection(self):
        epoch = 1794632136
        data, resource = b"unique application data", b"unique resource fork"
        raw = bytearray(text_macbinary(APP, data, epoch))
        raw[65:73] = b"APPLPdlE"
        struct.pack_into(">I", raw, 87, len(resource))
        struct.pack_into(">H", raw, 124, crc16_xmodem(raw[:124]))
        raw += resource + bytes(-len(resource) % 128)
        with tempfile.TemporaryDirectory() as directory:
            build = Path(directory)
            (build / "PoodleEarth-Finder.bin").write_bytes(raw)
            (build / "license").mkdir()
            for part in (1, 2):
                (build / "license" / f"License-{part}.txt").write_bytes(f"Part {part}\n".encode())
            image, second = build / "first.iso", build / "second.iso"
            package(build, image, epoch, False)
            time.sleep(1.1)  # Expose any accidental use of whole-second wall time.
            package(build, second, epoch, False)
            original = image.read_bytes()
            rebuilt = second.read_bytes()
            self.assertEqual(len(original), len(rebuilt))
            if original != rebuilt:
                offsets = [i for i, (a, b) in enumerate(zip(original, rebuilt)) if a != b]
                self.fail(f"{len(offsets)} different bytes; first offsets: {offsets[:60]}")
            partition = hybrid_partition(original)
            for label, before, after in (
                ("missing document", b"DataCredit", b"LostCredit"),
                ("resource fork", resource, bytes(len(resource))),
                ("Finder creator", b"APPLPdlE", b"APPLBAD!"),
            ):
                with self.subTest(label=label):
                    self.assertIn(before, partition)
                    offset = original.index(partition)
                    second.write_bytes(original[:offset] + partition.replace(before, after)
                                       + original[offset + len(partition):])
                    with self.assertRaises((ValueError, subprocess.CalledProcessError)):
                        package(build, second, epoch, True)
            second.write_bytes(original[:32768] + b"\0" + original[32769:])
            with self.assertRaisesRegex(ValueError, "primary volume descriptor"):
                package(build, second, epoch, True)


if __name__ == "__main__":
    unittest.main()
