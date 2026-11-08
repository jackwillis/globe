#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Extract a deliberately small, explicitly unreviewed identity contract fixture."""
import argparse
import json
from pathlib import Path

from audit import number, report
from catalog import validate
from name_profiles import display_candidate
from sources import ATLAS, locked_sources, read_source


def build(cache):
    registry = json.loads((ATLAS/'catalog/registry.json').read_text())
    result = {'schema': 1, 'status': 'research-prototype', 'entities': [], 'sources': [],
              'records': [], 'assertions': [], 'names': [], 'points': []}
    locks = {s['id']: s for s in locked_sources()}
    countries = {}
    places = []

    def metadata(record, rationale):
        return {'evidence': [record], 'viewpoint': 'Natural Earth pinned source; no Globe adjudication',
                'time': {'basis': 'source-unspecified', 'from': None, 'to': None},
                'review': {'status': 'unreviewed', 'reviewer': None, 'decision': None},
                'rationale': rationale}

    for kind, source_id in [('political', 'ne_10m_admin_0_countries'), ('places', 'ne_10m_populated_places'), ('map_units', 'ne_10m_admin_0_map_units')]:
        source = locks[source_id]
        result['sources'].append({'id': source_id, 'sha256': source['sha256'], 'lock': 'sources.lock.json'})
        _, rows = read_source(source, cache)
        found = set()
        for ordinal, (row, parts) in enumerate(rows, 1):
            key = row['ADM0_A3'] if kind == 'political' else row['GU_A3'] if kind == 'map_units' else row['NAME']
            if key not in registry[kind]:
                continue
            if key in found:
                raise ValueError('ambiguous fixture selector: ' + key)
            found.add(key)
            entity = registry[kind][key]
            record = source_id + ':' + str(ordinal)
            result['entities'].append({'id': entity, 'kind': 'settlement' if kind == 'places' else 'political-unit'})
            fields = ['NE_ID', 'ADM0_A3', 'SOV_A3', 'WIKIDATAID', 'NAME', 'NAME_EN']
            fields += ['ADMIN', 'TYPE', 'SOVEREIGNT', 'ISO_A3', 'ISO_A3_EH', 'GU_A3', 'GEOUNIT', 'LABEL_X', 'LABEL_Y'] if kind != 'places' else ['ADM0CAP', 'CAPALT', 'FEATURECLA', 'NAMEASCII']
            result['records'].append({'id': record, 'source': source_id, 'ordinal': ordinal,
                                      'fields': {k: row[k] for k in fields if k in row}})
            result['assertions'].append({'id': entity + ':source-link', 'subject': record, 'object': entity,
                'relation': 'same-entity', **metadata(record, 'Fixture association proposal; allocated project ID is not approval of identity.')})
            for field, language in [('NAME', 'und'), ('NAME_EN', 'en')]:
                if row.get(field):
                    result['names'].append({'id': entity + ':' + field, 'subject': entity, 'value': row[field],
                        'language': language, 'role': 'source' if field == 'NAME' else 'localized-source', 'source_field': field,
                        **metadata(record, 'Exact source string, without normalization or transliteration.')})
            if kind != 'places':
                coordinate = [float(row['LABEL_X']), float(row['LABEL_Y'])]
                countries[key] = (record, row)
            else:
                coordinate = list(parts[0][0])
                places.append((record, row))
                candidate = display_candidate(row, 'mac_roman')
                if candidate['fallback'] and candidate['display_candidate']:
                    result['names'].append({'id': entity + ':mac-roman-candidate', 'subject': entity,
                        'value': candidate['display_candidate'], 'language': 'und', 'source_field': 'NAMEASCII',
                        'role': 'display-fallback-candidate', 'loss': True, 'encoding': 'mac_roman',
                        'derived_from': entity + (':NAME_EN' if row.get('NAME_EN') else ':NAME'),
                        **metadata(record, 'Source ASCII fallback candidate; installed font coverage remains unverified.')})
            result['points'].append({'id': entity + ':point', 'subject': entity, 'coordinate': coordinate,
                'role': 'settlement-location' if kind == 'places' else 'source-label',
                'containment': 'unverified', **metadata(record, 'Representative point is independent of geometry; never an interior guarantee.')})
        if found != set(registry[kind]):
            raise ValueError('missing fixture selectors: ' + str(set(registry[kind]) - found))
    for subject_code, object_code, relation in [('GRL', 'DNK', 'source-sovereign-association'), ('PSX', 'ISR', 'contested-attribution')]:
        subject, subject_row = countries[subject_code]
        target, target_row = countries[object_code]
        if subject_row['SOV_A3'] != target_row['SOV_A3']:
            raise ValueError('explicit fixture source association changed')
        result['assertions'].append({'id': subject + ':review-association', 'subject': subject, 'object': target,
            'relation': relation, **metadata(subject, 'Explicit fixture proposal based on shared SOV_A3 namespace; preserve source viewpoint, not legal sovereignty.'),
            'evidence': [subject, target]})
    for code, (record, row) in countries.items():
        target = countries.get(row['SOV_A3'])
        if target and target[0] != record:
            result['assertions'].append({'id': record + ':sovereign-field', 'subject': record, 'object': target[0],
                'relation': 'source-sovereign-association', **metadata(record, 'SOV_A3 source association, not a legal part-of assertion.')})
    for record, row in places:
        # A code alias is a geometry association proposal, never an identity merge.
        if row['ADM0_A3'] == 'SSD':
            result['assertions'].append({'id': record + ':code-alias', 'subject': record, 'object': countries['SDS'][0],
                'relation': 'source-code-alias', **metadata(record, 'SSD place code / SDS country ADM0 code; confirm namespace before geometry join.')})
        if number(row, 'ADM0CAP') == 1 or number(row, 'CAPALT') == 1:
            code = 'SDS' if row['ADM0_A3'] == 'SSD' else row['ADM0_A3']
            if code in countries:
                result['assertions'].append({'id': record + ':capital-role', 'subject': record, 'object': countries[code][0],
                    'relation': 'capital-role', 'qualifiers': {'ADM0CAP': row['ADM0CAP'], 'CAPALT': row['CAPALT']},
                    **metadata(record, 'Source capital flags with unspecified effective date; role and country association require independent review.')})
        for other, other_row in places:
            if record < other and row.get('WIKIDATAID', '').startswith('Q') and row['WIKIDATAID'] == other_row.get('WIKIDATAID'):
                result['assertions'].append({'id': record + ':candidate:' + other, 'subject': record, 'object': other,
                    'relation': 'unresolved-candidate', **metadata(record, 'Shared source QID is a review trigger, never sufficient equality evidence.')})
    map_records = [(record, row) for code, (record, row) in countries.items() if code in registry['map_units']]
    for record, row in map_records:
        for other, other_row in map_records:
            if record < other and row.get('WIKIDATAID') == other_row.get('WIKIDATAID'):
                result['assertions'].append({'id': record + ':candidate:' + other, 'subject': record, 'object': other,
                    'relation': 'unresolved-candidate', **metadata(record, 'Spratly/Wake shared QID is a known review case; retain both source records.'),
                    'evidence': [record, other]})
    for table in ('entities', 'sources', 'records', 'assertions', 'names', 'points'):
        result[table].sort(key=lambda r: r['id'])
    validate(result)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    report(args.output, build(args.cache))
