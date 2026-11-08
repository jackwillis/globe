#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Small build-time assertion catalog. No inferred joins or runtime format."""
import argparse
from datetime import date
import hashlib
import json
import math
from pathlib import Path
import re

from sources import ATLAS, locked_sources, read_source

RELATIONS = {'same-entity', 'part-of', 'administrative-relationship', 'historical-successor',
             'contested-attribution', 'unresolved-candidate', 'source-sovereign-association',
             'capital-role', 'source-capital-classification', 'geometry-association', 'source-code-alias'}
STATUSES = {'unreviewed', 'accepted', 'rejected', 'deferred'}


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def validate(catalog):
    """Fail closed on structural errors; unresolved geography remains explicit warnings."""
    def require(condition, message):
        if not condition:
            raise ValueError(message)

    require(catalog['schema'] == 1 and catalog['status'] == 'research-prototype', 'catalog version/status')
    tables = {}
    all_ids = set()
    for table in ('entities', 'sources', 'records', 'assertions', 'names', 'points'):
        rows = catalog[table]
        ids = [r['id'] for r in rows]
        require(len(ids) == len(set(ids)) and not all_ids.intersection(ids), 'duplicate ID')
        all_ids.update(ids)
        tables[table] = {r['id']: r for r in rows}
    entities, sources, records = (tables[t] for t in ('entities', 'sources', 'records'))
    nodes = set(entities) | set(records)
    for entity in entities.values():
        require(re.fullmatch(r'gcat:[0-9]{6}', entity['id']) is not None, 'opaque project entity ID')
        require(entity['kind'] in {'political-unit', 'settlement'}, 'entity kind')
    for source in sources.values():
        require(re.fullmatch('[a-f0-9]{64}', source['sha256']) is not None, 'source digest')
        require(source['lock'] in {'sources.lock.json', 'references.lock.json'}, 'source lock')
        locked = json.loads((ATLAS/source['lock']).read_text())['sources']
        require(any(s['id'] == source['id'] and s['sha256'] == source['sha256'] for s in locked), 'source differs from lock')
    scopes = set()
    for record in records.values():
        require(record['source'] in sources, 'unknown record source')
        require(type(record['ordinal']) is int and record['ordinal'] > 0, 'record ordinal')
        scope = (record['source'], record['ordinal'])
        require(scope not in scopes, 'duplicate source record')
        scopes.add(scope)
        require(isinstance(record['fields'], dict) and bool(record['fields']), 'source fields')
    warnings = []

    def provenance(row):
        require(bool(row['evidence']) and set(row['evidence']) <= set(records), 'missing evidence record')
        time = row['time']
        require(time['basis'] in {'source-unspecified', 'effective-interval'}, 'temporal basis')
        bounds = []
        for field in ('from', 'to'):
            value = time[field]
            bounds.append(date.fromisoformat(value) if value is not None else None)
        if time['basis'] == 'source-unspecified':
            require(bounds == [None, None], 'unknown time must not invent dates')
        else:
            require(any(bounds), 'effective interval requires a bound')
        require(not all(bounds) or bounds[0] <= bounds[1], 'reversed interval')
        review = row['review']
        require(review['status'] in STATUSES, 'review status')
        require(bool(row['viewpoint']) and bool(row['rationale']), 'viewpoint/rationale')
        if review['status'] in {'accepted', 'rejected'}:
            require(bool(review['reviewer']) and bool(review['decision']), 'review decision required')
            require(isinstance(review.get('decided_on'), str), 'review decision date required')
            date.fromisoformat(review['decided_on'])
        if review['status'] != 'accepted':
            warnings.append(row['id'] + ': ' + review['status'])

    for assertion in tables['assertions'].values():
        require(assertion['subject'] in nodes and assertion['object'] in nodes, 'dangling assertion')
        require(assertion['relation'] in RELATIONS, 'relation type')
        require(assertion['subject'] != assertion['object'], 'self relation')
        provenance(assertion)
        if assertion['relation'] == 'capital-role':
            require('qualifiers' in assertion and set(assertion['qualifiers']) == {'ADM0CAP', 'CAPALT'}, 'capital source flags')
            require(any(assertion['qualifiers'].get(k) == '1' for k in ('ADM0CAP', 'CAPALT')), 'capital source flags')
            require(any(all(records[e]['fields'].get(k) == v for k, v in assertion['qualifiers'].items()) for e in assertion['evidence']), 'capital flags differ from evidence')
        if assertion['relation'] == 'source-capital-classification':
            qualifiers = assertion.get('qualifiers', {})
            require(set(qualifiers) == {'FEATURECLA'} and qualifiers['FEATURECLA'].startswith('Admin-0'), 'capital source classification')
            require(any(records[e]['fields'].get('FEATURECLA') == qualifiers['FEATURECLA'] for e in assertion['evidence']), 'capital classification differs from evidence')
        if assertion['relation'] == 'unresolved-candidate':
            require(assertion['review']['status'] != 'accepted', 'candidate cannot be accepted as equality')
    for name in tables['names'].values():
        require(name['subject'] in nodes, 'dangling name')
        require(bool(name['value']) and bool(name['language']) and bool(name['source_field']), 'name metadata')
        require(name['role'] in {'source', 'localized-source', 'display-fallback-candidate'}, 'name role')
        provenance(name)
        require(any(records[e]['fields'].get(name['source_field']) == name['value'] for e in name['evidence']), 'name differs from source field')
        if name['role'] == 'display-fallback-candidate':
            require(name['loss'] is True and name['derived_from'] in tables['names'], 'fallback lineage/loss')
            parent = tables['names'][name['derived_from']]
            require(parent['subject'] == name['subject'] and parent['role'] != 'display-fallback-candidate', 'fallback subject/source')
            require(bool(name['encoding']), 'fallback encoding')
            name['value'].encode(name['encoding'], errors='strict')
    for point in tables['points'].values():
        require(point['subject'] in nodes, 'dangling point')
        lon, lat = point['coordinate']
        require(all(type(v) in (float, int) and math.isfinite(v) for v in (lon, lat)) and
                -180 <= lon <= 180 and -90 <= lat <= 90, 'point bounds')
        require(point['role'] in {'source-label', 'settlement-location', 'interior-anchor-candidate'}, 'point role')
        require(point['containment'] in {'unverified', 'outside-planar-source', 'inside-planar-source'}, 'containment status')
        provenance(point)
    return {'tables': {t: len(v) for t, v in tables.items()}, 'warnings': sorted(warnings)}


def verify_records(catalog, cache):
    """Re-read locked archives and compare exact copied fields; never repair a source."""
    locks = {s['id']: s for s in locked_sources()}
    for source in catalog['sources']:
        require_source = locks[source['id']]
        if source['sha256'] != require_source['sha256']:
            raise ValueError('catalog source digest differs from lock')
        _, rows = read_source(require_source, cache)
        for record in catalog['records']:
            if record['source'] != source['id']:
                continue
            if record['ordinal'] > len(rows):
                raise ValueError('catalog record ordinal outside source')
            row, parts = rows[record['ordinal'] - 1]
            if any(row.get(k) != v for k, v in record['fields'].items()):
                raise ValueError('catalog source fields differ: ' + record['id'])
            for point in catalog['points']:
                if record['id'] not in point['evidence']:
                    continue
                if point['role'] == 'source-label':
                    expected = [float(row['LABEL_X']), float(row['LABEL_Y'])]
                elif point['role'] == 'settlement-location':
                    expected = list(parts[0][0])
                else:
                    continue  # Interior anchors need a future geometry oracle, not this adapter.
                if expected != point['coordinate']:
                    raise ValueError('point differs from source: ' + point['id'])
    return validate(catalog)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('catalog', type=Path)
    parser.add_argument('--cache', type=Path)
    args = parser.parse_args()
    catalog = json.loads(args.catalog.read_text())
    result = verify_records(catalog, args.cache) if args.cache else validate(catalog)
    print(json.dumps(result, sort_keys=True, indent=2))
