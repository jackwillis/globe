#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Validate Retro68 MacBinary fork roundtrip and designated HFS floppy payload budget."""
import argparse
import datetime
import hashlib
import json
import re
from pathlib import Path
import struct

IMAGE_BYTES = 80 * 2 * 18 * 512
PAYLOAD_BUDGET = 1_200_000  # leaves room for HFS structures, allocation rounding and documents

def macbinary(raw):
    if len(raw) < 128 or raw[0] or not 1 <= raw[1] <= 63 or raw[65:69] != b'APPL':
        raise ValueError('not the expected application MacBinary')
    data_size, resource_size = struct.unpack_from('>II', raw, 83)
    data_end = 128 + data_size
    resource_start = 128 + ((data_size + 127) // 128) * 128
    exact = resource_start + ((resource_size + 127) // 128) * 128
    if not data_size or not resource_size or len(raw) != exact:
        raise ValueError('truncated or trailing MacBinary forks')
    return {'name': raw[2:2+raw[1]].decode('mac_roman'),
            'data': raw[128:data_end],
            'resource': raw[resource_start:resource_start+resource_size]}

def sha(raw):
    return hashlib.sha256(raw).hexdigest()

def report(app_path, roundtrip_path, disk_path, documents, budget=PAYLOAD_BUDGET):
    raw = Path(app_path).read_bytes()
    app = macbinary(raw)
    copied = macbinary(Path(roundtrip_path).read_bytes())
    if app['data'] != copied['data'] or app['resource'] != copied['resource']:
        raise ValueError('HFS roundtrip changed application forks')
    disk = Path(disk_path).read_bytes()
    if len(disk) != IMAGE_BYTES or disk[1024:1026] != b'BD':
        raise ValueError('expected a raw 1.44 MB HFS image')
    docs = sum(Path(p).stat().st_size for p in documents)
    payload = len(app['data']) + len(app['resource']) + docs
    if payload > budget:
        raise ValueError('floppy payload exceeds budget: %d > %d' % (payload, budget))
    return {'edition': 'floppy: app + atlas + one-degree elevation',
            'measured_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'application': app['name'], 'data_fork_bytes': len(app['data']),
            'resource_fork_bytes': len(app['resource']), 'macbinary_bytes': len(raw),
            'documents_bytes': docs, 'payload_bytes': payload,
            'payload_budget_bytes': budget, 'payload_headroom_bytes': budget-payload,
            'image_bytes': len(disk), 'image_budget_bytes': IMAGE_BYTES,
            'app_macbinary_sha256': sha(raw), 'image_sha256': sha(disk),
            'fork_roundtrip': 'identical', 'execution': 'built and packaged; not run on OS 9',
            'limits': 'payload budget excludes HFS overhead; actual image creation and copy enforce disk fit; signature check is not a full HFS validator'}

def full_report(app_path, documents):
    raw = Path(app_path).read_bytes()
    if raw[:4] != b'\x7fELF':
        raise ValueError('full edition report expects the native X11 ELF executable')
    docs = sum(Path(p).stat().st_size for p in documents)
    return {'edition': 'full: native X11 app with embedded atlas, elevation, countries and places',
            'measured_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'application': Path(app_path).name, 'executable_bytes': len(raw),
            'documents_bytes': docs, 'payload_bytes': len(raw)+docs,
            'app_sha256': sha(raw), 'payload_budget_bytes': None,
            'execution': 'host tests run; real G4 display not run',
            'limits': 'native dynamically linked executable; system libraries and optional imported packs excluded; no floppy-fit claim'}

def free_bytes(text):
    match = re.search(r'Volume has ([0-9]+) bytes free', text)
    if not match:
        raise ValueError('cannot read HFS volume free space')
    return int(match.group(1))

def markdown(result):
    if result['edition'].startswith('full:'):
        return ('# Full edition size report\n\nNative executable: %s bytes; documents: %s bytes.\n\n%s.\n' % (format(result['executable_bytes'], ','), format(result['documents_bytes'], ','), result['limits']))
    rows = [('Application data fork', 'data_fork_bytes'),
            ('Application resource fork', 'resource_fork_bytes'),
            ('MacBinary transfer file', 'macbinary_bytes'),
            ('Documents', 'documents_bytes'), ('Payload', 'payload_bytes'),
            ('Payload budget', 'payload_budget_bytes'), ('Payload headroom', 'payload_headroom_bytes'),
            ('HFS disk image', 'image_bytes')]
    return ('# Designated floppy size report\n\n| Component | Bytes |\n| --- | ---: |\n' +
            ''.join('| %s | %s |\n' % (label, format(result[key], ',')) for label,key in rows) +
            '\nForks extracted from HFS match the application byte for byte.\n\n' + result['execution'] + '.\n')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--edition', choices=('floppy','full'), default='floppy')
    p.add_argument('--app', required=True); p.add_argument('--roundtrip')
    p.add_argument('--disk'); p.add_argument('--document', action='append', default=[])
    p.add_argument('--output', required=True)
    p.add_argument('--volume')
    args = p.parse_args()
    try:
        if args.edition == 'full':
            result = full_report(args.app, args.document)
        else:
            if not args.roundtrip or not args.disk:
                raise ValueError('floppy report requires --roundtrip and --disk')
            result = report(args.app, args.roundtrip, args.disk, args.document)
        if args.volume:
            result['image_free_bytes'] = free_bytes(Path(args.volume).read_text())
    except (OSError, ValueError) as error:
        p.exit(1, str(error)+'\n')
    output = Path(args.output)
    output.write_text(json.dumps(result, indent=2)+'\n')
    output.with_suffix('.md').write_text(markdown(result))
    print(markdown(result))

if __name__ == '__main__':
    main()
