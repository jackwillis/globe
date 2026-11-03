# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Full settled-frame pin, independently reproduced by the G4 benchmark."""
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

# Order selection goes through the same trace/model path as the interactive frontend.
# Preserve the established sampled final-frame pin for both experimental orders.
import tempfile
for order in ('center', 'dissolve'):
    original=Path(sys.argv[-1]).read_text()
    with tempfile.NamedTemporaryFile(mode='w',suffix='.trace') as trace:
        trace.write(original+'\norder 8000 '+order+'\n');trace.flush()
        replay=subprocess.run(sys.argv[1:-1]+[trace.name],capture_output=True,text=True,check=True)
        ordered=json.loads(replay.stdout)
        assert ordered['frame_checksum']==expected,(order,ordered)
