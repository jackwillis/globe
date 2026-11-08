#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Add the broader source-role review roster to catalog selection, without changing recipes."""
import argparse
import json
from pathlib import Path

from audit import emit_layer, number, place_record, report, select_places
from selection import metrics
from sources import ATLAS, locked_sources, read_source


def include_role_candidates(rows, selected):
    chosen = {r['NE_ID']: set(reasons) for r, p, reasons in selected}
    for row, parts in rows:
        if row['FEATURECLA'].startswith('Admin-0'):
            chosen.setdefault(row['NE_ID'], set()).add('source-admin0-role-review')
    return [(r, p, chosen[r['NE_ID']]) for r, p in rows if r['NE_ID'] in chosen]


def role_record(row, parts, reasons):
    record = place_record(row, parts, reasons)
    record['source_role_review'] = {'classification': row['FEATURECLA'],
        'national_flag': number(row, 'ADM0CAP') == 1, 'alternate_flag': number(row, 'CAPALT') == 1,
        'status': 'unreviewed source role; not an assertion of current capital status'}
    return record


def analyze(cache, output):
    output.mkdir(parents=True, exist_ok=True)
    locks = {s['id']: s for s in locked_sources()}
    _, rows = read_source(locks['ne_10m_populated_places'], cache)
    _, countries = read_source(locks['ne_10m_admin_0_countries'], cache)
    codes = {r['ADM0_A3'] for r, p in countries}
    recipe = json.loads((ATLAS/'editions/recipes.json').read_text())
    results = {}
    for edition in ('compact', 'standard'):
        base = select_places(rows, recipe[edition])
        expanded = include_role_candidates(rows, base)
        base_ids = {r['NE_ID'] for r, p, reasons in base}
        added = []
        records = []
        for row, parts, reasons in expanded:
            record = role_record(row, parts, reasons)
            records.append(record)
            if row['NE_ID'] not in base_ids:
                added.append({'source_id': row['NE_ID'], 'name': row['NAME'], 'country_source_code': row['ADM0_A3'],
                    'source_classification': row['FEATURECLA'], 'coordinate': list(parts[0][0])})
        result = metrics(rows, expanded, codes)
        result['added_source_role_candidates'] = added
        result['base_selected_records'] = len(base)
        result['base_retained_records'] = len(base_ids & {r['NE_ID'] for r, p, reasons in expanded})
        role_ids = {r['NE_ID'] for r, p in rows if r['FEATURECLA'].startswith('Admin-0') or number(r, 'ADM0CAP') == 1 or number(r, 'CAPALT') == 1}
        result['broader_role_roster_records'] = len(role_ids)
        result['missing_role_source_ids'] = sorted(role_ids - {r['NE_ID'] for r, p, reasons in expanded})
        result['actual_jsonl'] = emit_layer(output, edition + '-role-review', sorted(records, key=lambda r: r['id']))
        base_records = sorted([role_record(r, p, reasons) for r, p, reasons in base], key=lambda r: r['id'])
        result['base_actual_jsonl'] = emit_layer(output, edition + '-base-role-schema', base_records)
        result['gzip_delta_bytes'] = result['actual_jsonl']['gzip_bytes'] - result['base_actual_jsonl']['gzip_bytes']
        result['jsonl_delta_bytes'] = result['actual_jsonl']['jsonl_bytes'] - result['base_actual_jsonl']['jsonl_bytes']
        result['payload_comparison_note'] = 'Both emitted sets include source_role_review metadata; delta includes added records and additional provenance reasons for existing source-role candidates.'
        results[edition] = result
    return {'schema': 1, 'status': 'additive inclusion experiment; no approved recipe/visibility/capital-policy change',
        'source_sha256': {key: locks[key]['sha256'] for key in ('ne_10m_populated_places', 'ne_10m_admin_0_countries')},
        'editions': results,
        'limits': ['Class-only candidates include Juba, Yangon and Johannesburg. Inclusion does not resolve their current/historical/misclassified roles.',
                   'Regional/territory Admin-0 roles remain source-attributed; they are not promoted to sovereign-state capitals.',
                   'No source field or capital flag is repaired and no existing candidate is dropped.',
                   'Catalog inclusion is distinct from visible symbols, labels, font choice and current-capital editorial decisions.']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('report', type=Path)
    args = parser.parse_args()
    report(args.report, analyze(args.cache, args.output))
