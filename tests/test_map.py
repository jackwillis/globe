"""Exercise the actual runtime parser with valid and adversarial byte streams."""
import os
from pathlib import Path
import struct
import subprocess
import tempfile

source = Path('data/coastline.plnt').read_bytes()

def header(lines, points, endian='>'):
    return b'PLNTMAP\0' + struct.pack(endian + 'III', 1, lines, points)

valid = header(1, 2) + struct.pack('>IIIII', 2, 180000000, 90000000, 181000000, 90000000)
invalid = [b'', source[:7], source[:19], source[:-1], source + b'x',
           header(100001, 200002), header(1, 1000001), header(1, 1),
           header(1, 2, '<') + valid[20:],
           header(1, 2) + struct.pack('>I', 0),
           header(1, 2) + struct.pack('>IIIII', 2, 360000001, 0, 0, 0),
           header(1, 2) + struct.pack('>IIIII', 2, 0, 180000001, 0, 0)]
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'test.plnt'
    output = Path(directory) / 'test.ppm'
    for data, expected in [(valid, 0), (source, 0)] + [(x, 1) for x in invalid]:
        path.write_bytes(data)
        result = subprocess.run([os.environ.get('PLANET', './planet'), str(output), '0', '0', str(path)],
                                capture_output=True, timeout=10)
        assert result.returncode == expected, (len(data), result.stderr)
print('map validation tests passed')
