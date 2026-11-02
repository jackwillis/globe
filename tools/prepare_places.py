#!/usr/bin/env python3
"""Fetch a small Wikidata entity snapshot, or rebuild the runtime asset offline."""
import argparse
import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import time
import unicodedata
import urllib.parse
import urllib.request

IDS = ['Q90', 'Q84', 'Q60', 'Q1297', 'Q1490', 'Q85', 'Q3870', 'Q5465',
       'Q8678', 'Q3130', 'Q1353', 'Q956', 'Q1486']


def candidates(entity, prop):
    claims = [c for c in entity.get('claims', {}).get(prop, [])
              if c.get('rank') != 'deprecated' and c.get('mainsnak', {}).get('snaktype') == 'value']
    preferred = [c for c in claims if c.get('rank') == 'preferred']
    return preferred or claims


def value(claim):
    return claim['mainsnak']['datavalue']['value']


def point_time(claim):
    dates = claim.get('qualifiers', {}).get('P585', [])
    return next((d['datavalue']['value']['time'] for d in dates if d.get('snaktype') == 'value'), '')


def ascii_text(text, limit):
    text = ' '.join(unicodedata.normalize('NFKD', text).encode('ascii', 'replace').decode().split())
    return text[:limit]


def write_asset(snapshot, output):
    records = snapshot['places']
    if not 1 <= len(records) <= 1024:
        raise ValueError('invalid record count')
    date = snapshot['retrieved']
    if datetime.date.fromisoformat(date).isoformat() != date:
        raise ValueError('expected ISO snapshot date')
    seen = set()
    data = bytearray(b'PLNTPOI\0' + struct.pack('>II', 1, len(records)) + date.encode('ascii'))
    for record in records:
        lon, lat = record['longitude'], record['latitude']
        if not math.isfinite(lon + lat) or not -180 <= lon <= 180 or not -90 <= lat <= 90:
            raise ValueError('invalid Earth coordinate')
        if not re.fullmatch(r'Q[1-9][0-9]{0,8}', record['id']) or record['id'] in seen:
            raise ValueError('invalid or duplicate QID')
        seen.add(record['id'])
        qid = int(record['id'][1:])
        if record['wikipedia_url'] and not record['wikipedia_url'].startswith('https://en.wikipedia.org/wiki/'):
            raise ValueError('expected an English Wikipedia URL')
        data.extend(struct.pack('>IIII', qid, record['revision'], round((lon + 180) * 1e6), round((lat + 90) * 1e6)))
        for key, limit in [('label', 63), ('description', 191), ('population_text', 95), ('wikipedia_url', 255)]:
            text = ascii_text(record[key], limit) if key != 'wikipedia_url' else record[key]
            encoded = text.encode('ascii')
            if len(encoded) > limit or (key == 'label' and not encoded):
                raise ValueError('invalid text field')
            data.extend(struct.pack('>H', len(encoded)) + encoded)
    output.write_bytes(data)


def fetch(cache, date):
    cache.mkdir(parents=True, exist_ok=True)
    places = []
    for qid in IDS:
        url = f'https://www.wikidata.org/wiki/Special:EntityData/{qid}.json'
        path = cache / f'{qid}.json'
        if not path.exists():
            request = urllib.request.Request(url, headers={'User-Agent': 'PlanetLab/0.1 (offline educational globe snapshot)', 'Accept': 'application/json'})
            with urllib.request.urlopen(request, timeout=25) as response:
                raw = response.read(8 * 1024 * 1024 + 1)
            if len(raw) > 8 * 1024 * 1024:
                raise ValueError('entity exceeds size limit')
            path.write_bytes(raw)
            time.sleep(0.3)
        raw = path.read_bytes()
        entity = json.loads(raw)['entities'][qid]
        coords = [c for c in candidates(entity, 'P625') if value(c).get('globe') == 'http://www.wikidata.org/entity/Q2']
        if not coords:
            raise ValueError(f'{qid}: no Earth coordinate')
        coordinate = coords[0]
        populations = [c for c in candidates(entity, 'P1082')
                       if value(c).get('unit') == '1' and not point_time(c).startswith('-')
                       and (not point_time(c) or point_time(c)[1:11] <= date)]
        population = max(populations, key=point_time) if populations else None
        pop_text = 'Not recorded in this snapshot'
        if population:
            amount = float(value(population)['amount'])
            if not math.isfinite(amount) or amount < 0 or amount > 1e12:
                raise ValueError('invalid population')
            stamp = point_time(population)
            pop_text = f'{amount:,.0f}' + (f' ({stamp[1:5]})' if stamp else ' (date unspecified)')
        title = entity.get('sitelinks', {}).get('enwiki', {}).get('title')
        wikipedia = 'https://en.wikipedia.org/wiki/' + urllib.parse.quote(title.replace(' ', '_'), safe='()/') if title else ''
        places.append({'id': qid, 'revision': entity['lastrevid'], 'modified': entity.get('modified'),
                       'entity_url': url, 'revision_url': f'https://www.wikidata.org/w/index.php?title={qid}&oldid={entity["lastrevid"]}', 'source_sha256': hashlib.sha256(raw).hexdigest(),
                       'label': entity['labels']['en']['value'],
                       'description': entity.get('descriptions', {}).get('en', {}).get('value', ''),
                       'longitude': value(coordinate)['longitude'], 'latitude': value(coordinate)['latitude'],
                       'population_text': pop_text, 'wikipedia_url': wikipedia,
                       'coordinate_statement': coordinate, 'population_statement': population})
        print(qid, places[-1]['label'], pop_text, flush=True)
    return {'schema': 1, 'retrieved': date, 'license': 'CC0-1.0', 'places': places}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fetch', action='store_true')
    parser.add_argument('--cache', type=Path, default=Path('/tmp/planet-wikidata-cache'))
    parser.add_argument('--date', default=datetime.date.today().isoformat())
    parser.add_argument('--snapshot', type=Path, default=Path('data/wikidata/places.json'))
    parser.add_argument('--output', type=Path, default=Path('data/places.plnt'))
    args = parser.parse_args()
    if args.fetch:
        snapshot = fetch(args.cache / args.date, args.date)
        args.snapshot.write_text(json.dumps(snapshot, ensure_ascii=False, indent=2) + '\n')
    else:
        snapshot = json.loads(args.snapshot.read_text())
    write_asset(snapshot, args.output)
    print(f'{len(snapshot["places"])} places -> {args.output}')


if __name__ == '__main__':
    main()
