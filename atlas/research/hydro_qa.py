#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Endpoint/coast/lake diagnostics for six river systems. No inferred flow direction or topology repair."""
import argparse
from collections import Counter
import math
from pathlib import Path

from audit import report
from sources import locked_sources, read_source

RIVERS={'Nile':'Q3392','Amazon':'Q3783','Congo':'Q3503','Mississippi':'Q1497','Danube':'Q1653','Mekong':'Q41179'}


def endpoint_key(point):
    x,y=point
    return (round(((x+180)%360-180)*1e5),round(y*1e5))


def nearest_vertex_km(point, vertices):
    # Spherical great-circle vertex distance: an upper bound on distance to the line/polygon.
    # Latitude lower bound rejects distant vertices; no projection or coast orientation assumption.
    x,y=point
    phi=math.radians(y)
    best=math.pi
    for lon,lat in vertices:
        dphi=math.radians(lat-y)
        if abs(dphi)>best:
            continue
        dlambda=math.radians((lon-x+180)%360-180)
        a=math.sin(dphi/2)**2+math.cos(phi)*math.cos(math.radians(lat))*math.sin(dlambda/2)**2
        angle=2*math.asin(min(1,math.sqrt(max(0,a))))
        best=min(best,angle)
    return round(best*6371.0088,3)


def diagnose(cache):
    sources={s['id']:s for s in locked_sources()}
    result={}
    for scale in ('50m','10m'):
        _,rivers=read_source(sources[f'ne_{scale}_rivers_lake_centerlines'],cache)
        _,coast=read_source(sources[f'ne_{scale}_coastline'],cache)
        _,lakes=read_source(sources[f'ne_{scale}_lakes'],cache)
        coastline=[p for r,parts in coast for part in parts for p in part]
        shore=[p for r,parts in lakes for part in parts for p in part]
        incidence=Counter(endpoint_key(p) for r,parts in rivers for part in parts for p in (part[0],part[-1]))
        cases={}
        for name,qid in RIVERS.items():
            chosen=[(i,r,parts) for i,(r,parts) in enumerate(rivers,1) if r.get('WIKIDATAID')==qid]
            endpoints=[]
            for i,row,parts in chosen:
                for j,part in enumerate(parts):
                    for end,p in [('start',part[0]),('end',part[-1])]:
                        endpoints.append({'source_record_1_based':i,'part':j,'end':end,'coordinate':list(p),
                                          'all_river_endpoint_incidence_at_1e5_degree_quantum':incidence[endpoint_key(p)],
                                          'nearest_coast_vertex_km_upper_bound':nearest_vertex_km(p,coastline),
                                          'nearest_lake_vertex_km_upper_bound':nearest_vertex_km(p,shore)})
            cases[name]={'qid':qid,'source_record_count':len(chosen),'source_names':sorted({r.get('NAME','') for i,r,p in chosen}),
                         'endpoints':endpoints,
                         'closest_coast_endpoint':min(endpoints,key=lambda p:p['nearest_coast_vertex_km_upper_bound']) if endpoints else None}
        result[scale]=cases
    return {'method':'Spherical distance to source vertices (upper bound, not distance to shoreline segments); endpoint incidence uses 1e-5 degree rounded lon/lat. QID source grouping is provisional.',
            'interpretation':'Closest coast endpoint is a mouth candidate only. Direction, true outlet, confluences inside segments, island channels, lake interior connectivity, and navigability are unverified. No failure threshold is implied.',
            'cases':result}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    report(args.output,diagnose(args.cache))
