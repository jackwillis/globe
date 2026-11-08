#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Rebuild/verify retained raster sources in an output directory; never changes production data."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

from audit import report
from baseline import ROOT


def digest(path):
    value=hashlib.sha256()
    with path.open('rb') as file:
        for chunk in iter(lambda:file.read(1024*1024),b''):
            value.update(chunk)
    return value.hexdigest()


def analyze(output,etopo,ecoregions,lakes,python,run=False):
    output.mkdir(parents=True,exist_ok=True)
    if (ROOT/'data').resolve() in output.resolve().parents or output.resolve()==(ROOT/'data').resolve():
        raise ValueError('output must not be the production data directory')
    recipes=[('landclasses','.ggrd','landclasses-1deg.ggrd',None),
             ('elevation-1deg','.gelv','elevation-1deg.gelv',1),
             ('elevation-05deg','.gelv','elevation-0.5deg.gelv',0.5),
             ('elevation-025deg','.gelv',None,0.25)]
    results=[]
    for name,suffix,production,degrees in recipes:
        commands=[]
        builds=[]
        for iteration in ('a','b'):
            path=output/(name+'-'+iteration+suffix)
            if degrees is None:
                command=[str(python),str(ROOT/'tools/landclass-builder/build_landclasses.py'),str(ecoregions),str(path)]
            else:
                command=[str(python),str(ROOT/'tools/elevation-builder/build_elevation.py'),str(etopo),str(path),'--cell-degrees',str(degrees),'--lakes',str(lakes)]
            commands.append(command)
            if run:
                with (output/(name+'-'+iteration+'.log')).open('w') as stdout, (output/(name+'-'+iteration+'.time')).open('w') as stderr:
                    subprocess.run(['/usr/bin/time','-v',*command],stdout=stdout,stderr=stderr,check=True,cwd=ROOT)
            timing=(output/(name+'-'+iteration+'.time')).read_text()
            if 'Exit status: 0' not in timing:
                raise ValueError('conversion did not exit successfully: '+name+'-'+iteration)
            elapsed=re.search(r'Elapsed \(wall clock\) time \(h:mm:ss or m:ss\): ([0-9:.]+)',timing)
            rss=re.search(r'Maximum resident set size \(kbytes\): (\d+)',timing)
            builds.append({'iteration':iteration,'bytes':path.stat().st_size,'sha256':digest(path),
                           'host_elapsed_hms':elapsed.group(1) if elapsed else None,
                           'host_peak_process_rss_kib':int(rss.group(1)) if rss else None})
        identical=builds[0]['sha256']==builds[1]['sha256']
        matches=builds[0]['sha256']==digest(ROOT/'data'/production) if production else None
        if not identical or matches is False:
            raise ValueError('raster reproduction mismatch: '+name)
        results.append({'candidate':name,'production_file':production,'commands':commands,'builds':builds,
                        'independent_runs_identical':identical,'matches_production':matches})
    return {'schema':1,'sources':[{'role':role,'bytes':path.stat().st_size,'sha256':digest(path)} for role,path in [('etopo',etopo),('ecoregions',ecoregions),('lakes',lakes)]],
            'results':results,'basis':'New host source conversions; GNU time process RSS includes Python/build tools and is not runtime/G4 allocation evidence.',
            'no_production_files_written':True,'quarter_degree_status':'Additional research candidate only; not added to edition recipes or installed in Globe.'}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output',type=Path)
    parser.add_argument('--etopo',type=Path,required=True)
    parser.add_argument('--ecoregions',type=Path,required=True)
    parser.add_argument('--lakes',type=Path,required=True)
    parser.add_argument('--python',type=Path,default=Path(sys.executable))
    parser.add_argument('--run',action='store_true',help='explicitly perform all eight source conversions; otherwise verify existing outputs/logs')
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    report(args.report,analyze(args.output,args.etopo,args.ecoregions,args.lakes,args.python,args.run))
