#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("mbr", ROOT / "tools/macbinary_rename.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


def sample(name=b"PoodleEarth"):
    h = bytearray(128)
    h[1] = len(name)
    h[2:2 + len(name)] = name
    h[65:73] = b"APPLPdlE"
    struct.pack_into(">II", h, 83, 3, 3)
    struct.pack_into(">H", h, 124, m.crc16_xmodem(bytes(h[:124])))
    return bytes(h) + b"abc" + b"\0" * 125 + b"xyz" + b"\0" * 125


raw = sample()
out = m.rename(raw, "Poodle Earth")
assert out[128:] == raw[128:] and len(out) == len(raw)
assert out[1] == 12 and out[2:14] == b"Poodle Earth" and out[14:65] == b"\0" * 51
assert out[65:73] == b"APPLPdlE" and out[83:91] == raw[83:91]
assert struct.unpack_from(">H", out, 124)[0] == m.crc16_xmodem(out[:124])
# The CRC variant: a known value for the standard check string.
assert m.crc16_xmodem(b"123456789") == 0x31C3
for bad in (b"", raw[:100], bytes([1]) + raw[1:]):
    try:
        m.rename(bad, "X")
    except ValueError:
        pass
    else:
        raise AssertionError("invalid MacBinary accepted")
for bad_name in ("", "a:b", "x" * 64):
    try:
        m.rename(raw, bad_name)
    except ValueError:
        pass
    else:
        raise AssertionError("invalid name accepted")
print("ok")
