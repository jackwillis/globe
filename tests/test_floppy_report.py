#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import importlib.util
from pathlib import Path
import struct
import tempfile
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('report',ROOT/'tools/floppy_report.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def app(data=b'payload',resource=b'resource'):
    h=bytearray(128);h[1]=5;h[2:7]=b'Globe';h[65:69]=b'APPL'
    struct.pack_into('>II',h,83,len(data),len(resource))
    return bytes(h)+data+b'\0'*((-len(data))%128)+resource+b'\0'*((-len(resource))%128)
assert m.macbinary(app())['data']==b'payload'
assert m.macbinary(app())['resource']==b'resource'
for bad in [b'',app()[:127],app()[:-1],app()+b'\0']:
    try:m.macbinary(bad)
    except ValueError:pass
    else:raise AssertionError('invalid MacBinary accepted')
with tempfile.TemporaryDirectory() as temp:
    p=Path(temp);(p/'app.bin').write_bytes(app());(p/'round.bin').write_bytes(app())
    disk=bytearray(1474560);disk[1024:1026]=b'BD';(p/'disk.dsk').write_bytes(disk)
    (p/'readme').write_text('Read me\n');(p/'license').write_text('License\n')
    result=m.report(p/'app.bin',p/'round.bin',p/'disk.dsk',[p/'readme',p/'license'])
    assert result['image_bytes']==1474560 and result['payload_bytes']==31
    (p/'full').write_bytes(b'\x7fELFtest')
    full=m.full_report(p/'full',[p/'readme'])
    assert full['executable_bytes']==8 and full['payload_budget_bytes'] is None
    for mutate in ['overbudget','roundtrip','geometry','signature']:
        try:
            if mutate=='overbudget':m.report(p/'app.bin',p/'round.bin',p/'disk.dsk',[],budget=1)
            elif mutate=='roundtrip':
                (p/'round.bin').write_bytes(app(b'changed'));m.report(p/'app.bin',p/'round.bin',p/'disk.dsk',[])
            elif mutate=='geometry':
                (p/'disk.dsk').write_bytes(disk[:-1]);m.report(p/'app.bin',p/'app.bin',p/'disk.dsk',[])
            else:
                disk[1024:1026]=b'??';(p/'disk.dsk').write_bytes(disk);m.report(p/'app.bin',p/'app.bin',p/'disk.dsk',[])
        except ValueError:pass
        else:raise AssertionError(mutate+' accepted')
assert m.free_bytes('Volume has 1178624 bytes free')==1178624
print('ok')
