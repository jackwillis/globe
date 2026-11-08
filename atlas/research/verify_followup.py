#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Cross-report semantic invariants; existing geographic warnings remain visible."""
import hashlib
import json
from pathlib import Path

from sources import ATLAS


def verify():
    read=lambda name:json.loads((ATLAS/'reports'/name).read_text())
    names=read('name-profiles.json')
    editions=read('editions.json')
    rivers=read('river-components.json')
    concordance=read('concordance.json')
    reproduction=read('raster-reproduction.json')
    checks=[]
    def check(label,condition):
        checks.append({'check':label,'pass':bool(condition)})
    check('reference snapshot digest',concordance['reference_lock_sha256']==hashlib.sha256((ATLAS/'references.lock.json').read_bytes()).hexdigest())
    for edition in ('compact','standard','expanded'):
        for codec,profile in names['editions'][edition].items():
            check(edition+' '+codec+' retains all selected identities',profile['records']==editions[edition]['place_metrics']['records'])
            check(edition+' '+codec+' lossy candidates explicitly marked',all(row['loss'] and row['review_required'] for row in profile['review_cases']) and (not profile['review_cases_complete'] or len(profile['review_cases'])==profile['fallback_candidates']))
    for edition in ('compact','standard'):
        graph=rivers['scales'][edition+'-recipe']
        check(edition+' river geometry count matches recipe',graph['source_geometry_records_selected']==editions[edition]['layers']['rivers']['records'])
    for profile,graph in rivers['scales'].items():
        for name,system in graph['systems'].items():
            distances=[c['closest_degree_one_shore_endpoint']['spherical_distance_to_planar_nearest_km'] for c in system['connected_components'] if c['closest_degree_one_shore_endpoint']]
            check(profile+' '+name+' has a cartographic near-shore component',bool(distances) and min(distances)<1)
    for raster in reproduction['results']:
        check(raster['candidate']+' independent conversion equality',raster['independent_runs_identical'])
        if raster['production_file']:
            check(raster['candidate']+' matches production bytes',raster['matches_production'])
    failures=[c['check'] for c in checks if not c['pass']]
    if failures:
        raise ValueError('semantic cross-report failures: '+', '.join(failures))
    return {'checks':checks,'failures':failures,'interpretation':'Internal consistency and scoped cartographic checks, not certification of source identities, global topology or hydrological routing.'}

if __name__=='__main__':
    print(json.dumps(verify(),indent=2))
