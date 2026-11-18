#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
import json,struct,sys,tempfile
from pathlib import Path
import numpy as np
from sidecars import generate
from review_instruments import feature_metrics
from oracle_rows import expand


def serialize(path,features):
    data=bytearray(b'PEAAAB1\0'+struct.pack('<I',len(features)))
    for identity,color,chains in features:
        data.extend(struct.pack('<IHI',identity,color,len(chains)))
        for p,closed in chains:
            data.extend(struct.pack('<BI',int(closed),len(p)))
            for x,y in p:data.extend(struct.pack('<dd',x,y))
    path.write_bytes(data)


def run(driver):
    with tempfile.TemporaryDirectory(prefix='aa-sidecar-synthetic-') as directory:
        root=Path(directory);g=root/'synthetic.geometry';bg=root/'background.rgb565'
        features=[(0,0xffff,[([(1.5,2.5),(5.5,2.5)],False)]),
                  (1,0xf800,[([(4.5,4.5),(6.5,4.5)],False)])]
        serialize(g,features);np.zeros((8,8),dtype='<u2').tofile(bg)
        # Synthetic-only margin .1 is not approval of any geographic margin.
        generate(g,bg,driver,root/'sides',8,8,.1)
        a=np.load(root/'sides/feature0000.npz');b=np.load(root/'sides/feature0001.npz')
        assert a['candidate_a'][2,3]==a['candidate_b'][2,3]==0xffff
        assert b['candidate_a'][4,5]==b['candidate_b'][4,5]==0xf800
        assert a['candidate_a'][4,5]==a['candidate_b'][4,5]==0
        assert b['candidate_a'][2,3]==b['candidate_b'][2,3]==0
        assert a['reference'][2,1,0]==.5
        assert a['quantized'][2,1]==0x8410
        m=feature_metrics(a['candidate_b'],a['reference'],a['background'],a['source'],a['support'],a['probes'])
        assert m['peak_retention']==1 and m['visibility_fraction']==1
        assert a['probes'].shape==(1,2)
        assert len(json.loads((root/'sides/manifest.json').read_text()))==2
        # The reader must reject malformed/truncated input without image output.
        import subprocess
        g.write_bytes(g.read_bytes()[:-1])
        r=subprocess.run([str(driver),str(g),str(bg),'8','8','0','B'],stdout=subprocess.PIPE)
        assert r.returncode and r.stdout==b''
    print('PASS isolated A/B and independent reference sidecars; other-feature exclusion, analytic cap, probes, metric integration and malformed-input failure')

if __name__=='__main__':run(Path(sys.argv[1]))
