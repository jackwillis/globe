#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Shared build-time semantics for EXP-WI-DATA-01: identity, claims, time, provenance.

This is the one thing every architectural model needs (prediction P4). Entity references are
prototype-only synthetic integers in the reserved test range of draft ADR 0005; nothing here
is a production encoding. Source IDs never become entity identity.
"""
import datetime
import re

SYNTHETIC_BASE = 0xF0000000
EPOCH = datetime.date(1970, 1, 1)
UNBOUNDED_FROM, UNBOUNDED_TO = -2**31, 2**31 - 1

KIND_TRACT, KIND_COUNTY, KIND_WATERBODY, KIND_PROPOSAL = 1, 2, 3, 4

# prop id -> (name, value kind, scale, unit, source-code table, top code)
PROPS = {
    1: ('name', 'text', 1, '', None, None),
    2: ('population', 'int', 1, 'persons', 'acs', None),
    3: ('median household income', 'int', 1, 'USD (2022 inflation-adjusted)', 'acs', 250001),
    4: ('median home value', 'int', 1, 'USD', 'acs', 2000001),
    5: ('median age', 'int', 10, 'years', 'acs', None),
    6: ('boundary', 'geom', 1, 'microdegrees', None, None),
    7: ('source feature id', 'text', 1, '', None, None),
    8: ('change note', 'text', 1, '', None, None),
}
ACS_PROPS = {'B01003': 2, 'B19013': 3, 'B25077': 4, 'B01002': 5}

# Source table. Index is the on-disk source id in every model.
SOURCES = [
    ('census-cb2022-tract', 'Census Cartographic Boundary 2022 tracts 1:500k', 'GENZ2022', 'unreviewed'),
    ('census-acs2022-5yr', 'Census ACS 2018-2022 5-year table-based summary file', '2022', 'unreviewed'),
    ('newberry-ahcb-wi', 'Newberry Atlas of Historical County Boundaries, Wisconsin', '2016-12-15', 'conflicting'),
    ('wdnr-24k-waterbody', 'WDNR 24K Waterbody (USGS 1:24,000 base)', 'service 2026-11-16', 'unreviewed'),
]
SRC_TRACT, SRC_ACS, SRC_AHCB, SRC_DNR = range(4)

ID_PATTERNS = {'geoid': r'[0-9]{11}', 'ahcb-id': r'[a-z][a-z0-9_]{1,40}', 'hydroid': r'[0-9]{1,12}'}

def days(date):
    return (date - EPOCH).days

def from_days(n):
    return EPOCH + datetime.timedelta(days=n)

# ACS 2018-2022 5-year estimates describe a reference period, not an instant (half-open day numbers).
ACS_PERIOD = (days(datetime.date(2018, 1, 1)), days(datetime.date(2023, 1, 1)))

class Registry:
    """Source-qualified keys -> stable synthetic entity references. Never reuses a reference."""

    def __init__(self, existing=None):
        self.refs = dict(existing or {})
        self.kinds = {}

    @staticmethod
    def key(namespace, id_type, scope, value):
        if id_type not in ID_PATTERNS or re.fullmatch(ID_PATTERNS[id_type], value) is None:
            raise ValueError(f'malformed {id_type}: {value!r}')
        return f'{namespace}:{id_type}@{scope}:{value}'

    def allocate(self, keyed):
        """keyed: iterable of (key, kind). Sorted allocation makes the result input-order independent."""
        seen = {}
        for key, kind in keyed:
            if key in seen and seen[key] != kind:
                raise ValueError(f'ambiguous entity kind for {key}')
            seen[key] = kind
        next_ref = max(self.refs.values(), default=SYNTHETIC_BASE) + 1
        for key in sorted(seen):
            if key not in self.refs:
                self.refs[key] = next_ref
                next_ref += 1
            self.kinds[self.refs[key]] = seen[key]
        return self.refs

def _county_kind(c):
    """AHCB CNTY_TYPE separates real counties from unsuccessful proposals; proposals never answer
    'which county contained P', but their polygons and provenance are kept."""
    return KIND_COUNTY if c['cnty_type'] == 'County' else KIND_PROPOSAL

def claim(entity, prop, value=None, *, status='ok', moe=None, moe_status='ok', censored=False,
          valid_from=None, valid_to=None, horizon=False, source, evidence):
    """One attributed statement. valid_* are half-open day numbers; None is unbounded."""
    if prop not in PROPS or source >= len(SOURCES):
        raise ValueError('unknown property or source')
    lo = UNBOUNDED_FROM if valid_from is None else valid_from
    hi = UNBOUNDED_TO if valid_to is None else valid_to
    if lo >= hi:
        raise ValueError('inverted or empty validity interval')
    if status != 'ok' and value is not None:
        raise ValueError('a value with a missing-status reason')
    if moe is not None and value is None:
        raise ValueError('margin of error without an estimate')
    return {'entity': entity, 'prop': prop, 'value': value, 'status': status, 'moe': moe,
            'moe_status': moe_status, 'censored': censored, 'from': lo, 'to': hi,
            'horizon': horizon, 'source': source, 'evidence': evidence}

def check_temporal(claims):
    """Per (entity, prop): no overlap; gaps are returned, not rejected."""
    gaps = []
    groups = {}
    for c in claims:
        if c['from'] != UNBOUNDED_FROM or c['to'] != UNBOUNDED_TO:
            groups.setdefault((c['entity'], c['prop']), []).append(c)
    for key, items in groups.items():
        items.sort(key=lambda c: c['from'])
        for a, b in zip(items, items[1:]):
            if a['to'] > b['from']:
                raise ValueError(f'overlapping validity for {key}')
            if a['to'] < b['from']:
                gaps.append((key, a['to'], b['from']))
    return gaps

def check_contradictions(claims):
    """Two different values for one (entity, prop, interval, source) is a source conflict."""
    seen = {}
    for c in claims:
        key = (c['entity'], c['prop'], c['from'], c['to'], c['source'])
        value = (c['value'] if c['prop'] != 6 else None, c['status'], c['moe'])
        if key in seen and seen[key] != value:
            raise ValueError(f'contradictory claims for {key}')
        seen[key] = value

def build_claims(tracts, acs, counties, lakes):
    """Real-source adapter. Returns (registry, claims, bookkeeping). Quirks go to readers.EXCEPTIONS."""
    import readers
    registry = Registry()
    keyed = []
    for t in tracts:
        keyed.append((Registry.key('census', 'geoid', str(t['vintage']), t['geoid']), KIND_TRACT))
    for c in counties:
        keyed.append((Registry.key('ahcb', 'ahcb-id', '', c['id']), _county_kind(c)))
    for lake in lakes:
        keyed.append((Registry.key('wdnr', 'hydroid', '', str(lake['hydroid'])), KIND_WATERBODY))
    registry.allocate(keyed)
    claims = []
    for t in tracts:
        ref = registry.refs[Registry.key('census', 'geoid', str(t['vintage']), t['geoid'])]
        claims.append(claim(ref, 1, t['name'], source=SRC_TRACT, evidence=f"geoid {t['geoid']}"))
        claims.append(claim(ref, 6, t['rings'], source=SRC_TRACT, evidence=f"geoid {t['geoid']}"))
        for table, prop in ACS_PROPS.items():
            cell = acs[table][t['geoid']]
            (e, e_reason), (m, m_reason) = cell['E'], cell['M']
            top = PROPS[prop][5]
            censored = top is not None and e == top and m_reason == 'moe-open-interval'
            if e_reason is None and m_reason is not None and not censored:
                readers.note_exception(f'acs:{table}:moe-missing-with-estimate:{m_reason}')
            claims.append(claim(ref, prop, e, status=e_reason or 'ok', moe=m,
                                moe_status=m_reason or 'ok', censored=censored,
                                valid_from=ACS_PERIOD[0], valid_to=ACS_PERIOD[1], source=SRC_ACS, evidence=f"geoid {t['geoid']} {table}"))
    versions = {}
    for c in counties:
        ref = registry.refs[Registry.key('ahcb', 'ahcb-id', '', c['id'])]
        horizon = c['end'] == datetime.date(2000, 12, 31)
        if horizon:
            readers.note_exception('ahcb:end-2000-12-31-is-data-horizon')
        if c['cnty_type'] != 'County':
            readers.note_exception(f"ahcb:cnty-type:{c['cnty_type']}")
        # Source dates are closed day ranges; the canonical form is half-open.
        valid = dict(valid_from=days(c['start']), valid_to=days(c['end']) + 1, horizon=horizon)
        claims.append(claim(ref, 6, c['rings'], source=SRC_AHCB, evidence=f"{c['id']} v{c['version']}", **valid))
        claims.append(claim(ref, 8, c['change'], source=SRC_AHCB, evidence=f"{c['id']} v{c['version']}", **valid))
        claims.append(claim(ref, 1, c['full_name'] or c['name'], source=SRC_AHCB, evidence=f"{c['id']} v{c['version']}", **valid))
        versions[ref] = versions.get(ref, 0) + 1
    for lake in lakes:
        ref = registry.refs[Registry.key('wdnr', 'hydroid', '', str(lake['hydroid']))]
        claims.append(claim(ref, 1, lake['name'], source=SRC_DNR, evidence=f"hydroid {lake['hydroid']}"))
        claims.append(claim(ref, 6, lake['rings'], source=SRC_DNR, evidence=f"hydroid {lake['hydroid']}"))
        claims.append(claim(ref, 7, f"WBIC {lake['wbic']}", source=SRC_DNR, evidence=f"hydroid {lake['hydroid']}"))
    gaps = check_temporal(claims)
    check_contradictions(claims)
    return registry, claims, {'temporal_gaps': gaps, 'county_versions': versions}
