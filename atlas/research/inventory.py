#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Expand pinned source concepts and capital-role review inventory, without merging entities."""
import argparse
from collections import Counter, defaultdict
from copy import deepcopy
import hashlib
import json
from pathlib import Path

from audit import json_bytes, names, number, report, select_places, valid_qid
from catalog import validate
from name_profiles import display_candidate
from reviews import apply as apply_reviews, queue as review_queue
from sources import ATLAS, locked_sources, read_source

LAYERS = ('ne_10m_admin_0_countries', 'ne_10m_admin_0_map_units', 'ne_10m_populated_places')
ALLOCATION_PATH = ATLAS/'catalog/allocations.json'


def metadata(record, rationale, evidence=None):
    return {'evidence': evidence or [record],
            'viewpoint': 'Natural Earth pinned source; no Globe adjudication',
            'time': {'basis': 'source-unspecified', 'from': None, 'to': None},
            'review': {'status': 'unreviewed', 'reviewer': None, 'decision': None},
            'rationale': rationale}


def selected_sources(cache):
    locks = {s['id']: s for s in locked_sources()}
    fixture = json.loads((ATLAS/'catalog/fixture.json').read_text())
    fixture_keys = {(r['source'], r['fields']['NE_ID']) for r in fixture['records']}
    selected = []
    for layer in LAYERS:
        _, rows = read_source(locks[layer], cache)
        for ordinal, (row, parts) in enumerate(rows, 1):
            if layer != LAYERS[2] or number(row, 'ADM0CAP') == 1 or number(row, 'CAPALT') == 1 or row['FEATURECLA'].startswith('Admin-0') or (layer, row['NE_ID']) in fixture_keys:
                selected.append((layer, ordinal, row, parts))
    return locks, selected, fixture


def allocations(locks, selected, fixture, existing=None, allow_new=False):
    expected_sources = {s: locks[s]['sha256'] for s in LAYERS}
    if existing is not None:
        if existing['schema'] != 1 or existing['sources'] != expected_sources:
            raise ValueError('allocation source revision changed; explicit reconciliation required')
        result = deepcopy(existing)
    else:
        result = {'schema': 1, 'status': 'proposed source concepts; not approved entity concordances',
                  'sources': expected_sources, 'next_numeric_id': 1000, 'allocations': []}
    if type(result['next_numeric_id']) is not int or not 1000 <= result['next_numeric_id'] <= 999999:
        raise ValueError('invalid next entity ID')
    allocated_numbers = [int(a['entity_id'].split(':')[1]) for a in result['allocations']]
    if allocated_numbers and result['next_numeric_id'] <= max(allocated_numbers):
        raise ValueError('next entity ID would recycle an allocated number')
    keys, ids = {}, set()
    for entry in result['allocations']:
        key = entry['source'], entry['ne_id']
        if key in keys or entry['entity_id'] in ids:
            raise ValueError('duplicate allocation identity')
        keys[key] = entry
        ids.add(entry['entity_id'])
    fixture_links = {a['subject']: a['object'] for a in fixture['assertions'] if a['relation'] == 'same-entity'}
    fixture_ids = {(r['source'], r['fields']['NE_ID']): fixture_links[r['id']] for r in fixture['records']}
    for layer, ordinal, row, parts in sorted(selected, key=lambda x: (x[0], int(x[2]['NE_ID']))):
        key = layer, row['NE_ID']
        if key in keys:
            entry = keys[key]
            if entry['ordinal'] != ordinal or (key in fixture_ids and entry['entity_id'] != fixture_ids[key]):
                raise ValueError('existing allocation changed')
            continue
        if not allow_new:
            raise ValueError('unallocated source concept; use explicit --allocate after reviewing scope')
        entity = fixture_ids.get(key)
        if entity is None:
            entity = 'gcat:' + str(result['next_numeric_id']).zfill(6)
            result['next_numeric_id'] += 1
        if entity in ids:
            raise ValueError('entity ID would be reused')
        entry = {'entity_id': entity, 'source': layer, 'ne_id': row['NE_ID'], 'ordinal': ordinal}
        result['allocations'].append(entry)
        keys[key] = entry
        ids.add(entity)
    required = {(layer, row['NE_ID']) for layer, ordinal, row, parts in selected}
    if set(keys) != required:
        raise ValueError('allocation roster changed; retire/reconcile explicitly, never drop IDs')
    result['allocations'].sort(key=lambda a: a['entity_id'])
    return result


def code_targets(code, countries, units):
    """Return attributed source-code candidates; never claim identity or legal sovereignty."""
    if not code or code.startswith('-'):
        return [], 'unresolved-source-code'
    exact = [r for r in countries if r['fields']['ADM0_A3'] == code]
    if exact:
        return exact, 'exact-country-source-code'
    aliases = [r for r in countries if code in {r['fields'].get('ISO_A3'), r['fields'].get('ISO_A3_EH')} and code not in {'', '-99'}]
    if aliases:
        return aliases, 'iso-code-alias-candidate'
    parents = [r for r in units if code in {r['fields'].get('GU_A3'), r['fields'].get('ISO_A3'), r['fields'].get('ISO_A3_EH')} and code not in {'', '-99'}]
    return parents, 'map-unit-source-code-candidate' if parents else 'unresolved-source-code'


def build(locks, selected, fixture, registry):
    lookup = {(a['source'], a['ne_id']): a for a in registry['allocations']}
    result = {'schema': 1, 'status': 'research-prototype', 'entities': [], 'sources': [],
              'records': [], 'assertions': [], 'names': [], 'points': []}
    for layer in LAYERS:
        result['sources'].append({'id': layer, 'sha256': locks[layer]['sha256'], 'lock': 'sources.lock.json'})
    place_rows = {}
    entity_for_record = {}
    for layer, ordinal, row, parts in selected:
        entity = lookup[layer, row['NE_ID']]['entity_id']
        record = layer + ':' + str(ordinal)
        entity_for_record[record] = entity
        settlement = layer == LAYERS[2]
        result['entities'].append({'id': entity, 'kind': 'settlement' if settlement else 'political-unit'})
        # Keep the exact source fields used by every assertion/name; no normalized source repairs.
        fields = {key: value for key, value in row.items() if key in {'NE_ID', 'ADM0_A3', 'SOV_A3', 'GU_A3', 'SU_A3', 'ISO_A3', 'ISO_A3_EH', 'UN_A3', 'TYPE', 'FEATURECLA', 'ADMIN', 'GEOUNIT', 'SOVEREIGNT', 'WIKIDATAID', 'ADM0CAP', 'CAPALT', 'LABEL_X', 'LABEL_Y', 'MIN_ZOOM', 'LABELRANK', 'POP_MAX', 'NAMEASCII'} or key == 'NAME' or key.startswith('NAME_')}
        result['records'].append({'id': record, 'source': layer, 'ordinal': ordinal, 'fields': fields})
        result['assertions'].append({'id': entity + ':source-link', 'subject': record, 'object': entity,
            'relation': 'same-entity', **metadata(record, 'Allocated source-concept proposal; not an approved cross-source identity.')})
        for name in names(row):
            if not name['value']:
                continue
            field = name['source_field']
            result['names'].append({'id': entity + ':' + field, 'subject': entity, 'value': name['value'],
                'language': name['language'], 'source_field': field, 'role': name['role'],
                **metadata(record, 'Exact source name and field; source generic language remains und.')})
        if settlement:
            point = list(parts[0][0])
            place_rows[record] = row
            candidate = display_candidate(row, 'mac_roman')
            if candidate['fallback'] and candidate['display_candidate']:
                result['names'].append({'id': entity + ':mac-roman-candidate', 'subject': entity,
                    'value': candidate['display_candidate'], 'language': 'und', 'source_field': 'NAMEASCII',
                    'role': 'display-fallback-candidate', 'loss': True, 'encoding': 'mac_roman',
                    'derived_from': entity + (':NAME_EN' if row.get('NAME_EN') else ':NAME'),
                    **metadata(record, 'Lossy source fallback candidate; no installed-glyph guarantee.')})
        else:
            point = [float(row['LABEL_X']), float(row['LABEL_Y'])]
        result['points'].append({'id': entity + ':point', 'subject': entity, 'coordinate': point,
            'role': 'settlement-location' if settlement else 'source-label', 'containment': 'unverified',
            **metadata(record, 'Source representative coordinate, not a verified interior/picking anchor.')})
    countries = [r for r in result['records'] if r['source'] == LAYERS[0]]
    units = [r for r in result['records'] if r['source'] == LAYERS[1]]
    capital_review = []
    fixture_assertions = {(a['subject'], a['object'], a['relation']): a['id'] for a in fixture['assertions']}
    for record, row in place_rows.items():
        flagged = number(row, 'ADM0CAP') == 1 or number(row, 'CAPALT') == 1
        classified = row['FEATURECLA'].startswith('Admin-0')
        if not flagged and not classified:
            continue
        targets, resolution = code_targets(row['ADM0_A3'], countries, units)
        capital_review.append({'source_record': record, 'entity_id': entity_for_record[record], 'name': row['NAME'],
            'country_source_code': row['ADM0_A3'], 'national_flag': number(row, 'ADM0CAP') == 1,
            'alternate_flag': number(row, 'CAPALT') == 1, 'source_classification': row['FEATURECLA'],
            'selection_basis': 'flag-and-class' if flagged and classified else 'flag-only' if flagged else 'class-only',
            'resolution': resolution,
            'target_source_records': [r['id'] for r in targets]})
        for target in targets:
            relation = 'capital-role' if flagged else 'source-capital-classification'
            assertion_id = fixture_assertions.get((record, target['id'], relation), record + ':' + relation + ':' + target['id'])
            qualifiers = {'ADM0CAP': row['ADM0CAP'], 'CAPALT': row['CAPALT']} if flagged else {'FEATURECLA': row['FEATURECLA']}
            result['assertions'].append({'id': assertion_id,
                'subject': record, 'object': target['id'], 'relation': relation,
                'qualifiers': qualifiers,
                **metadata(record, resolution + '; source role and jurisdiction remain unreviewed.', [record, target['id']])})
    # Retain special fixture assertions and IDs, without overwriting generated capital links.
    existing_ids = {a['id'] for a in result['assertions']}
    for assertion in fixture['assertions']:
        if assertion['relation'] not in {'same-entity', 'capital-role'} and assertion['id'] not in existing_ids:
            result['assertions'].append(deepcopy(assertion))
    for table in ('entities', 'sources', 'records', 'assertions', 'names', 'points'):
        result[table].sort(key=lambda r: r['id'])
    return result, capital_review


def summarize(catalog, capital_review, registry):
    records = catalog['records']
    political = [r for r in records if r['source'] != LAYERS[2]]
    groups = {}
    for field in ('NE_ID', 'WIKIDATAID', 'ISO_A3_EH'):
        values = defaultdict(list)
        for record in political:
            value = record['fields'].get(field)
            if value and value != '-99' and (field != 'WIKIDATAID' or valid_qid(value)):
                values[value].append(record['id'])
        groups[field] = {key: ids for key, ids in sorted(values.items()) if len(ids) > 1}
    geometry_path = ATLAS/'reports/geometry-qa.json'
    geometry = json.loads(geometry_path.read_text())
    if geometry['source_lock_sha256'] != hashlib.sha256((ATLAS/'sources.lock.json').read_bytes()).hexdigest():
        raise ValueError('point diagnostics use a different source lock')
    outside = geometry['place_country_membership']['10m']['country_label_points_outside_geometry']
    political_by_code = {r['fields']['ADM0_A3']: r['id'] for r in political if r['source'] == LAYERS[0]}
    return {'schema': 1, 'status': 'unreviewed source-concept inventory; no sovereign-state count',
        'validation': {k: v for k, v in validate(catalog).items() if k != 'warnings'},
        'unreviewed_rows': len(validate(catalog)['warnings']),
        'source_records': dict(Counter(r['source'] for r in records)),
        'source_classifications': {s: dict(sorted(Counter(r['fields'].get('TYPE', r['fields'].get('FEATURECLA', '')) for r in records if r['source'] == s).items())) for s in LAYERS},
        'source_sha256': registry['sources'],
        'capital_roster': capital_review,
        'point_review_cases': [{**case, 'source_record': political_by_code[case['adm0_a3']], 'status': 'review-required; no new spherical conclusion'} for case in outside],
        'point_diagnostic_provenance': {'report': 'reports/geometry-qa.json', 'sha256': hashlib.sha256(geometry_path.read_bytes()).hexdigest(), 'basis': 'Previously recorded planar source-geometry oracle; catalog points remain unverified.'},
        'name_languages': dict(sorted(Counter(n['language'] for n in catalog['names']).items())),
        'mac_roman_fallback_candidates': sum(n['role'] == 'display-fallback-candidate' for n in catalog['names']),
        'capital_resolution_counts': dict(Counter(r['resolution'] for r in capital_review)),
        'capital_candidate_basis_counts': dict(Counter(r['selection_basis'] for r in capital_review)),
        'source_country_codes_without_capital_candidate': sorted({r['fields']['ADM0_A3'] for r in political if r['source'] == LAYERS[0]} - {r['country_source_code'] for r in capital_review}),
        'repeated_political_source_identifiers': groups,
        'limits': ['Repeated identifiers are candidate review groups, never automatic entity equality.',
                   'Country/map-unit concepts remain separate even if the same NE_ID/QID/ISO code appears.',
                   'Source capital flags are not an independently approved inventory of current capital roles.',
                   'Source code matching proposes role targets; it does not determine sovereignty.',
                   'All name, point and relationship assertions remain unreviewed.',
                   'Full generated catalog stays outside Git; allocation registry and review summary are retained.']}


def run(cache, output, summary_path, allow_new=False, reviews=None, queue_path=None):
    locks, selected, fixture = selected_sources(cache)
    existing = json.loads(ALLOCATION_PATH.read_text()) if ALLOCATION_PATH.exists() else None
    registry = allocations(locks, selected, fixture, existing, allow_new)
    catalog, capitals = build(locks, selected, fixture, registry)
    if reviews is not None:
        catalog = apply_reviews(catalog, json.loads(reviews.read_text()))
    summary = summarize(catalog, capitals, registry)
    # Evaluate existing recipes against the broader role review roster without adopting it as
    # a mandatory selection policy. Class-only rows can be historic or erroneous source claims.
    _, all_places = read_source(locks[LAYERS[2]], cache)
    recipes = json.loads((ATLAS/'editions/recipes.json').read_text())
    record_ids = {r['id']: r['fields']['NE_ID'] for r in catalog['records']}
    summary['existing_recipe_capital_candidate_coverage'] = {}
    for edition in ('compact', 'standard', 'expanded'):
        chosen = {r['NE_ID'] for r, p, reasons in select_places(all_places, recipes[edition])}
        omitted = [r for r in capitals if record_ids[r['source_record']] not in chosen]
        summary['existing_recipe_capital_candidate_coverage'][edition] = {
            'review_roster_records': len(capitals), 'included': len(capitals) - len(omitted),
            'omitted_candidates': omitted,
            'interpretation': 'Source role/class candidate coverage, not a current-capital inventory or an approved mandatory inclusion rule.'}
    summary['review_status_counts'] = dict(Counter(r['review']['status'] for table in ('assertions', 'names', 'points') for r in catalog[table]))
    if queue_path is not None:
        queue_path.write_bytes(b''.join(json_bytes(row) for row in review_queue(catalog)))
    raw = json_bytes(catalog)
    output.write_bytes(raw)
    summary['catalog_json_bytes'] = len(raw)
    summary['catalog_sha256'] = hashlib.sha256(raw).hexdigest()
    if allow_new:
        report(ALLOCATION_PATH, registry)
    report(summary_path, summary)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('summary', type=Path)
    parser.add_argument('--allocate', action='store_true', help='Explicitly allocate new source-concept IDs; existing IDs are never changed.')
    parser.add_argument('--reviews', type=Path, default=ATLAS/'catalog/review-decisions.json')
    parser.add_argument('--review-queue', type=Path)
    args = parser.parse_args()
    run(args.cache, args.output, args.summary, args.allocate, args.reviews, args.review_queue)
