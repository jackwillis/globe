# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Full settled-frame pin, independently reproduced by the G4 benchmark."""
import tempfile
import json
from pathlib import Path
import subprocess
import sys
result=subprocess.run(sys.argv[1:],capture_output=True,text=True,check=True)
print(result.stdout,end='')
stats=json.loads(result.stdout)
expected=(Path(__file__).parent/'golden/refine-checksum.txt').read_text().strip()
assert stats['frame_checksum']==expected,stats
assert stats['refine_steps']>0 and stats['refine_ms']>0,stats
assert 0<stats['terrain_cache_bytes']<=32*1024*1024,stats

# Changing the default look must not alter the existing Hypsometric terrain path.
with tempfile.NamedTemporaryFile(mode='w',suffix='.trace') as trace:
    trace.write(Path(sys.argv[-1]).read_text()+'\nstyle 8000\nstyle 8001\n');trace.flush()
    legacy=json.loads(subprocess.run(sys.argv[1:-1]+[trace.name],capture_output=True,text=True,check=True).stdout)
    assert legacy['frame_checksum']=='efa030af1a10f887',legacy
