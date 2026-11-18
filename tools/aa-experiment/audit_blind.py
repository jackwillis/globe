#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import csv,json,re,struct,sys
from pathlib import Path


def audit(root):
    root=Path(root);names={p.name for p in root.iterdir()}
    expected={f'pair{i:02d}-{side}.png' for i in range(1,33) for side in ('left','right')}
    assert names==expected|{'review.html','judgments.csv'}
    for name in expected:
        data=(root/name).read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n'
        pos=8;kinds=[]
        while pos<len(data):
            n=struct.unpack_from('>I',data,pos)[0];kind=data[pos+4:pos+8];kinds.append(kind)
            if kind==b'IHDR':assert struct.unpack_from('>II',data,pos+8)==(640,480)
            pos+=n+12
        assert kinds==[b'IHDR',b'IDAT',b'IEND'] and pos==len(data) # no metadata identity channels
    page=(root/'review.html').read_text()
    assert set(re.findall(r'src="([^"]+)"',page))==expected
    assert not re.search(r'PRIVATE|identity|\.rgb565|seed|\barm\b',page,re.I)
    with (root/'judgments.csv').open() as f: rows=list(csv.reader(f))
    assert len(rows)==33 and len(rows[0])==8
    assert all(len(r)==8 and r[0]==f'pair{i:02d}' and not any(r[1:]) for i,r in enumerate(rows[1:],1))
    print(json.dumps(dict(png_count=64,dimensions=[640,480],neutral_names=True,identity_material=False,
                         metadata_chunks=False,html_images=64,judgment_rows=32,judgment_columns=8)))

if __name__=='__main__':audit(sys.argv[1])
