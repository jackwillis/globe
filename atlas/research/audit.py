#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Reproducible source audit and JSONL edition experiments; never writes production packs."""
import argparse
from collections import Counter, defaultdict
import gzip
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import re
import sys
import unicodedata

from baseline import ROOT, inventory
from sources import ATLAS, locked_sources, read_source


def json_bytes(value):
    return (json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(',', ':'), allow_nan=False) + '\n').encode()


def report(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False) + '\n')


def number(row, field, default=0):
    try:
        value = float(row.get(field, ''))
        return value if math.isfinite(value) else default
    except ValueError:
        return default


def valid_qid(value):
    return bool(re.fullmatch(r'Q[1-9][0-9]*', value or ''))


def identity(row, kind):
    # Provisional source namespace, intentionally not a new canonical country standard.
    key = row.get('ADM0_A3') if kind == 'admin0' else row.get('NE_ID')
    if not key or key == '-99':
        raise ValueError(f'missing {kind} source identity')
    return f'ne:{kind}:{key}'


def names(row):
    out = [{'value': row.get('NAME', row.get('ADMIN', '')), 'language': 'und', 'role': 'source', 'source_field': 'NAME'}]
    out += [{'value': value, 'language': 'zh-Hant' if key == 'NAME_ZHT' else key[5:].lower(), 'role': 'localized-source', 'source_field': key}
            for key, value in sorted(row.items()) if key.startswith('NAME_') and value and key not in {'NAME_ALT','NAME_SORT','NAME_LONG','NAME_LEN','NAME_CIAWF'}]
    return out


def place_record(row, parts, reasons):
    return {'id': identity(row, 'place'), 'source_id': row['NE_ID'],
            'wikidata': row['WIKIDATAID'] if valid_qid(row.get('WIKIDATAID')) else None,
            'source': 'ne_10m_populated_places', 'country_source_code': row['ADM0_A3'], 'sovereign_source_code': row['SOV_A3'],
            'classification': row['FEATURECLA'], 'capital_source_flag': number(row, 'ADM0CAP') == 1,
            'alternate_capital_source_flag': number(row, 'CAPALT') == 1,
            'names': names(row), 'source_ascii_name': row.get('NAMEASCII'),
            'coordinate_microdegrees': [round(x*1e6) for x in parts[0][0]],
            'label_priority': {'min_zoom': number(row, 'MIN_ZOOM', 99), 'labelrank': number(row, 'LABELRANK', 99)},
            'population': {'value': number(row, 'POP_MAX'), 'basis': 'Natural Earth POP_MAX; mixed source dates, metro-oriented, not a current census'},
            'selection_reasons': sorted(reasons)}


def select_places(rows, recipe):
    chosen = defaultdict(set)
    for row, parts in rows:
        key = row['NE_ID']
        if number(row, 'ADM0CAP') == 1 or number(row, 'CAPALT') == 1:
            chosen[key].add('capital-source-flag')
        if number(row, 'MIN_ZOOM', 99) <= recipe['place_min_zoom']:
            chosen[key].add('cartographic-min-zoom')
        if 'Admin-1' in row['FEATURECLA'] and number(row, 'MIN_ZOOM', 99) <= recipe['regional_min_zoom']:
            chosen[key].add('regional-center')
    # One source-ranked representative for each ADM0 code and occupied angular bin.
    # Equal-angle bins are a reproducible sampling heuristic, not equal-area fairness.
    seen_country, seen_cell = set(), set()
    for row, parts in sorted(rows, key=lambda item: (number(item[0], 'MIN_ZOOM', 99), -number(item[0], 'POP_MAX'), int(item[0]['NE_ID']))):
        lon, lat = parts[0][0]
        degrees = recipe['coverage_cell_degrees']
        cell = (int(((lon+180) % 360) // degrees), min(int((lat+90)//degrees), int(180/degrees)-1))
        if row['ADM0_A3'] not in seen_country:
            chosen[row['NE_ID']].add('country-representative')
            seen_country.add(row['ADM0_A3'])
        if cell not in seen_cell:
            chosen[row['NE_ID']].add('occupied-grid-representative')
            seen_cell.add(cell)
    return [(r, p, chosen[r['NE_ID']]) for r, p in rows if r['NE_ID'] in chosen]


def duplicate_values(rows, field):
    groups = defaultdict(list)
    for row, parts in rows:
        value = row.get(field)
        if value and value != '-99':
            groups[value].append(row.get('NAME', row.get('ADMIN', '')))
    return {key: value for key, value in sorted(groups.items()) if len(value) > 1}


def summary(shape, rows):
    return {'shape_type': shape, 'records': len(rows), 'null_geometry_records': [i+1 for i,(r,p) in enumerate(rows) if not p],
            'parts': sum(len(p) for r,p in rows), 'vertices': sum(len(part) for r,p in rows for part in p),
            'multipart_records': sum(len(p)>1 for r,p in rows),
            'antimeridian_segment_records': [i+1 for i,(r,p) in enumerate(rows) if any(abs(a[0]-b[0])>180 for part in p for a,b in zip(part,part[1:]))],
            'feature_classes': dict(sorted(Counter(r.get('FEATURECLA', '') for r,p in rows).items())),
            'duplicate_ne_ids': duplicate_values(rows, 'NE_ID'),
            'duplicate_wikidata_ids': duplicate_values(rows, 'WIKIDATAID')}


def place_metrics(rows):
    keys = [r.get('WIKIDATAID') for r,p in rows if valid_qid(r.get('WIKIDATAID'))]
    return {'records': len(rows), 'adm0_codes': len({r['ADM0_A3'] for r,p in rows}),
            'sovereign_codes': len({r['SOV_A3'] for r,p in rows}),
            'capital_flag_records': sum(number(r,'ADM0CAP')==1 for r,p in rows),
            'alternate_capital_records': sum(number(r,'CAPALT')==1 for r,p in rows),
            'missing_qid_records': sum(not valid_qid(r.get('WIKIDATAID')) for r,p in rows),
            'duplicate_qids': {k:v for k,v in Counter(keys).items() if v>1},
            'zero_population_records': sum(number(r,'POP_MAX')<=0 for r,p in rows),
            'country_distribution': dict(sorted(Counter(r['ADM0_A3'] for r,p in rows).items()))}


def geometry_record(row, parts, source, index):
    return {'source': source, 'source_record_1_based': index,
            'source_ne_id': row.get('NE_ID'), 'source_code': row.get('ADM0_A3'),
            'name': row.get('NAME_EN') or row.get('NAME') or row.get('ADMIN') or '',
            'classification': row.get('FEATURECLA', ''), 'scalerank': number(row,'SCALERANK'),
            # Keep source parts, including closure/hole orientation; no claim of repaired topology.
            'parts_microdegrees': [[[round(max(-180,min(180,x))*1e6), round(y*1e6)] for x,y in part] for part in parts]}


def emit_layer(folder, name, records):
    raw = b''.join(json_bytes(record) for record in records)
    packed = gzip.compress(raw, compresslevel=9, mtime=0)
    (folder / (name+'.jsonl')).write_bytes(raw)
    (folder / (name+'.jsonl.gz')).write_bytes(packed)
    return {'path': name+'.jsonl', 'records': len(records), 'jsonl_bytes': len(raw),
            'gzip_bytes': len(packed), 'sha256': hashlib.sha256(raw).hexdigest(),
            'gzip_sha256': hashlib.sha256(packed).hexdigest()}


def main(cache, output):
    output.mkdir(parents=True, exist_ok=True)
    data, summaries = {}, {}
    for source in locked_sources():
        shape, rows = read_source(source, cache)
        data[source['id']] = rows
        summaries[source['id']] = summary(shape, rows)
    report(output/'sources.json', summaries)
    required = json.loads((ATLAS/'qa/required.json').read_text())
    countries = data['ne_10m_admin_0_countries']
    political = []
    for row, parts in countries:
        code = row['ADM0_A3']
        political.append({'id': identity(row,'admin0'), 'source': 'ne_10m_admin_0_countries', 'adm0_a3': code, 'name': row['ADMIN'],
                          'wikidata': row.get('WIKIDATAID'), 'classification': row['TYPE'],
                          'sovereign_source_code': row['SOV_A3'], 'sovereign_source_name': row['SOVEREIGNT'],
                          'names': names(row),
                          'representative_point': [number(row,'LABEL_X'), number(row,'LABEL_Y')],
                          'point_role': 'Natural Earth label point; not verified as inside every polygon',
                          'tiny_point_scales': [scale for scale in ('110m','50m') if any(r['ADM0_A3']==code for r,p in data[f'ne_{scale}_admin_0_tiny_countries'])],
                          'geometry_scales': [scale for scale in ('110m','50m','10m') if any(r['ADM0_A3']==code for r,p in data[f'ne_{scale}_admin_0_countries'])]})
    report(output/'political-coverage.json', {'catalog_basis':'10m Natural Earth admin0 countries; source convention, not a sovereign-state list',
         'required_cases': [p for p in political if p['adm0_a3'] in required['political']],
         'omitted_at_110m': [{k:p[k] for k in ('id','name','wikidata','classification')} for p in political if '110m' not in p['geometry_scales']],
         'omitted_at_50m': [{k:p[k] for k in ('id','name','wikidata','classification')} for p in political if '50m' not in p['geometry_scales']]})
    places = data['ne_10m_populated_places']
    spec = importlib.util.spec_from_file_location('production_places', ROOT/'tools/places-builder/build_places.py')
    production = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(production)
    old_rows = json.loads((ROOT/'data/wikidata-cities-large-20261102.json').read_bytes())['results']['bindings']
    old = production.large_source(ROOT/'data/wikidata-cities-large-20261102.json')
    old_ids = {f'Q{r[0]}' for r in old}
    recipes = json.loads((ATLAS/'editions/recipes.json').read_text())
    candidates = {edition: select_places(places, recipes[edition]) for edition in ('compact','standard','expanded')}
    b = data['ne_50m_populated_places']
    strategy = {'A': {'records':len(old), 'source_rows':len(old_rows), 'snapshot_country_ids':len({r['country']['value'] for r in old_rows if 'country' in r and r['item']['value'].rsplit('/',1)[1] in old_ids}),
                      'names':[r[1] for r in sorted(old)], 'note':'Exact production builder; country IDs are Wikidata, not comparable to ADM0 count without a crosswalk.'},
                'B': place_metrics(b), 'C': place_metrics([(r,p) for r,p,reasons in candidates['standard']])}
    comparisons = []
    for qid in required['places']:
        matched = [(r,p) for r,p in places if r.get('WIKIDATAID')==qid]
        comparisons.append({'qid':qid,'fixture_label':required['place_labels'][qid], 'source_names':[r['NAME'] for r,p in matched],
                            'A':qid in old_ids, 'B':any(r.get('WIKIDATAID')==qid for r,p in b),
                            **{edition:any(r.get('WIKIDATAID')==qid for r,p,reasons in rows) for edition,rows in candidates.items()}})
    report(output/'places.json', {'strategies':strategy,'required_cases':comparisons,
          'A_qids_not_in_NE10m':sorted(old_ids-{r.get('WIKIDATAID') for r,p in places}),
          'NE10m_capitals_omitted_by_A':[{'name':r['NAME'],'qid':r.get('WIKIDATAID'),'adm0':r['ADM0_A3'],'classification':r['FEATURECLA']} for r,p in places if number(r,'ADM0CAP')==1 and r.get('WIKIDATAID') not in old_ids],
          'crosswalk_note':'QID equality is a proposed concordance, not verification of coordinate/population/entity equivalence. No fuzzy name joins.'})
    name_changes, encoding = [], {}
    for row, parts in places:
        name = row['NAME']
        try:
            folded = production.ascii_name(name).decode()
        except ValueError:
            folded = None
        if name != folded:
            name_changes.append({'id':identity(row,'place'),'source':name,'legacy_ascii':folded,'source_ascii':row.get('NAMEASCII')})
    for codec in ('ascii','mac_roman','iso8859-1'):
        failed=[]
        for row, parts in places:
            try:
                unicodedata.normalize('NFC',row['NAME']).encode(codec,errors='strict')
            except UnicodeEncodeError:
                failed.append({'id':identity(row,'place'),'name':row['NAME']})
        encoding[codec]={'unrepresentable_count':len(failed),'examples':failed[:30]}
    report(output/'names.json',{'encoding_profiles':encoding,'legacy_changes':name_changes,
          'country_legacy_changes':[{'source':r['ADMIN'],'legacy':unicodedata.normalize('NFKD',r['ADMIN']).encode('ascii','ignore').decode()} for r,p in countries if r['ADMIN']!=unicodedata.normalize('NFKD',r['ADMIN']).encode('ascii','ignore').decode()],
          'note':'Codec representability only, not installed glyph or shaping verification. Source NAME has unspecified language; language-specific fields retained separately.'})
    hydro = {}
    for scale in ('50m','10m'):
        rows = data[f'ne_{scale}_rivers_lake_centerlines']
        hydro[scale] = {'summary':summaries[f'ne_{scale}_rivers_lake_centerlines'],
            'unnamed_records':sum(not (r.get('NAME') or r.get('NAME_EN')) for r,p in rows),
            'rank_distribution':dict(sorted(Counter(r['SCALERANK'] for r,p in rows).items())),
            'lake_rank_distribution':dict(sorted(Counter(r['SCALERANK'] for r,p in data[f'ne_{scale}_lakes']).items())),
            'lake_zoom_distribution':dict(sorted(Counter(r['MIN_ZOOM'] for r,p in data[f'ne_{scale}_lakes']).items())),
            'review_cases': {name:[{'record_1_based':i,'name':r.get('NAME'),'english':r.get('NAME_EN'),'rank':r['SCALERANK'],'class':r['FEATURECLA'],'parts':len(p),'vertices':sum(map(len,p))} for i,(r,p) in enumerate(rows,1) if name.casefold() in {r.get('NAME','').casefold(),r.get('NAME_EN','').casefold()}] for name in required['rivers']}}
    report(output/'hydrography.json',hydro)
    baseline = {Path(p['path']).name:p for p in inventory()['packs']}
    editions = {}
    for edition, selected_places in candidates.items():
        recipe = recipes[edition]
        folder = output/edition
        folder.mkdir(exist_ok=True)
        layers = {}
        layers['identity'] = emit_layer(folder,'identity',political)
        place_records = sorted([place_record(r,p,reasons) for r,p,reasons in selected_places],key=lambda r:r['id'])
        layers['places'] = emit_layer(folder,'places',place_records)
        for layer, source_id, rank in [('countries',f"ne_{recipe['geometry_scale']}_admin_0_countries",None),
                                       ('coastline',f"ne_{recipe['geometry_scale']}_coastline",None),
                                       ('borders',f"ne_{recipe['geometry_scale']}_admin_0_boundary_lines_land",None),
                                       ('rivers',f"ne_{recipe['hydro_scale']}_rivers_lake_centerlines",recipe['river_max_scalerank']),
                                       ('lakes',f"ne_{recipe['hydro_scale']}_lakes",recipe['lake_max_min_zoom'])]:
            selected=[(i,r,p) for i,(r,p) in enumerate(data[source_id],1) if p and (rank is None or number(r,'MIN_ZOOM' if layer == 'lakes' else 'SCALERANK',99)<=rank)]
            records=[geometry_record(r,p,source_id,i) for i,r,p in selected]
            layer_info=emit_layer(folder,layer,records)
            vertices=sum(len(part) for i,r,p in selected for part in p)
            parts_count=sum(len(p) for i,r,p in selected)
            # A deliberately simple hypothetical fixed-width typed representation.
            decoded=8*vertices+16*parts_count+32*len(selected)
            vector_cache=24*vertices
            layer_info.update(vertices=vertices,parts=parts_count,
                memory_estimate={'decoded_typed_payload_bytes':decoded,'persistent_with_unit_vectors_bytes':decoded+vector_cache,
                                 'peak_additional_load_bytes':layer_info['gzip_bytes']+decoded,
                                 'evictable_detail_bytes':decoded+vector_cache},source=source_id)
            layers[layer]=layer_info
        terrain=sum(baseline[p]['terrain_payload_estimate_bytes'] for p in recipe['elevation'])
        terrain_packed=sum(baseline[p]['packed_bytes'] for p in recipe['elevation'])
        # Compact point structs: position + ID + name offsets + priority/flags, plus chosen UTF8 display text.
        point_estimate=40*len(place_records)+sum(len((r.get('source_ascii_name') or r['names'][0]['value']).encode('utf-8')) for r in place_records)
        identity_estimate=40*len(political)+sum(len(r['name'].encode('utf-8')) for r in political)
        vector_resident=sum(layer.get('memory_estimate',{}).get('persistent_with_unit_vectors_bytes',0) for layer in layers.values())
        total=vector_resident+terrain+terrain_packed+baseline['landclasses-1deg.ggrd']['packed_bytes']+64800+point_estimate+identity_estimate
        flat_records=[{'layer':name,'record':json.loads(line)} for name in sorted(layers) for line in (folder/(name+'.jsonl')).read_bytes().splitlines()]
        mono=emit_layer(folder,'monolithic',flat_records)
        manifest={'schema':1,'edition':edition,'recipe':recipe,'source_lock_sha256':hashlib.sha256((ATLAS/'sources.lock.json').read_bytes()).hexdigest(),
             'tool_sha256':{name:hashlib.sha256((ATLAS/'research'/name).read_bytes()).hexdigest() for name in ('audit.py','sources.py','baseline.py')},
             'recipes_sha256':hashlib.sha256((ATLAS/'editions/recipes.json').read_bytes()).hexdigest(),
             'layers':layers,'existing_rasters':{p:baseline[p] for p in recipe['elevation']+['landclasses-1deg.ggrd']},
             'comparison':{'independent_jsonl_bytes':sum(l['jsonl_bytes'] for l in layers.values()),'independent_gzip_bytes':sum(l['gzip_bytes'] for l in layers.values()),'monolithic':mono},
             'memory_estimate':{'atlas_resident_payload_bytes':total,'terrain_payload_bytes':terrain,'raster_packed_bytes':terrain_packed+3607,
               'mandatory_identity_point_payload_bytes':point_estimate+identity_estimate,
               'peak_additional_largest_vector_load_bytes':max(l.get('memory_estimate',{}).get('peak_additional_load_bytes',0) for l in layers.values()),
               'replacement_peak_basis':'old resident + full candidate resident + largest layer loading temporary; no automatic old-pack release',
               'excludes':'framebuffers, scene caps/indexes, per-job geometry, label layout, glyphs, allocator/container overhead, Python/JSON objects and OS memory',
               'basis':'analytical fixed-width typed payload plus current terrain formula; NOT memory of JSON parser or measured G4 allocations'},
             'place_metrics':place_metrics([(r,p) for r,p,reasons in selected_places])}
        report(folder/'manifest.json',manifest)
        editions[edition]=manifest
    report(output/'editions.json',editions)
    failures=[]
    checks=[]
    for source_id, stats in summaries.items():
        canonical_layer=source_id.endswith('_admin_0_countries') or source_id.endswith('_populated_places')
        ok=not stats['duplicate_ne_ids']
        checks.append({'kind':'unique-source-ne-id' if canonical_layer else 'geometry-feature-id-reuse',
                       'source':source_id,'status':'pass' if ok else ('failure' if canonical_layer else 'warning'),
                       'interpretation':'Geometry records may share a feature ID; record identity includes source lock and ordinal.'})
        if canonical_layer and not ok:failures.append(source_id+' duplicate NE_ID')
        accepted_nulls=[x['record_1_based'] for x in required['accepted_source_exceptions'] if x['source']==source_id and x['kind']=='null_geometry']
        ok=stats['null_geometry_records']==accepted_nulls
        checks.append({'kind':'expected-null-geometries','source':source_id,'status':'pass' if ok else 'failure'})
        if not ok:failures.append(source_id+' unexpected null geometry')
    codes=[p['adm0_a3'] for p in political]
    if len(codes)!=len(set(codes)):
        failures.append('duplicate political source code')
    for scale in ('110m','50m','10m'):
        fiji=[parts for row,parts in data[f'ne_{scale}_admin_0_countries'] if row['ADM0_A3']=='FJI']
        ok=len(fiji)==1 and len(fiji[0])>1
        checks.append({'kind':'multipart-archipelago','source_scale':scale,'id':'FJI','status':'pass' if ok else 'failure'})
        if not ok:failures.append('Fiji multipart '+scale)
    required_capital_ids={r['NE_ID'] for r,p in places if number(r,'ADM0CAP')==1 or number(r,'CAPALT')==1}
    for edition,chosen in candidates.items():
        selected_ids={r['NE_ID'] for r,p,reasons in chosen}
        missing=sorted(required_capital_ids-selected_ids)
        checks.append({'kind':'all-source-capital-flags-preserved','edition':edition,
                       'missing_source_ids':missing,'status':'failure' if missing else 'pass'})
        if missing:failures.append(edition+' missing capital source flags')
    for code in required['political']:
        ok=any(p['adm0_a3']==code for p in political)
        checks.append({'kind':'required-political-identity','id':code,'status':'pass' if ok else 'failure'})
        if not ok:failures.append(code)
    for item in comparisons:
        ok=item['fixture_label'] in item['source_names'] and item['standard']
        checks.append({'kind':'required-place-standard','id':item['qid'],'status':'pass' if ok else 'failure'})
        if not ok:failures.append(item['qid'])
    for scale,h in hydro.items():
        for name,records in h['review_cases'].items():
            ok=bool(records)
            checks.append({'kind':'required-river-source','scale':scale,'name':name,'status':'pass' if ok else 'failure'})
            if not ok:failures.append(name+scale)
    report(output/'qa.json',{'failures':failures,'checks':checks,'accepted_exceptions':required['accepted_source_exceptions'],
         'warnings':['Source identities/QIDs and political classifications need editorial review; duplicate source QIDs are reported in sources.json.',
                     'Hydrography presence is verified; connectivity, direction, river mouths and lake/coast alignment are not certified.',
                     'Required cases are a small fixture, not a complete sovereign/capital list.']})
    if failures:
        raise ValueError('semantic QA failures: '+', '.join(failures))
    print('Audit and edition experiments complete:',output)

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache',type=Path)
    parser.add_argument('output',type=Path)
    args=parser.parse_args()
    main(args.cache,args.output)
