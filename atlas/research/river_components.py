#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Undirected cartographic connectivity diagnostics for the six reviewed river systems."""
import argparse
from collections import Counter, defaultdict
from pathlib import Path

import shapely
from shapely import MultiLineString, Point

from audit import number, report
from concordance import distance_km
from sources import locked_sources, read_source

SYSTEMS={'Nile':'Q3392','Amazon':'Q3783','Congo':'Q3503','Mississippi':'Q1497','Danube':'Q1653','Mekong':'Q41179'}


def key(p):
    return (round(p[0]*1e6),round(p[1]*1e6))


def analyze(cache):
    sources={s['id']:s for s in locked_sources()}
    result={}
    for profile,scale,rank in [('full-50m','50m',None),('compact-recipe','50m',2),('standard-recipe','50m',5),('full-10m','10m',None)]:
        _,rows=read_source(sources[f'ne_{scale}_rivers_lake_centerlines'],cache)
        _,coast_rows=read_source(sources[f'ne_{scale}_coastline'],cache)
        rows=[(row,parts) for row,parts in rows if row['FEATURECLA']!='Canal' and (rank is None or number(row,'SCALERANK',99)<=rank)]
        parts=[part for row,parts in rows for part in parts if len(part)>1]
        noded=shapely.node(MultiLineString(parts))
        graph=defaultdict(set)
        degree=Counter()
        for line in noded.geoms:
            a,b=key(line.coords[0]),key(line.coords[-1])
            graph[a].add(b)
            graph[b].add(a)
            degree[a]+=1
            degree[b]+=1
        component={}
        groups=[]
        for start in sorted(graph):
            if start in component:continue
            index=len(groups)
            group=set()
            stack=[start]
            while stack:
                p=stack.pop()
                if p in group:continue
                group.add(p)
                component[p]=index
                stack.extend(graph[p]-group)
            groups.append(group)
        coast=MultiLineString([part for row,parts in coast_rows for part in parts if len(part)>1])
        cases={}
        for name,qid in SYSTEMS.items():
            seeds={key(p) for row,parts in rows if row.get('WIKIDATAID')==qid for part in parts for p in (part[0],part[-1])}
            ids=sorted({component[p] for p in seeds if p in component})
            selected=[]
            for index in ids:
                group=groups[index]
                leaves=[p for p in group if degree[p]==1]
                candidates=[]
                for p in leaves:
                    point=(p[0]/1e6,p[1]/1e6)
                    shore=shapely.shortest_line(Point(point),coast)
                    q=shore.coords[-1]
                    candidates.append({'endpoint':list(point),'nearest_shore':list(q),'planar_distance_degrees':shore.length,
                                       'spherical_distance_to_planar_nearest_km':round(distance_km(point,q),3)})
                records=[(i,row) for i,(row,parts) in enumerate(rows,1) if any(key(part[0]) in group or key(part[-1]) in group for part in parts)]
                selected.append({'component_id':index,'nodes':len(group),'degree_one_nodes':len(leaves),
                    'source_records':len(records),'source_names':sorted({row.get('NAME') or row.get('NAME_EN') or '' for i,row in records}),
                    'source_classes':sorted({row['FEATURECLA'] for i,row in records}),
                    'closest_degree_one_shore_endpoint':min(candidates,key=lambda r:r['planar_distance_degrees']) if candidates else None})
            cases[name]={'source_qid':qid,'seed_endpoints':len(seeds),'unresolved_seed_endpoints':sorted(seeds-set(component)),
                         'connected_components':selected}
        result[profile]={'source_scale':scale,'maximum_scalerank':rank,'source_records_selected_including_null':len(rows),'source_geometry_records_selected':sum(bool(parts) for row,parts in rows),'null_geometry_records_selected':sum(not parts for row,parts in rows),'featurecla_canal_records_excluded':True,'canal_name_hints_retained':sorted({row.get('NAME') or row.get('NAME_EN') or '' for row,parts in rows if 'canal' in (row.get('NAME','')+' '+row.get('NAME_EN','')).lower()}),'noded_line_parts':len(noded.geoms),'graph_nodes':len(graph),'components':len(groups),'systems':cases}
    return {'schema':1,'shapely':shapely.__version__,'geos':shapely.geos_version_string,
            'method':'GEOS planar noding of selected river/centerline parts, excluding canals; endpoint node keys rounded to microdegrees; undirected components include tributaries; full-source and recipe rank-filtered profiles are compared.',
            'scales':result,'limits':['Not a hydrological routing model, flow-direction solution or verified river-mouth graph. FEATURECLA Canal records are excluded, but named canals tagged River/Lake Centerline remain and are reported.',
                'Planar intersections can connect cartographic crossings; microdegree keys can merge near endpoints. No claim of surveyed connectivity.',
                'Only degree-one nodes are tested as shore candidates; closed loops and coast-touching internal nodes are not outlets in this diagnostic.',
                'A named main stem stopping inland is not an omission if separately named distributaries connect to the coast; inspect component names before drawing conclusions.']}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    report(args.output,analyze(args.cache))
