#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Compare actual equal-angle and equal-area place sets without changing edition recipes."""
import argparse
from collections import Counter, defaultdict
import json
import math
from pathlib import Path

from audit import emit_layer, number, place_record, report, select_places
from sources import ATLAS, locked_sources, read_source


def grid_cell(lon, lat, degrees, equal_area=False):
    if not (math.isfinite(lon) and math.isfinite(lat) and -180 <= lon <= 180 and -90 <= lat <= 90):
        raise ValueError('invalid coordinate')
    if degrees <= 0 or 180 % degrees != 0:
        raise ValueError('cell degrees must divide 180')
    nx, ny = int(360 / degrees), int(180 / degrees)
    y = (math.sin(math.radians(lat)) + 1) / 2 if equal_area else (lat + 90) / 180
    return (int(((lon + 180) % 360) / 360 * nx), min(int(y * ny), ny - 1))


def select_equal_area(rows, recipe):
    # Preserve all non-grid reasons from the existing algorithm.
    chosen = {r['NE_ID']: set(reasons) - {'occupied-grid-representative'}
              for r, p, reasons in select_places(rows, recipe)}
    seen = set()
    for row, parts in sorted(rows, key=lambda item: (number(item[0], 'MIN_ZOOM', 99),
                                                    -number(item[0], 'POP_MAX'), int(item[0]['NE_ID']))):
        cell = grid_cell(*parts[0][0], recipe['coverage_cell_degrees'], equal_area=True)
        if cell not in seen:
            chosen.setdefault(row['NE_ID'], set()).add('occupied-equal-area-representative')
            seen.add(cell)
    return [(r, p, chosen[r['NE_ID']]) for r, p in rows if chosen.get(r['NE_ID'])]



def equal_budget(rows, selected, target):
    """Fill a candidate set to a comparison count by deterministic spherical farthest-first."""
    if target < len(selected) or target > len(rows):
        raise ValueError('target must contain seed set and fit source roster')
    chosen = {r['NE_ID']: set(reasons) for r, p, reasons in selected}
    selected_vectors = [vector(p[0][0]) for r, p, reasons in selected]
    if not selected_vectors:
        raise ValueError('nonempty seed required')
    candidates = []
    for row, parts in rows:
        if row['NE_ID'] not in chosen:
            v = vector(parts[0][0])
            closest_dot = max(sum(a*b for a, b in zip(v, w)) for w in selected_vectors)
            candidates.append([closest_dot, number(row, 'MIN_ZOOM', 99), -number(row, 'POP_MAX'),
                               int(row['NE_ID']), row, parts, v])
    while len(chosen) < target:
        index = min(range(len(candidates)), key=lambda i: tuple(candidates[i][:4]))
        item = candidates.pop(index)
        chosen[item[4]['NE_ID']] = {'equal-budget-distance-supplement'}
        for other in candidates:
            other[0] = max(other[0], sum(a*b for a, b in zip(item[6], other[6])))
    return [(r, p, chosen[r['NE_ID']]) for r, p in rows if r['NE_ID'] in chosen]


def vector(point):
    lon, lat = map(math.radians, point)
    return math.cos(lat)*math.cos(lon), math.cos(lat)*math.sin(lon), math.sin(lat)


def nearest_gaps(rows, selected):
    # Exact spherical nearest-selected distance on a spherical Earth, not a polygon coverage claim.
    selected_ids = {r['NE_ID'] for r, p, reasons in selected}
    vectors = [vector(p[0][0]) for r, p, reasons in selected]
    gaps = []
    for row, parts in rows:
        if row['NE_ID'] in selected_ids:
            distance = 0.0
        else:
            x, y, z = vector(parts[0][0])
            dot = max(x*a + y*b + z*c for a, b, c in vectors)
            distance = 6371.0088 * math.acos(max(-1, min(1, dot)))
        gaps.append((distance, row['NE_ID'], row['NAME']))
    ordered = sorted(d for d, _, _ in gaps)
    return {'basis': 'Nearest selected settlement for every source settlement; not land/ocean coverage.',
            'median_km': round(ordered[len(ordered)//2], 3),
            'p95_km': round(ordered[math.ceil(len(ordered)*0.95)-1], 3),
            'largest_gaps': [{'km': round(d, 3), 'source_id': key, 'name': name}
                             for d, key, name in sorted(gaps, reverse=True)[:10]]}


def metrics(rows, selected, political_codes):
    ids = {r['NE_ID'] for r, p, reasons in selected}
    codes = {r['ADM0_A3'] for r, p, reasons in selected}
    capitals = {r['NE_ID'] for r, p in rows if number(r, 'ADM0CAP') == 1 or number(r, 'CAPALT') == 1}
    bands = lambda lat: 'south-of-60S' if lat < -60 else '60S-to-30S' if lat < -30 else '30S-to-equator' if lat < 0 else 'equator-to-30N' if lat < 30 else '30N-to-60N' if lat < 60 else 'north-of-60N'
    full_cells = Counter(grid_cell(*p[0][0], 15, True) for r, p in rows)
    selected_cells = Counter(grid_cell(*p[0][0], 15, True) for r, p, reasons in selected)
    country_distribution = Counter(r['ADM0_A3'] for r, p, reasons in selected)
    result = {'records': len(selected), 'source_capital_records': len(capitals),
        'missing_capital_source_ids': sorted(capitals - ids),
        'selected_national_capital_flag_records': sum(number(r, 'ADM0CAP') == 1 for r, p, reasons in selected),
        'selected_alternate_capital_flag_records': sum(number(r, 'CAPALT') == 1 for r, p, reasons in selected),
        'adm0_source_codes': len(codes), 'country_distribution': dict(sorted(country_distribution.items())),
        'raw_country_layer_codes_without_selected_settlement': sorted(political_codes - codes),
        'raw_source_codes_without_any_settlement': sorted(political_codes - {r['ADM0_A3'] for r, p in rows}),
        'latitude_band_distribution': dict(sorted(Counter(bands(p[0][0][1]) for r, p, reasons in selected).items())),
        'reference_15deg_equal_area_occupied_cells': len(full_cells),
        'represented_reference_cells': len(set(full_cells) & set(selected_cells)),
        'missing_reference_cells': [list(c) for c in sorted(set(full_cells) - set(selected_cells))],
        'required_small_country_codes': {code: country_distribution[code] for code in ('SMR', 'VAT', 'MCO', 'AND', 'LIE', 'KIR', 'TUV', 'MDV')},
        'source_settlement_distance': nearest_gaps(rows, selected)}
    records = [place_record(r, p, reasons) for r, p, reasons in selected]
    # Scenario intentionally excludes names already embedded in other hypothetical tables.
    text = sum(len(r['names'][0]['value'].encode('utf-8')) + 1 for r in records)
    result['resident_payload_scenario'] = {'record_bytes': 32*len(records), 'default_utf8_name_bytes': text,
        'total_bytes': 32*len(records) + text,
        'basis': 'Illustrative 32-byte fixed record plus NUL-terminated exact default UTF-8 name; not a decoder, allocator measurement, approved format or total runtime RAM.'}
    return result


def analyze(cache, output):
    output.mkdir(parents=True, exist_ok=True)
    sources = {s['id']: s for s in locked_sources()}
    _, rows = read_source(sources['ne_10m_populated_places'], cache)
    _, countries = read_source(sources['ne_10m_admin_0_countries'], cache)
    political_codes = {r['ADM0_A3'] for r, p in countries}
    recipes = json.loads((ATLAS/'editions/recipes.json').read_text())
    comparisons = {}
    for edition in ('compact', 'standard'):
        variants = {'equal-angle': select_places(rows, recipes[edition]),
                    'equal-area': select_equal_area(rows, recipes[edition])}
        variants['equal-area-equal-count'] = equal_budget(rows, variants['equal-area'], len(variants['equal-angle']))
        results = {}
        for policy, selected in variants.items():
            records = sorted([place_record(r, p, reasons) for r, p, reasons in selected], key=lambda r: r['id'])
            results[policy] = metrics(rows, selected, political_codes)
            results[policy]['actual_jsonl'] = emit_layer(output, edition + '-' + policy, records)
        sets = {policy: {r['NE_ID'] for r, p, reasons in selected} for policy, selected in variants.items()}
        def describe(ids):
            return [{'source_id': r['NE_ID'], 'name': r['NAME'], 'adm0_source_code': r['ADM0_A3'],
                     'coordinate': list(p[0][0])} for r, p in rows if r['NE_ID'] in ids]
        results['added_by_equal_area'] = describe(sets['equal-area'] - sets['equal-angle'])
        results['removed_by_equal_area'] = describe(sets['equal-angle'] - sets['equal-area'])
        results['equal_count_supplement'] = describe(sets['equal-area-equal-count'] - sets['equal-area'])
        results['intersection_records'] = len(sets['equal-angle'] & sets['equal-area'])
        comparisons[edition] = results
    return {'schema': 1, 'status': 'research comparison; no recipe or runtime change',
        'source_sha256': {key: sources[key]['sha256'] for key in ('ne_10m_populated_places', 'ne_10m_admin_0_countries')},
        'method': 'Same role thresholds, source rank, country representatives and nominal grid count; equal-area uses uniform longitude and uniform sin(latitude) bins. A separate equal-count candidate fills to the equal-angle count by farthest-first spherical distance, with source importance/population/ID tie breaks.',
        'equal_area_cell_area_basis': '4*pi*R^2 / ((360/degrees)*(180/degrees)); cells have equal spherical area, not equal shape or diameter.',
        'limits': ['Neither grid guarantees regional fairness or coverage of places absent from the source.',
                   'Country codes are source coverage units, not reviewed canonical identities. SSD/SDS and territory/map-unit distinctions remain unresolved here.',
                   'Capital completeness is relative to source ADM0CAP/CAPALT flags, not an independently approved capital inventory.',
                   'Point sampling says nothing about land area or population represented; national representatives can still crowd one region.',
                   'Names and record sizes are build-time payloads; compression and resident scenarios are not target memory measurements.',
                   'No editorial include/exclude override is applied; inspect actual changed candidates before policy approval.'],
        'comparisons': comparisons}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('report', type=Path)
    args = parser.parse_args()
    report(args.report, analyze(args.cache, args.output))
