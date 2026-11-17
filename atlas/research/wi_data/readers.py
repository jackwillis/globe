#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Source adapters for EXP-WI-DATA-01. Research only; reads an ignored local cache.

Each adapter returns plain dicts. The only reuse of existing code is `atlas/research/sources.py`
(strict SHP/DBF subset). Source quirks are recorded through `note_exception` so that the
experiment can count them instead of hiding them in branches.
"""
import datetime
import decimal
import json
from pathlib import Path
import struct
import sys
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import sources as ne  # noqa: E402  (existing strict SHP/DBF reader)

EXCEPTIONS = {}

def note_exception(key):
    EXCEPTIONS[key] = EXCEPTIONS.get(key, 0) + 1

# Census ACS estimate/annotation codes, from the Census page "Notes on ACS Estimate and Annotation
# Values" (https://www.census.gov/data/developers/data-sets/acs-1year/notes-on-acs-estimate-and-annotation-values.html).
ESTIMATE_CODES = {-666666666: 'insufficient-sample', -999999999: 'insufficient-cases',
                  -888888888: 'not-applicable'}
MOE_CODES = {-222222222: 'moe-insufficient-sample', -333333333: 'moe-open-interval',
             -555555555: 'moe-controlled-zero', -999999999: 'moe-insufficient-cases',
             -888888888: 'moe-not-applicable'}
# Documented top codes (median in the highest open-ended interval is reported as the bound + 1).
TOP_CODES = {'B19013': 250001, 'B25077': 2000001}

def read_acs_psv(path, table, scale=1):
    """{geoid: {'E': (scaled int or None, reason), 'M': (...)}} for one ACS table extract.

    Only `{table}_E001/M001` is read; other columns in the file are ignored. A decimal value is
    scaled to an exact integer (`scale` 10 for one decimal) and anything inexact is an error.
    """
    lines = Path(path).read_text().splitlines()
    header = lines[0].split('|')
    want = ['GEO_ID', f'{table}_E001', f'{table}_M001']
    if header[:3] != want:
        raise ValueError(f'{table}: unexpected header {header}')
    rows = {}
    for line in lines[1:]:
        cells = line.split('|')
        key = cells[0]
        if not key.startswith('1400000US'):
            raise ValueError(f'{table}: not a tract row {key}')
        geoid = key[len('1400000US'):]
        if geoid in rows:
            raise ValueError(f'{table}: duplicate {geoid}')
        rows[geoid] = {'E': _acs_value(cells[1], ESTIMATE_CODES, table, 'E', scale),
                       'M': _acs_value(cells[2], MOE_CODES, table, 'M', scale)}
    return rows

def _acs_value(text, codes, table, kind, scale):
    if text == '':
        note_exception(f'acs:{table}:empty-{kind}')
        return None, 'empty-cell'
    number = decimal.Decimal(text)
    if number == number.to_integral_value() and int(number) in codes:
        note_exception(f'acs:sentinel:{int(number)}')
        return None, codes[int(number)]
    if number <= -100000000:
        raise ValueError(f'{table}: unknown sentinel {text}')
    exact = number * scale
    if exact != exact.to_integral_value():
        raise ValueError(f'{table}: {text} is not exact at scale {scale}')
    return int(exact), None

def read_tracts(zip_path, county='025'):
    """Dane tracts: GEOID, name, rings (lon/lat degrees, NAD83 geographic) from the CB file."""
    with zipfile.ZipFile(zip_path) as archive:
        base = 'cb_2022_55_tract_500k'
        attrs = ne.dbf_records(archive.read(base + '.dbf'))
        _, shapes = ne.shp_records(archive.read(base + '.shp'))
    if len(attrs) != len(shapes):
        raise ValueError('tract DBF/SHP count mismatch')
    out = []
    for attr, shape in zip(attrs, shapes):
        if attr['COUNTYFP'] == county:
            out.append({'geoid': attr['GEOID'], 'name': attr['NAMELSAD'], 'vintage': 2020,
                        'rings': shape_rings(shape), 'aland': int(attr['ALAND']), 'awater': int(attr['AWATER'])})
    return out

def shape_rings(parts):
    """The existing reader yields one list of (lon, lat) points per part, in file order."""
    return [[tuple(p) for p in part] for part in parts]

def _dbf_loose(raw):
    """Newberry's DBF has numeric/date/float fields; the strict reader accepts only text."""
    count = struct.unpack_from('<I', raw, 4)[0]
    header, size = struct.unpack_from('<HH', raw, 8)
    fields, offset = [], 1
    for i in range(32, header - 1, 32):
        fields.append((raw[i:i + 11].rstrip(b'\0').decode('ascii'), offset, raw[i + 16]))
        offset += raw[i + 16]
    rows = []
    for k in range(count):
        row = raw[header + k * size: header + (k + 1) * size]
        if row[0] != 32:
            raise ValueError('deleted DBF record')
        rows.append({n: row[o:o + ln].decode('latin-1').strip() for n, o, ln in fields})
    return rows

def read_ahcb(zip_path):
    note_exception('ahcb:loose-dbf')
    prefix = 'WI_AtlasHCB/WI_Historical_Counties/WI_Historical_Counties'
    with zipfile.ZipFile(zip_path) as archive:
        attrs = _dbf_loose(archive.read(prefix + '.dbf'))
        _, shapes = ne.shp_records(archive.read(prefix + '.shp'))
    if len(attrs) != len(shapes):
        raise ValueError('AHCB DBF/SHP count mismatch')
    out = []
    for attr, shape in zip(attrs, shapes):
        start = parse_yyyymmdd(attr['START_DATE'])
        end = parse_yyyymmdd(attr['END_DATE'])
        out.append({'id': attr['ID'], 'name': attr['NAME'], 'full_name': attr['FULL_NAME'],
                    'fips': attr['FIPS'], 'version': int(attr['VERSION']), 'start': start, 'end': end,
                    'change': attr['CHANGE'], 'cnty_type': attr['CNTY_TYPE'], 'rings': shape_rings(shape)})
    return out

def parse_yyyymmdd(text):
    if len(text) != 8 or not text.isdigit():
        raise ValueError(f'bad date {text!r}')
    return datetime.date(int(text[:4]), int(text[4:6]), int(text[6:8]))

def read_dnr(path):
    data = json.loads(Path(path).read_text())
    if data.get('geometryType') != 'esriGeometryPolygon' or data.get('spatialReference', {}).get('wkid') != 4326:
        raise ValueError('unexpected DNR geometry type or spatial reference')
    if data.get('exceededTransferLimit'):
        raise ValueError('DNR response truncated')
    out = []
    for feature in data['features']:
        a = feature['attributes']
        out.append({'wbic': a['WATERBODY_WBIC'], 'hydroid': a['HYDROID'], 'objectid': a['OBJECTID'],
                    'name': a['WATERBODY_NAME'], 'source_year': a['ORIG_HRZ_SRC_YR'],
                    'source_scale': a['ORIG_HRZ_SRC_DNOM_AMT'],
                    'updated': datetime.datetime.fromtimestamp(a['LAST_UPDATE_DATE'] / 1000, datetime.timezone.utc).date(),
                    'rings': [[tuple(p) for p in ring] for ring in feature['geometry']['rings']]})
    return out
