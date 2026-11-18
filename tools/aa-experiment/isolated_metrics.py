#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Future authorized isolated-feature diagnostics from sidecars; no rendering or timing."""
import json,sys
from pathlib import Path
import numpy as np
from review_instruments import feature_metrics
root,out=map(Path,sys.argv[1:]);records=[]
for row in json.loads((root/'manifest.json').read_text()):
    with np.load(root/row['file']) as f:
        metrics={arm:feature_metrics(f['candidate_'+arm.lower()],f['reference'],f['background'],
                                    f['source'],f['support'],f['probes']) for arm in ('A','B')}
    records.append(dict(index=row['index'],identity=row['identity'],metrics=metrics))
out.write_text(json.dumps(records,indent=2,allow_nan=False)+'\n')
