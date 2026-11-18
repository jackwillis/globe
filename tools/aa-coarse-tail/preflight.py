#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,hashlib,zipfile,gzip,time,sys
from pathlib import Path
from admission import validate
from accounting import initialize
start=time.process_time();out=Path(sys.argv[1]);archive=Path('/tmp/PE-AA-01-coarse-full-fixture-feasibility-evidence.zip');expected='d198144721dc036020ac0332c25499eececb312e33fce76fd88d7eb58555649b'
assert hashlib.sha256(archive.read_bytes()).hexdigest()==expected
old=Path('/tmp/aa-full-coarse-evidence');records=[json.loads(x) for x in (old/'completed.jsonl').read_text().splitlines()];assert len(records)==4138
with zipfile.ZipFile(archive) as z:
 hashes=json.loads(z.read('evidence-hashes.json'))
 for n,h in hashes.items():assert hashlib.sha256(z.read(n)).hexdigest()==h,n
 for i,rec in enumerate(records):
  assert rec['index']==i
  path=old/'tiles'/f'{i:04d}.json.gz';raw=path.read_bytes();assert hashlib.sha256(raw).hexdigest()==rec['sha256']==hashes[f'tiles/{i:04d}.json.gz'];validate(json.loads(gzip.decompress(raw)),i)
(out/'recovery.json').write_text(json.dumps({'archive_sha256':expected,'archive_entries_verified':len(hashes),'verified_prefix':4138,'exact_pixels':4138*64,'original_manifest_sha256':hashlib.sha256((old/'completed.jsonl').read_bytes()).hexdigest(),'prefix_hashes':{str(r['index']):r['sha256'] for r in records},'cpu_seconds':time.process_time()-start},indent=2))
l=initialize(out/'ledger.json');l.measured('preparation',time.process_time()-start)
print('archive and4138 certificates verified')
