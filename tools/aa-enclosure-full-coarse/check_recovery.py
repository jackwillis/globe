#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Admission mutations on a completed certificate, no reference computation."""
import sys,json,gzip,copy,hashlib,time
from pathlib import Path
from analyze import validate
root=Path(sys.argv[1]);raw=(root/'tiles/0000.json.gz').read_bytes();r=json.loads(gzip.decompress(raw));validate(r,0);checks=[]
for name,change in [('partial_63_pixels',lambda a:a['pixels'].pop()),('inflight_flag',lambda a:a.update(complete=False)),('wrong_tile_identity',lambda a:a.update(tile_index=1)),('false_precision_claim',lambda a:a['pixels'][0].update(precision_reached=False)),('invalid_enclosure',lambda a:a['pixels'][0]['upper_numerator'].__setitem__(0,-1))]:
 a=copy.deepcopy(r);change(a)
 try:validate(a,0)
 except AssertionError:checks.append({'mutation':name,'rejected':True})
 else:raise AssertionError('admission mutant survived '+name)
rec=json.loads((root/'completed.jsonl').read_text().splitlines()[0]);assert hashlib.sha256(raw).hexdigest()==rec['sha256'];assert hashlib.sha256(raw+b'x').hexdigest()!=rec['sha256']
(root/'recovery-controls.json').write_text(json.dumps({'controls':checks,'changed_bytes_hash_rejected':True,'temporary_suffix_not_a_completed_tile':not Path('0000.json.gz.tmp').match('*.json.gz'),'cpu_seconds':time.process_time()},indent=2))
