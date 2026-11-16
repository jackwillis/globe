#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Build and independently read back the Classic HFS view of a hybrid CD."""
import argparse
from datetime import datetime, timezone
import os
from pathlib import Path
import struct
import subprocess
import tempfile

from floppy_report import macbinary
from macbinary_rename import MAC_EPOCH_OFFSET, crc16_xmodem, rename


APP = "Poodle Earth"
DOCUMENTS = {
    "ReadMe": Path("os9/ReadMe.txt"),
    "License (1 of 2)": Path("license/License-1.txt"),
    "License (2 of 2)": Path("license/License-2.txt"),
    "DataCredit": Path("data/DATA-CREDITS.txt"),
}
ENV = {**os.environ, "LC_ALL": "C", "TZ": "UTC"}


def run(*args, **kwargs):
    return subprocess.run(args, check=True, env=ENV, **kwargs)


def text_macbinary(name, data, epoch):
    header = bytearray(128)
    header[65:73] = b"TEXTttxt"
    struct.pack_into(">I", header, 83, len(data))
    header[122:124] = bytes((129, 129))
    struct.pack_into(">H", header, 124, crc16_xmodem(header[:124]))
    return rename(bytes(header) + data + bytes(-len(data) % 128), name, epoch)


def hybrid_partition(raw):
    if len(raw) % 2048 or raw[32768:32775] != b"\x01CD001\x01":
        raise ValueError("missing ISO-9660 primary volume descriptor")
    iso_blocks = struct.unpack_from("<I", raw, 32768 + 80)[0]
    if not 16 < iso_blocks <= len(raw) // 2048:
        raise ValueError("invalid ISO-9660 volume size")
    if raw[:2] != b"ER":
        raise ValueError("missing Apple driver descriptor")
    block = struct.unpack_from(">H", raw, 2)[0]
    if block not in (512, 2048) or raw[block:block + 2] != b"PM":
        raise ValueError("invalid Apple Partition Map block size or signature")
    count = struct.unpack_from(">I", raw, block + 4)[0]
    if not 1 <= count < 32768 // block:
        raise ValueError("invalid Apple Partition Map entry count")
    for index in range(1, count + 1):
        offset = block * index
        entry = raw[offset:offset + block]
        if entry[:2] != b"PM":
            raise ValueError("invalid Apple Partition Map entry")
        if entry[48:80].split(b"\0", 1)[0] == b"Apple_HFS":
            start, length = struct.unpack_from(">II", entry, 8)
            start, end = start * block, (start + length) * block
            if end > len(raw) or end - start < 1536 or raw[start + 1024:start + 1026] != b"BD":
                raise ValueError("invalid classic HFS partition")
            return raw[start:end]
    raise ValueError("missing Apple_HFS partition")


def check_application(actual, original):
    files = macbinary(actual)
    if (actual[65:73] != b"APPLPdlE" or original[65:73] != actual[65:73]
            or files != macbinary(original) or files["name"] != APP):
        raise ValueError("application name, type/creator or forks differ")


def verify(image, original, documents, work):
    # Read the HFS partition with hfsutils, independently of cdrkit's writer.
    partition = work / "verify.hfs"
    partition.write_bytes(hybrid_partition(image.read_bytes()))
    run("hmount", partition, stdout=subprocess.DEVNULL)
    try:
        listing = run("hls", "-1a", ":", capture_output=True, text=True).stdout.splitlines()
        expected = {APP, *documents}
        if set(listing) != expected:
            raise ValueError(f"unexpected HFS root entries: {listing}")
        extracted = work / "extracted.bin"
        run("hcopy", "-m", f":{APP}", extracted)
        check_application(extracted.read_bytes(), original)
        for name, data in documents.items():
            run("hcopy", "-m", f":{name}", extracted)
            raw = extracted.read_bytes()
            size, resource_size = struct.unpack_from(">II", raw, 83)
            if raw[65:73] != b"TEXTttxt" or resource_size or raw[128:128 + size] != data:
                raise ValueError(f"HFS document content or metadata differ: {name}")
    finally:
        run("humount")


def package(build, output, epoch, verify_only):
    original = (build / "PoodleEarth-Finder.bin").read_bytes()
    check_application(original, original)
    documents = {
        name: (build / path if path.parts[0] == "license" else path).read_bytes().replace(b"\n", b"\r")
        for name, path in DOCUMENTS.items()
    }
    with tempfile.TemporaryDirectory(dir=output.parent) as directory:
        work = Path(directory)
        if verify_only:
            verify(output, original, documents, work)
            return
        staging = work / "staging"
        staging.mkdir()
        (staging / "PoodleEarth.bin").write_bytes(rename(original, APP, epoch))
        for index, (name, data) in enumerate(documents.items()):
            (staging / f"document{index}.bin").write_bytes(text_macbinary(name, data, epoch))
        for path in (*staging.iterdir(), staging):
            os.utime(path, (epoch, epoch))
        stamp = datetime.fromtimestamp(epoch, timezone.utc).strftime("%Y-%m-%d %H:%M:%S")
        image = work / "candidate.iso"
        run("faketime", "-f", stamp, "genisoimage", "-hfs", "-part", "--macbin", "-mac-name",
            "-no-desktop",
            "-V", "POODLE_EARTH", "-hfs-volid", APP, "-o", image, staging)
        verify(image, original, documents, work)
        image.replace(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("build", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("epoch", type=int)
    args = parser.parse_args()
    if not 0 <= args.epoch <= 0xffffffff - MAC_EPOCH_OFFSET:
        parser.error("epoch outside the supported Classic HFS date range")
    try:
        package(args.build, args.output, args.epoch, args.verify_only)
    except (OSError, ValueError, struct.error, subprocess.CalledProcessError) as error:
        parser.exit(1, f"CD packaging: {error}\n")
    print(f"HFS filenames, documents, application forks and type/creator verified: {args.output}")


if __name__ == "__main__":
    main()
