#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Create review-only PNG gallery; identity stays in a separate private path.
No source/raw output, A/B filenames, seed, identity key or manifests copied into review package.
"""
import argparse, json, secrets
from pathlib import Path
from images import png565
p=argparse.ArgumentParser();p.add_argument('fixtures',type=Path);p.add_argument('review',type=Path);p.add_argument('private_key',type=Path)
a=p.parse_args()
if a.review.exists() or a.private_key.exists(): raise ValueError('use fresh destinations')
if a.review.resolve() in a.private_key.resolve().parents: raise ValueError('key must be outside review tree')
a.review.mkdir(parents=True);a.private_key.parent.mkdir(parents=True,exist_ok=True)
rows=[json.loads(s) for s in (a.fixtures/'verification.jsonl').read_text().splitlines()]
if len(rows)!=32 or not all(r['A_replay_equal'] for r in rows): raise ValueError('fixture admission')
secrets.SystemRandom().shuffle(rows)
key={};page=['<!doctype html><meta charset=utf-8><title>Native-size line review</title>',
'<h1>Native-size line review</h1><p>Use 100% browser zoom. Images are 640x480. Record preference, crispness, continuity, joins, halos and thin-feature visibility for each pair before identities are revealed. Enlargements are diagnostic only. Left/right identities vary independently.</p>']
judgments=['pair,preference(left/right/tie),crispness,continuity,joins,halos,thin_features,notes']
for i,row in enumerate(rows,1):
    pair=f'pair{i:02d}';arms=['A','B'];secrets.SystemRandom().shuffle(arms)
    page.append(f'<h2>{pair}</h2>')
    for label,arm in zip(('left','right'),arms):
        filename=f'{pair}-{label}.png'
        (a.review/filename).write_bytes(png565(a.fixtures/(row['fixture']+'-'+arm+'.rgb565')))
        key[filename]=dict(fixture=row['fixture'],arm=arm)
        page.append(f'<h3>{label}</h3><img width="640" height="480" src="{filename}" alt="{pair} {label}">')
    judgments.append(pair+',,,,,,,,')
(a.review/'review.html').write_text('\n'.join(page)+'\n')
(a.review/'judgments.csv').write_text('\n'.join(judgments)+'\n')
a.private_key.write_text(json.dumps(key,indent=2)+'\n');a.private_key.chmod(0o600)
assert len(list(a.review.glob('*.png')))==64
assert all(p.suffix in ('.png','.html','.csv') for p in a.review.iterdir())
print('PASS: 64 neutral PNGs, gallery and blank judgments only; identity outside package')
