#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Give a Retro68 MacBinary file its Finder name (CMake target names cannot contain spaces).

    macbinary_rename.py IN.bin OUT.bin "Poodle Earth"

Only the header's file name changes (and its CRC); both forks are copied unchanged.
"""
import struct
import sys


def crc16_xmodem(data: bytes) -> int:
    crc = 0
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def rename(raw: bytes, name: str) -> bytes:
    encoded = name.encode("mac_roman")
    if not 1 <= len(encoded) <= 63 or b":" in encoded:
        raise ValueError("invalid Macintosh file name")
    if len(raw) < 128 or raw[0] != 0 or raw[74] != 0:
        raise ValueError("not a MacBinary file")
    if struct.unpack_from(">H", raw, 124)[0] != crc16_xmodem(raw[:124]):
        raise ValueError("MacBinary header CRC does not match")
    header = bytearray(raw[:128])
    header[1] = len(encoded)
    header[2:65] = encoded.ljust(63, b"\0")
    struct.pack_into(">H", header, 124, crc16_xmodem(bytes(header[:124])))
    return bytes(header) + raw[128:]


def main() -> None:
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    with open(sys.argv[1], "rb") as source:
        out = rename(source.read(), sys.argv[3])
    with open(sys.argv[2], "wb") as target:
        target.write(out)


if __name__ == "__main__":
    main()
