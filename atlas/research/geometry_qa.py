#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Optional independent planar GEOS oracle; never repairs or emits runtime geometry."""
import argparse
from collections import Counter
import hashlib
import math
from pathlib import Path
import sys
import struct
import zlib

try:
    import shapely
    from shapely import LineString, MultiLineString, MultiPolygon, Point, Polygon, STRtree
except ImportError:
    raise SystemExit('Optional QA requires the pinned environment in atlas/research/requirements-research.lock.txt')

from audit import number, report
from concordance import distance_km
from sources import ATLAS, locked_sources, read_source
from baseline import ROOT


def ring_area(part):
    return sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(part,part[1:]+part[:1]))/2


def quantize(parts,step):
    if step is None:
        return parts
    result=[]
    for part in parts:
        points=[]
        for x,y in part:
            p=(round(max(-180,min(180,x))/step)*step,round(y/step)*step)
            if not points or p!=points[-1]:
                points.append(p)
        if points and points[0]==points[-1]:
            points.pop()
        result.append(points)
    return result


def polygon_from_parts(parts):
    """ESRI clockwise shells; associate holes by smallest containing shell, not source ordering."""
    shells=[]
    holes=[]
    diagnostics=[]
    for i,part in enumerate(parts):
        if len(set(part))<3:
            diagnostics.append({'part':i,'kind':'collapsed-ring'})
            continue
        signed=ring_area(part)
        if signed==0:
            diagnostics.append({'part':i,'kind':'zero-signed-area'})
            continue
        if signed<0:
            shells.append((i,part,Polygon(part)))
        else:
            holes.append((i,part,Polygon(part)))
    assigned={i:[] for i,part,p in shells}
    for i,part,hole in holes:
        candidates=[(shell.area,index) for index,points,shell in shells if shell.covers(hole.representative_point())]
        if not candidates:
            diagnostics.append({'part':i,'kind':'unassigned-hole'})
        else:
            assigned[min(candidates)[1]].append(part)
    polygons=[Polygon(part,assigned[i]) for i,part,shell in shells]
    if not polygons:
        return None,diagnostics
    geometry=polygons[0] if len(polygons)==1 else MultiPolygon(polygons)
    return geometry,diagnostics


def production_gpol_audit():
    raw=(ROOT/'data/countries.gpol').read_bytes()
    if raw[:8]!=b'GPOL\x01\x00\x00\x00':
        raise ValueError('production GPOL header')
    count,rings,points,text=struct.unpack_from('<IIII',raw,8)
    if len(raw)!=32+16*count+16*rings+8*points+text or zlib.crc32(raw[32:],zlib.crc32(raw[:28]))!=struct.unpack_from('<I',raw,28)[0]:
        raise ValueError('production GPOL bounds/checksum')
    ring_offset=32+16*count
    point_offset=ring_offset+16*rings
    text_offset=point_offset+8*points
    invalid=[]
    for i in range(count):
        first,n,at,size=struct.unpack_from('<IIII',raw,32+16*i)
        country_rings=[]
        for j in range(first,first+n):
            start,length,hole,parent=struct.unpack_from('<IIII',raw,ring_offset+16*j)
            coords=[struct.unpack_from('<ii',raw,point_offset+8*k) for k in range(start,start+length)]
            country_rings.append((j,hole,parent,coords))
        polys=[Polygon(coords,[other for j,is_hole,parent,other in country_rings if is_hole and parent==index])
               for index,hole,parent,coords in country_rings if not hole]
        geometry=polys[0] if len(polys)==1 else MultiPolygon(polys)
        if not shapely.is_valid(geometry):
            invalid.append({'country_index':i,'name':raw[text_offset+at:text_offset+at+size].decode('ascii'),
                            'reason_integer_microdegree_coordinates':shapely.is_valid_reason(geometry)})
    return {'pack_sha256':hashlib.sha256(raw).hexdigest(),'country_records':count,'invalid_planar_records':invalid,
            'interpretation':'Direct GPOL ring/hole metadata and exact integer microdegree coordinates. Planar GEOS validity is additional evidence, not a spherical picking oracle or proof of a visible runtime failure.'}


def polygon_audit(source_id,rows):
    profiles={}
    for label,step in [('source',None),('microdegree',1e-6),('gatl-angular-step',360/65536)]:
        issues=[]
        areas=[]
        for index,(row,parts) in enumerate(rows,1):
            geometry,diagnostics=polygon_from_parts(quantize(parts,step))
            reason=shapely.is_valid_reason(geometry) if geometry is not None else 'No polygon survived'
            if diagnostics or reason!='Valid Geometry':
                issues.append({'record_1_based':index,'name':row.get('NAME') or row.get('ADMIN'),'ne_id':row.get('NE_ID'),
                               'source_code':row.get('ADM0_A3'),'validity_reason':reason,'ring_diagnostics':diagnostics})
            areas.append(geometry.area if geometry is not None else 0)
        profiles[label]={'step_degrees':step,'issue_records':len(issues),'issues':issues,'angular_areas':areas}
    base=profiles['source'].pop('angular_areas')
    for label in ('microdegree','gatl-angular-step'):
        values=profiles[label].pop('angular_areas')
        profiles[label]['area_change_over_one_percent']=[{'record_1_based':i+1,'name':rows[i][0].get('NAME') or rows[i][0].get('ADMIN'),
              'fractional_angular_area_change':round(value/base[i]-1,8) if base[i] else None}
              for i,value in enumerate(values) if base[i] and abs(value/base[i]-1)>0.01]
    return profiles


def expected_geometry_codes(code,countries,units):
    exact={row['ADM0_A3'] for row,p in countries}
    if code in exact:
        return {code},'exact-admin0-source-code'
    aliases={row['ADM0_A3'] for row,p in countries if code in {row.get('ISO_A3'),row.get('ISO_A3_EH')}}
    if aliases:
        return aliases,'iso-alias-to-admin0'
    parents={row['ADM0_A3'] for row,p in units if code in {row.get('GU_A3'),row.get('ISO_A3'),row.get('ISO_A3_EH')}}
    return parents,'cartographic-parent-via-map-unit' if parents else 'unresolved-source-code'


def country_point_audit(scale,countries,places,units):
    geometries=[]
    codes=[]
    source_issues=[]
    labels=[]
    for row,parts in countries:
        geometry,diagnostics=polygon_from_parts(parts)
        code=row['ADM0_A3']
        if geometry is None or not shapely.is_valid(geometry) or diagnostics:
            source_issues.append(code)
            continue
        codes.append(code)
        geometries.append(geometry)
        p=Point(number(row,'LABEL_X'),number(row,'LABEL_Y'))
        if not geometry.covers(p):
            labels.append({'adm0_a3':code,'name':row['ADMIN'],'label_coordinate':[p.x,p.y],
                           'planar_distance_to_geometry_degrees':round(geometry.distance(p),9)})
    tree=STRtree(geometries)
    unexpected=[]
    absent=[]
    ambiguous=[]
    matches=0
    resolution_counts=Counter()
    for row,parts in places:
        p=Point(parts[0][0])
        expected=row['ADM0_A3']
        resolved,resolution=expected_geometry_codes(expected,countries,units)
        resolution_counts[resolution]+=1
        hit_ids=tree.query(p,predicate='intersects')
        hit_codes=sorted({codes[int(i)] for i in hit_ids})
        if resolved.intersection(hit_codes):
            matches+=1
        elif not hit_codes:
            absent.append({'ne_id':row['NE_ID'],'name':row['NAME'],'expected_adm0_a3':expected,'resolved_geometry_codes':sorted(resolved),'resolution':resolution,'coordinate':[p.x,p.y]})
        else:
            unexpected.append({'ne_id':row['NE_ID'],'name':row['NAME'],'expected_adm0_a3':expected,'resolved_geometry_codes':sorted(resolved),'resolution':resolution,'hit_adm0_a3':hit_codes,'coordinate':[p.x,p.y]})
        if len(hit_codes)>1:
            ambiguous.append({'ne_id':row['NE_ID'],'name':row['NAME'],'hit_adm0_a3':hit_codes})
    return {'scale':scale,'places_tested':len(places),'expected_geometry_code_present':matches,'source_code_resolution_counts':dict(resolution_counts),
            'no_polygon_hit_count':len(absent),'no_hit_with_invalid_expected_geometry_count':sum(bool(set(r['resolved_geometry_codes']).intersection(source_issues)) for r in absent),'other_code_only_count':len(unexpected),
            'ambiguous_count':len(ambiguous),'invalid_countries_excluded':source_issues,
            'country_label_points_outside_geometry':labels,'other_code_only':unexpected,
            'no_polygon_hit':absent,'ambiguous':ambiguous}


def hydro_audit(scale,rivers,lakes,coasts):
    coast=MultiLineString([part for row,parts in coasts for part in parts if len(part)>=2])
    lake_geometries=[]
    lake_names=[]
    rejected=[]
    for row,parts in lakes:
        geometry,diagnostics=polygon_from_parts(parts)
        if geometry is not None and shapely.is_valid(geometry) and not diagnostics:
            lake_geometries.append(geometry)
            lake_names.append(row.get('NAME') or row.get('NAME_EN') or '')
        else:
            rejected.append(row.get('NE_ID'))
    tree=STRtree(lake_geometries)
    centerlines=[]
    for index,(row,parts) in enumerate(rivers,1):
        if row['FEATURECLA']!='Lake Centerline' or not parts:
            continue
        samples=[]
        for part in parts:
            line=LineString(part)
            samples += [line.interpolate(t,normalized=True) for t in (0.1,0.5,0.9)]
        hit_sets=[{int(i) for i in tree.query(point,predicate='intersects')} for point in samples]
        matched=set().union(*hit_sets)
        inside=sum(bool(s) for s in hit_sets)
        centerlines.append({'source_record_1_based':index,'name':row.get('NAME') or row.get('NAME_EN') or '',
                            'sample_count':len(samples),'samples_in_lake':inside,'lake_names':sorted({lake_names[i] for i in matched})})
    systems={'Nile':'Q3392','Amazon':'Q3783','Congo':'Q3503','Mississippi':'Q1497','Danube':'Q1653','Mekong':'Q41179'}
    outlets={}
    for name,qid in systems.items():
        group=[(i,row,part) for i,(row,parts) in enumerate(rivers,1) if row.get('WIKIDATAID')==qid for part in parts]
        candidates=[]
        for index,row,part in group:
            for which,p in [('start',part[0]),('end',part[-1])]:
                point=Point(p)
                nearest=shapely.shortest_line(point,coast)
                q=nearest.coords[-1]
                candidates.append({'record_1_based':index,'end':which,'coordinate':list(p),
                                   'nearest_coast_coordinate':list(q),'nearest_planar_coast_degrees':round(nearest.length,9),
                                   'spherical_distance_to_planar_nearest_km':round(distance_km(p,q),3)})
        outlets[name]=min(candidates,key=lambda r:r['nearest_planar_coast_degrees']) if candidates else None
    return {'lake_centerline_records':len(centerlines),'centerlines_with_no_samples_in_lake':sum(r['samples_in_lake']==0 for r in centerlines),
            'centerlines_with_all_samples_in_lake':sum(r['samples_in_lake']==r['sample_count'] for r in centerlines),
            'invalid_lake_geometries_excluded':rejected,'centerline_details':centerlines,'named_system_closest_coast_endpoints':outlets,
            'sample_policy':'0.1/0.5/0.9 of each source part by planar length; membership is diagnostic, not proof the entire centerline is covered.'}


def analyze(cache):
    sources={s['id']:s for s in locked_sources()}
    loaded={}
    def rows(source_id):
        if source_id not in loaded:
            loaded[source_id]=read_source(sources[source_id],cache)[1]
        return loaded[source_id]
    polygons={}
    points={}
    for scale in ('110m','50m','10m'):
        source_id=f'ne_{scale}_admin_0_countries'
        polygons[source_id]=polygon_audit(source_id,rows(source_id))
        points[scale]=country_point_audit(scale,rows(source_id),rows('ne_10m_populated_places'),rows('ne_10m_admin_0_map_units'))
    hydro={}
    for scale in ('50m','10m'):
        lake_id=f'ne_{scale}_lakes'
        polygons[lake_id]=polygon_audit(lake_id,rows(lake_id))
        hydro[scale]=hydro_audit(scale,rows(f'ne_{scale}_rivers_lake_centerlines'),rows(lake_id),rows(f'ne_{scale}_coastline'))
    return {'schema':1,'oracle':{'shapely':shapely.__version__,'geos':shapely.geos_version_string,'python':sys.version.split()[0]},
            'source_lock_sha256':hashlib.sha256((ATLAS/'sources.lock.json').read_bytes()).hexdigest(),
            'production_gpol_planar':production_gpol_audit(),'polygon_profiles':polygons,'place_country_membership':points,'hydrography':hydro,
            'limits':['GEOS uses Cartesian longitude/latitude, not spherical polygon interiors or geodesic distance.',
                      'No make_valid, buffering, snapping or source repair. Invalid source geometries are reported and excluded from membership oracles.',
                      'Country point disagreement can reflect source worldview, changed assignments, generalized coasts or genuine errors; never automatically reassign.',
                      'The angular quantization profile is a precision experiment, not a proposed GATL polygon encoding or runtime migration.',
                      'Planar-nearest shore coordinate is selected in lon/lat then spherical distance is evaluated; this is not a spherical nearest-shore solution.',
                      'River mouth candidates do not infer direction or trace delta connectivity.']}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    report(args.output,analyze(args.cache))
