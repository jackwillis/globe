#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Model D test: can the existing atlas/research/catalog.py contract carry the Wisconsin data?

Each attempt feeds a document to the real `catalog.validate` and records the first rejection.
`validate` itself is untouched; only its lock-directory constant is pointed at a temporary directory
for the attempts that ask what would happen once the lock accepted the new sources.
"""
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import catalog  # noqa: E402

REVIEW = {'decision': None, 'reviewer': None, 'status': 'unreviewed'}
UNKNOWN = {'basis': 'source-unspecified', 'from': None, 'to': None}

def _sha(text):
    return hashlib.sha256(text.encode()).hexdigest()

def base_doc():
    """A minimal document in the catalog's own shape: one tract, one county with two versions."""
    sources = [{'id': 'syn-census', 'sha256': _sha('c'), 'lock': 'sources.lock.json'},
               {'id': 'syn-ahcb', 'sha256': _sha('a'), 'lock': 'sources.lock.json'}]
    records = [
        {'id': 'syn-census:1', 'source': 'syn-census', 'ordinal': 1, 'fields': {'NAMELSAD': 'Synthetic Tract 1'}},
        {'id': 'syn-ahcb:1', 'source': 'syn-ahcb', 'ordinal': 1, 'fields': {'NAME': 'SYN_ALPHA', 'START_DATE': '18400101'}},
        {'id': 'syn-ahcb:2', 'source': 'syn-ahcb', 'ordinal': 2, 'fields': {'NAME': 'SYN_ALPHA', 'START_DATE': '18510101'}}]
    entities = [{'id': 'gcat:900001', 'kind': 'political-unit'}, {'id': 'gcat:900002', 'kind': 'political-unit'}]
    common = {'viewpoint': 'synthetic', 'rationale': 'synthetic fixture', 'review': REVIEW}
    names = [
        dict(common, id='n1', subject='gcat:900001', value='Synthetic Tract 1', language='und', source_field='NAMELSAD',
             role='source', evidence=['syn-census:1'], time=UNKNOWN),
        dict(common, id='n2', subject='gcat:900002', value='SYN_ALPHA', language='und', source_field='NAME',
             role='source', evidence=['syn-ahcb:1'],
             time={'basis': 'effective-interval', 'from': '1840-01-01', 'to': '1850-12-31'})]
    assertions = [dict(common, id='a1', subject='syn-ahcb:1', object='syn-ahcb:2', relation='historical-successor',
                       evidence=['syn-ahcb:1'], time={'basis': 'effective-interval', 'from': '1851-01-01', 'to': None})]
    return {'schema': 1, 'status': 'research-prototype', 'sources': sources, 'records': records,
            'entities': entities, 'assertions': assertions, 'names': names, 'points': []}

def _try(doc, atlas=None):
    saved = catalog.ATLAS
    try:
        if atlas is not None:
            catalog.ATLAS = atlas
        catalog.validate(doc)
        return 'accepted'
    except Exception as error:  # the point is to record which rule fires
        return f'rejected: {error}'
    finally:
        catalog.ATLAS = saved

def attempts():
    out = []
    with tempfile.TemporaryDirectory() as tmp:
        lock = Path(tmp)
        doc = base_doc()
        lock.joinpath('sources.lock.json').write_text(json.dumps({'sources': [{'id': s['id'], 'sha256': s['sha256']} for s in doc['sources']]}))
        out.append(('identity, time-qualified names, successor assertion, once the lock lists the sources',
                    _try(doc, lock)))
        out.append(('same document against the real lock directory (sources not locked there)', _try(doc)))
        tract = copy.deepcopy(doc)
        tract['entities'][0]['kind'] = 'tract'
        out.append(('entity kind "tract"', _try(tract, lock)))
        stat = copy.deepcopy(doc)
        stat['names'].append(dict(stat['names'][0], id='n3', value='2453', role='estimate', source_field='NAMELSAD'))
        out.append(('an ACS estimate carried as a name with role "estimate"', _try(stat, lock)))
        stat2 = copy.deepcopy(doc)
        stat2['assertions'].append(dict(stat2['assertions'][0], id='a2', relation='statistic'))
        out.append(('a statistic as an assertion with relation "statistic"', _try(stat2, lock)))
        extra = copy.deepcopy(doc)
        extra['statistics'] = [{'subject': 'gcat:900001', 'value': 'garbage', 'moe': 'not a number'}]
        out.append(('a new top-level "statistics" table with invalid content', _try(extra, lock)))
        geom = copy.deepcopy(doc)
        geom['assertions'].append(dict(geom['assertions'][0], id='a3', subject='gcat:900002', object='syn-ahcb:1',
                                       relation='geometry-association'))
        out.append(('geometry carried by reference (entity to source record)', _try(geom, lock)))
    return out
