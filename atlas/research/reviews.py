#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Evidence-bound review overlay; decisions never mutate source fields or relation meaning."""
from copy import deepcopy
from datetime import date
import hashlib

from audit import json_bytes
from catalog import validate

TABLES = ('assertions', 'names', 'points')


def fingerprint(catalog, table, row):
    if table not in TABLES:
        raise ValueError('unsupported review table')
    records = {r['id']: r for r in catalog['records']}
    sources = {s['id']: s for s in catalog['sources']}
    nodes = {r['id']: r for r in catalog['entities'] + catalog['records']}
    payload = {key: value for key, value in row.items() if key != 'review'}
    evidence = [records[key] for key in sorted(row['evidence'])]
    lineage = []
    if table == 'names' and row.get('derived_from'):
        parent = next(n for n in catalog['names'] if n['id'] == row['derived_from'])
        lineage.append({key: value for key, value in parent.items() if key != 'review'})
    context = {'lineage': lineage, 'table': table, 'claim': payload, 'evidence': evidence,
               'sources': [sources[key] for key in sorted({r['source'] for r in evidence})],
               'nodes': [nodes[key] for key in sorted({row['subject']} | ({row['object']} if 'object' in row else set()))]}
    return hashlib.sha256(json_bytes(context)).hexdigest()


def apply(catalog, overlay):
    if overlay['schema'] != 1:
        raise ValueError('review overlay version')
    result = deepcopy(catalog)
    rows = {(table, row['id']): row for table in TABLES for row in result[table]}
    seen = set()
    for decision in overlay['decisions']:
        if set(decision) != {'table', 'target_id', 'basis_sha256', 'status', 'reviewer', 'decision', 'decided_on'}:
            raise ValueError('unexpected/missing review decision fields')
        key = decision['table'], decision['target_id']
        if key in seen:
            raise ValueError('duplicate review decision; resolve explicitly')
        seen.add(key)
        if key not in rows:
            raise ValueError('review target missing')
        row = rows[key]
        if decision['basis_sha256'] != fingerprint(result, key[0], row):
            raise ValueError('stale review evidence: ' + decision['target_id'])
        if decision['status'] not in {'accepted', 'rejected', 'deferred'}:
            raise ValueError('explicit review status required')
        if not decision['reviewer'] or not decision['decision']:
            raise ValueError('named review decision required')
        date.fromisoformat(decision['decided_on'])
        row['review'] = {field: decision[field] for field in ('status', 'reviewer', 'decision', 'decided_on')}
    # In particular, accepting an unresolved candidate is invalid: proposing a typed same-entity
    # assertion with its own evidence is a separate operation, not a status toggle.
    validate(result)
    return result


def queue(catalog):
    """Relationship/point/fallback queue; exact localized source strings need not flood review."""
    rows = []
    for table in TABLES:
        for row in catalog[table]:
            if table == 'names' and row['role'] != 'display-fallback-candidate':
                continue
            rows.append({'table': table, 'target_id': row['id'], 'basis_sha256': fingerprint(catalog, table, row),
                         'current_review': row['review'],
                         'claim': {key: value for key, value in row.items() if key != 'review'}})
    return rows
