"""Offline asset reproducibility and adversarial runtime parser checks."""
import os
from pathlib import Path
import struct
import subprocess
import tempfile


def header(count=1, date=b'2026-11-01', endian='>'):
    return b'PLNTPOI\0' + struct.pack(endian + 'II', 1, count) + date


def record(qid=90, lon=180000000, label=b'Paris', url=b'https://en.wikipedia.org/wiki/Paris'):
    fields = [label, b'City', b'1,000 (2020)', url]
    return struct.pack('>IIII', qid, 1, lon, 90000000) + b''.join(struct.pack('>H', len(s)) + s for s in fields)


source = Path('data/places.plnt').read_bytes()
valid = header() + record()
bad = [b'', source[:7], source[:25], source[:-1], source + b'x', header(0), header(1025),
       header(endian='<') + record(), header() + record(qid=0), header() + record(lon=360000001),
       header() + record(label=b''), header() + record(label=b'x' * 64),
       header() + record(label=b'Par\x00is'), header() + record(url=b'javascript:alert(1)'),
       header() + record(url=b'https://unrelated.example/wiki/Paris'),
       header(2) + record() + record(), header(date=b'2025-02-29') + record(),
       header(date=b'2026-13-01') + record()]
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'places.plnt'
    for data, expected in [(valid, 0), (source, 0), (header(date=b'2024-02-29') + record(), 0)] + [(x, 1) for x in bad]:
        path.write_bytes(data)
        result = subprocess.run([os.environ.get('PLACE_CHECK', './test_places'), str(path)], capture_output=True, timeout=10)
        assert result.returncode == expected, (len(data), result.stderr)
    subprocess.run(['python3', 'tools/prepare_places.py', '--output', str(path)], check=True, timeout=10)
    assert path.read_bytes() == source, 'offline rebuild changed prepared bytes'
print('place parser adversarial and offline reproducibility checks passed')
