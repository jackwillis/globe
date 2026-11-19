#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""SYNTHETIC fixtures in the same raw-record shape as the real adapters produce.

Every name starts with "Synthetic", GEOID state code 99 does not exist, and no number here is a
measurement of anything. They exist so the committed tests run without licensed or unreviewed data.
"""
import datetime
from pathlib import Path

import model
import readers

D = datetime.date

def square(x0, y0, x1, y1, closed=True):
    ring = [(x0, y0), (x0, y1), (x1, y1), (x1, y0)]   # clockwise: exterior in shapefile convention
    return ring + [ring[0]] if closed else ring

def acs_files(directory):
    """Write ACS-shaped extracts: sentinels, a top-coded median and a decimal median age."""
    directory = Path(directory)
    geo = [f'99001{n:06d}' for n in range(101, 107)]
    tables = {
        'B01003': [(1000 + 111 * i, 90 + i) for i in range(6)],
        'B19013': [(40000 + 5000 * i, 4000 + 100 * i) for i in range(6)],
        'B25077': [(150000 + 20000 * i, 15000 + 500 * i) for i in range(6)],
        'B01002': [('3%d.5' % i, '2.%d' % i) for i in range(6)],
    }
    tables['B19013'][2] = (-666666666, -222222222)       # estimate not computable
    tables['B19013'][3] = (250001, -333333333)           # top-coded median, MOE not computable
    tables['B25077'][4] = (-666666666, -222222222)
    for table, rows in tables.items():
        lines = [f'GEO_ID|{table}_E001|{table}_M001']
        lines += [f'1400000US{g}|{e}|{m}' for g, (e, m) in zip(geo, rows)]
        (directory / f'acs2022_{table.lower()}_dane_tracts.psv').write_text('\n'.join(lines) + '\n')
    return geo

def records(directory):
    geo = acs_files(directory)
    tracts = [{'geoid': g, 'name': f'Synthetic Tract {i + 1}', 'vintage': 2020,
               'rings': [square(-89.40 + 0.01 * (i % 3), 43.05 + 0.01 * (i // 3), -89.39 + 0.01 * (i % 3), 43.06 + 0.01 * (i // 3))],
               'aland': 1, 'awater': 0} for i, g in enumerate(geo)]
    acs = {t: readers.read_acs_psv(Path(directory) / f'acs2022_{t.lower()}_dane_tracts.psv', t, 10 if t == 'B01002' else 1)
           for t in model.ACS_PROPS}

    def county(id_, version, start, end, rings, kind='County', note='synthetic change note'):
        return {'id': id_, 'name': id_.upper(), 'full_name': id_.upper(), 'fips': '', 'version': version,
                'start': start, 'end': end, 'change': note, 'cnty_type': kind, 'rings': rings}

    counties = [
        county('syn_alpha', 1, D(1840, 1, 1), D(1850, 12, 31), [square(-89.40, 43.0, -89.30, 43.10)]),
        county('syn_alpha', 2, D(1851, 1, 1), D(2000, 12, 31),
               [[(-89.40, 43.0), (-89.40, 43.10), (-89.25, 43.10), (-89.25, 43.06), (-89.30, 43.0), (-89.40, 43.0)]]),
        county('syn_beta', 1, D(1840, 1, 1), D(1850, 12, 31), [square(-89.30, 43.0, -89.10, 43.10)]),
        county('syn_beta', 2, D(1851, 1, 1), D(2000, 12, 31), [square(-89.25, 43.0, -89.10, 43.10)]),
        county('syn_gamma', 1, D(1840, 1, 1), D(1845, 12, 31), [square(-89.40, 43.20, -89.30, 43.30)]),
        county('syn_gamma', 2, D(1852, 1, 1), D(1860, 12, 31), [square(-89.40, 43.20, -89.30, 43.30)]),
        county('syn_prop', 1, D(1840, 1, 1), D(1850, 12, 31), [square(-89.40, 43.0, -89.30, 43.10)], kind='Proposal'),
    ]
    hole = square(-89.395, 43.0575, -89.392, 43.0595)[::-1]
    lakes = [
        {'wbic': 999001, 'hydroid': 900000001, 'objectid': 1, 'name': 'Synthetic Lake One', 'source_year': 1983,
         'source_scale': 24000, 'updated': D(2013, 4, 23), 'rings': [square(-89.398, 43.055, -89.385, 43.065), hole]},
        {'wbic': 999001, 'hydroid': 900000002, 'objectid': 2, 'name': 'Synthetic Lake One', 'source_year': 1983,
         'source_scale': 24000, 'updated': D(2013, 4, 23), 'rings': [square(-89.382, 43.055, -89.378, 43.058)]},
    ]
    return {'tracts': tracts, 'acs': acs, 'counties': counties, 'lakes': lakes, 'mode': 'synthetic'}

SYN_PROBES = dict(
    cities={'alpha': (-89.35, 43.05), 'beta': (-89.15, 43.05), 'gamma': (-89.35, 43.25), 'notch': (-89.27, 43.02)},
    capitol=(-89.395, 43.055), lake_name='Synthetic Lake One', dane_id='syn_alpha', boundary_city='alpha',
    dates=[D(1840, 1, 1), D(1845, 12, 31), D(1846, 1, 1), D(1848, 6, 1), D(1850, 12, 31), D(1851, 1, 1), D(1852, 1, 1), D(2000, 12, 31)])
