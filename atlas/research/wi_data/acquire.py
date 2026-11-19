#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Bounded, allowlisted acquisition of the EXP-WI-DATA-01 research-only samples.

Nothing here runs implicitly. `--record` writes a lock; the default mode verifies bytes against
it. Raw bytes belong in an ignored cache directory and are never committed.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import urllib.parse
import urllib.request

HERE = Path(__file__).resolve().parent
LOCK = HERE / 'sources.lock.json'
HOSTS = {'www2.census.gov', 'api.census.gov', 'publications.newberry.org', 'dnrmaps.wi.gov'}
MAX_BYTES = 8 * 1024 * 1024
ACS = ['B01003', 'B19013', 'B25077', 'B01002']
# The Data API now demands a key (a registration email, which the owner keeps out of requests), and
# the nationwide table files are 17-26 MB each. The summary-file rows are sorted by GEO_ID and the
# server honours Range, so only the Dane County tract rows are read, by bounded binary search.
ACS_BASE = 'https://www2.census.gov/programs-surveys/acs/summary_file/2022/table-based-SF/data/5YRData/'
MAX_REQUESTS = 64
DNR_BASE = ('https://dnrmaps.wi.gov/arcgis/rest/services/DW_Map_Dynamic/'
            'EN_SurfaceWater_WTM_Ext_Dynamic_L16/MapServer/5/query')
LAKES = ['LAKE MENDOTA', 'LAKE MONONA', 'LAKE WINGRA', 'LAKE WAUBESA', 'LAKE KEGONSA']
DNR_QUERY = {
    'where': 'UPPER(WATERBODY_NAME) IN (' + ','.join(f"'{n}'" for n in LAKES) + ')',
    'outFields': '*', 'returnGeometry': 'true', 'outSR': '4326', 'f': 'json'}

SOURCES = [
    {'id': 'census_cb2022_tract_wi', 'path': 'cb_2022_55_tract_500k.zip',
     'url': 'https://www2.census.gov/geo/tiger/GENZ2022/shp/cb_2022_55_tract_500k.zip',
     'version': 'Cartographic Boundary 2022, 1:500,000, tracts, Wisconsin',
     'license_status': 'unreviewed; TIGER public-domain status is an open question in docs/licensing-policy.md',
     'terms_url': 'https://www.census.gov/about/policies.html'},
    *[{'id': f'census_acs2022_{t.lower()}_dane', 'path': f'acs2022_{t.lower()}_dane_tracts.psv',
       'kind': 'ranged-psv', 'prefix': '1400000US55025',
       'url': ACS_BASE + f'acsdt5y2022-{t.lower()}.dat',
       'version': 'ACS 2018-2022 5-year, table-based summary file, ranged extract of Dane tract rows',
       'license_status': 'unreviewed; Census Bureau data, status not yet reviewed under docs/licensing-policy.md',
       'terms_url': 'https://www.census.gov/about/policies.html'} for t in ACS],
    {'id': 'newberry_ahcb_wi', 'path': 'WI_AtlasHCB.zip',
     'url': 'https://publications.newberry.org/ahcb/downloads/gis/WI_AtlasHCB.zip',
     'max_bytes': 26 * 1024 * 1024,  # smallest Wisconsin file offered (24,535,249 bytes by HEAD)
     'version': 'Atlas of Historical County Boundaries, Wisconsin GIS download',
     'license_status': 'conflicting: the download page says the data may be "reused for any lawful purpose, commercial or non-commercial, without licensing or permission fees" and calls older licences out of date, but the bundled READ ME and deed say CC BY-NC-SA 2.5 (NC is on the policy hold list). Research-only until the owner reviews',
     'terms_url': 'https://digital.newberry.org/ahcb/download'},
    {'id': 'wdnr_24k_waterbody_dane', 'path': 'wdnr_24k_waterbody_dane.json',
     'url': DNR_BASE + '?' + urllib.parse.urlencode(DNR_QUERY),
     'version': 'WDNR 24K Waterbody layer 5 (USGS 1:24,000 base, WBIC added), live service query',
     'license_status': 'unreviewed; the service states no licence',
     'terms_url': 'https://dnrmaps.wi.gov/arcgis/rest/services/DW_Map_Dynamic/EN_SurfaceWater_WTM_Ext_Dynamic_L16/MapServer'},
]

def fetch(source):
    parts = urllib.parse.urlsplit(source['url'])
    if parts.scheme != 'https' or parts.hostname not in HOSTS:
        raise ValueError(f"{source['id']}: host not allowlisted")
    request = urllib.request.Request(source['url'], headers={'User-Agent': 'globe-atlas-research/0.1'})
    with urllib.request.urlopen(request, timeout=120) as response:
        cap = source.get('max_bytes', MAX_BYTES)
        raw = response.read(cap + 1)
    if len(raw) > cap:
        raise ValueError(f"{source['id']}: exceeds {cap} byte cap")
    return raw

def _ranged(url, start, end):
    request = urllib.request.Request(url, headers={'User-Agent': 'globe-atlas-research/0.1',
                                                   'Range': f'bytes={start}-{end - 1}'})
    with urllib.request.urlopen(request, timeout=60) as response:
        if response.status != 206:
            raise ValueError('server ignored Range')
        return response.read(end - start + 1)

def _first_key_after(url, offset, size):
    chunk = _ranged(url, offset, min(offset + 4096, size))
    newline = chunk.index(b'\n') + 1
    line = chunk[newline:].split(b'\n', 1)[0]
    return newline, line.split(b'|', 1)[0].decode('ascii')

def fetch_ranged(source):
    """Header line plus every row whose GEO_ID starts with the source prefix."""
    url, prefix = source['url'], source['prefix']
    parts = urllib.parse.urlsplit(url)
    if parts.scheme != 'https' or parts.hostname not in HOSTS:
        raise ValueError(f"{source['id']}: host not allowlisted")
    head = urllib.request.Request(url, method='HEAD', headers={'User-Agent': 'globe-atlas-research/0.1'})
    with urllib.request.urlopen(head, timeout=60) as response:
        size = int(response.headers['Content-Length'])
    header = _ranged(url, 0, 512).split(b'\n', 1)[0] + b'\n'
    low, high, requests = len(header), size - 4096, 0
    while high - low > 8192:  # lowest offset whose next full line sorts at or after the prefix
        middle = (low + high) // 2
        _, key = _first_key_after(url, middle, size)
        requests += 1
        if requests > MAX_REQUESTS:
            raise ValueError('request cap')
        low, high = (middle, high) if key < prefix else (low, middle)
    rows, offset = [], low
    while True:
        chunk = _ranged(url, offset, min(offset + 65536, size))
        requests += 1
        if requests > MAX_REQUESTS:
            raise ValueError('request cap')
        lines = chunk.split(b'\n')
        complete, offset = lines[:-1], offset + len(chunk) - len(lines[-1])
        keys = [line.split(b'|', 1)[0].decode('ascii') for line in complete]
        rows += [line for line, key in zip(complete, keys) if key.startswith(prefix)]
        if any(key > prefix and not key.startswith(prefix) for key in keys) or offset >= size:
            break
    if not rows:
        raise ValueError(f"{source['id']}: no rows for {prefix}")
    source['source_file_bytes'] = size
    source['requests'] = requests
    return header + b'\n'.join(rows) + b'\n'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('cache')
    parser.add_argument('--record', action='store_true', help='download and write the lock')
    parser.add_argument('--download', action='store_true', help='download missing files, then verify')
    args = parser.parse_args()
    cache = Path(args.cache)
    cache.mkdir(parents=True, exist_ok=True)
    if args.record:
        entries = []
        for source in SOURCES:
            raw = fetch_ranged(source) if source.get('kind') == 'ranged-psv' else fetch(source)
            (cache / source['path']).write_bytes(raw)
            entries.append(dict(source, bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest(),
                                retrieved_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                                research_only=True))
        LOCK.write_text(json.dumps({'schema': 1, 'sources': entries}, indent=1) + '\n')
        return
    for entry in json.loads(LOCK.read_text())['sources']:
        path = cache / entry['path']
        if not path.exists() and args.download:
            path.write_bytes(fetch_ranged(dict(entry)) if entry.get('kind') == 'ranged-psv' else fetch(entry))
        raw = path.read_bytes()
        ok = len(raw) == entry['bytes'] and hashlib.sha256(raw).hexdigest() == entry['sha256']
        print(('ok       ' if ok else 'MISMATCH ') + entry['id'], len(raw))
        if not ok:
            raise SystemExit(1)

if __name__ == '__main__':
    main()
