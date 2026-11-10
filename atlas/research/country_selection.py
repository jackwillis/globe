#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Research-only country-local settlement pools, reviewed seeds and spatial scores.

No source acquisition, identity joins, source corrections or production outputs.
"""
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
import hashlib
import itertools
import json
import math
import re
import unicodedata


@dataclass(frozen=True)
class Place:
    key: str
    country: str
    name: str
    lon: float
    lat: float
    population: float | None
    qid: str = ''
    aliases: tuple = ()
    role: str = ''
    national: bool = False
    alternate: bool = False
    ordinal: int = 0
    record_sha256: str = ''


def population(value):
    try:
        result = float(value)
        return result if math.isfinite(result) and result > 0 else None
    except (TypeError, ValueError):
        return None


def adapt(rows):
    """Ordinal is provenance in the locked archive, never an identity across revisions."""
    return [Place(r['NE_ID'], r['ADM0_A3'], r['NAME'], *p[0][0],
                  population(r.get('POP_MAX')), r.get('WIKIDATAID', ''),
                  tuple(sorted({v for k, v in r.items() if k.startswith('NAME') and v and v != '-99'})),
                  r.get('FEATURECLA', ''), r.get('ADM0CAP') == '1', r.get('CAPALT') == '1', i,
                  hashlib.sha256(json.dumps([r, p], sort_keys=True, ensure_ascii=False, separators=(',', ':')).encode()).hexdigest())
            for i, (r, p) in enumerate(rows, 1)]


def distance(a, b):
    """Spherical great-circle km; stable at poles, antipodes and the dateline."""
    x, y = math.radians(a.lat), math.radians(b.lat)
    h = math.sin((y-x)/2)**2 + math.cos(x)*math.cos(y)*math.sin(math.radians(b.lon-a.lon)/2)**2
    return 12742.0176 * math.asin(math.sqrt(max(0, min(1, h))))


def normalized(name):
    return ''.join(c for c in unicodedata.normalize('NFKD', name).casefold()
                   if c.isalnum())


def basis(place):
    return hashlib.sha256(json.dumps(asdict(place), sort_keys=True, ensure_ascii=False,
                                     separators=(',', ':')).encode()).hexdigest()


def reviewed_seeds(document, places, source_sha256):
    """Explicit research seed adapter; approval must bind to exact source and record semantics.

    Does not infer approvals from catalog allocation IDs or checkpoint acceptance.
    """
    if document.get('source_sha256') != source_sha256:
        raise ValueError('seed source digest mismatch')
    lookup = {p.key: p for p in places}
    result = {}
    for item in document.get('seeds', []):
        key = item['source_id']
        if key in result or key not in lookup:
            raise ValueError('duplicate or missing seed')
        if (item.get('status') != 'approved' or not item.get('reviewer') or
                not item.get('evidence') or not re.fullmatch(r'\d{4}-\d{2}-\d{2}', item.get('date', '')) or
                item.get('role') not in ('national', 'multiple-function', 'territorial', 'regional') or
                item.get('basis_sha256') != basis(lookup[key])):
            raise ValueError('seed requires evidence-bound explicit role review')
        result[key] = item['role']
    return result


def duplicate_audit(places, source_sha256):
    """Evidence edges, not equivalence clusters: no transitive alias inference or merging.

    Connected components are deliberately not interpreted as one real-world entity.
    """
    places = sorted(places, key=lambda p: (p.key, p.ordinal, basis(p)))
    reasons = defaultdict(set)
    ids, qids, buckets = defaultdict(list), defaultdict(list), defaultdict(list)
    for i, p in enumerate(places):
        ids[p.key].append(i)
        if re.fullmatch(r'Q[1-9][0-9]*', p.qid):
            qids[p.qid].append(i)
        # 3D Cartesian bucket side = chord upper bound for 25 km. Adjacent buckets suffice.
        x, y = math.radians(p.lon), math.radians(p.lat)
        cell = tuple(math.floor(v/25) for v in
                     (6371.0088*math.cos(y)*math.cos(x), 6371.0088*math.cos(y)*math.sin(x), 6371.0088*math.sin(y)))
        for delta in itertools.product((-1, 0, 1), repeat=3):
            for j in buckets[tuple(a+b for a, b in zip(cell, delta))]:
                other = places[j]
                km = distance(p, other)
                if km > 25:
                    continue
                pair = (j, i)
                if km <= .25:
                    reasons[pair].add('near-coincident-different-records')
                names = {normalized(n) for n in (p.name, *p.aliases)} - {''}
                other_names = {normalized(n) for n in (other.name, *other.aliases)} - {''}
                if names & other_names:
                    reasons[pair].add('nearby-normalized-name-or-alias')
                # Lexical heuristic only; POP_MAX's metro-oriented definition is a separate global caveat.
                strip = lambda n: normalized(re.sub(r'\b(metropolitan|metro|urban|area|city)\b', '', n, flags=re.I))
                if strip(p.name) and strip(p.name) == strip(other.name) and p.name != other.name:
                    reasons[pair].add('possible-city-versus-metro-name')
                if p.country != other.country and (km <= .25 or names & other_names):
                    reasons[pair].add('cross-country-attribution-review')
        buckets[cell].append(i)
    for groups, label in ((ids, 'repeated-source-id'), (qids, 'shared-source-qid')):
        for group in groups.values():
            for a, b in itertools.combinations(group, 2):
                reasons[a, b].add(label)
                if places[a].country != places[b].country:
                    reasons[a, b].add('cross-country-attribution-review')
    evidence = []
    for (a, b), why in sorted(reasons.items()):
        pa, pb = places[a], places[b]
        # Ordinal is excluded when testing identical-record duplication.
        same = {k: v for k, v in asdict(pa).items() if k != 'ordinal'} == {k: v for k, v in asdict(pb).items() if k != 'ordinal'}
        evidence.append({'records': [asdict(pa), asdict(pb)], 'distance_km': round(distance(pa, pb), 6),
                         'reasons': sorted(why), 'classification': 'definite-same-record' if same and pa.record_sha256 else
                         'probable-alias-review' if 'nearby-normalized-name-or-alias' in why else 'ambiguous-review'})
    known = {'Pyongsan', 'Pyongyang', 'Bose', 'Baicheng', 'Kuta', 'Denpasar', 'Spratly Islands', 'Wake Island', 'Jianmen', 'Ascension'}
    return {'source_sha256': source_sha256, 'pair_evidence': evidence,
            'reason_counts': dict(sorted(Counter(r for e in evidence for r in e['reasons']).items())),
            'known_anomaly_records': [asdict(p) for p in places if p.name in known],
            'known_anomaly_reference': 'atlas/reports/concordance.json and atlas/catalog/README.md; no QID repair',
            'policy': 'Pair evidence only; retain distinct identities; repeated IDs block selection. No automatic merge.'}


@dataclass(frozen=True)
class Parameters:
    alpha: float = 1.0
    beta: float = 1.0
    strength: float = 1.0
    spacing_km: float = 100.0
    smoothing_km: float = 25.0
    min_population: float = 50000.0
    min_score: float = .05
    cross_border: bool = False
    major_population: float = 1000000.0

    def validate(self):
        for k, v in asdict(self).items():
            if k != 'cross_border' and (not math.isfinite(v) or v < 0):
                raise ValueError('parameters must be finite and nonnegative')
        if self.smoothing_km <= 0 or self.spacing_km <= 0:
            raise ValueError('positive distance scales required')


def score(candidate, selected, policy, params):
    intrinsic = ((candidate.population or 0)/1000000)**params.alpha
    if policy == 'population':
        return intrinsic
    competition = [s for s in selected if s.country == candidate.country or
                   (params.cross_border and (s.population or 0) >= params.major_population)]
    if policy == 'inverse-square':
        penalty = sum(((s.population or 0)/1000000)**params.beta * params.spacing_km**2 /
                      (distance(candidate, s)**2 + params.smoothing_km**2) for s in competition)
        return intrinsic / (1 + params.strength*penalty)
    if policy == 'nearest':
        d = min((distance(candidate, s) for s in competition), default=math.inf)
        return intrinsic * min(1, d/params.spacing_km)
    raise ValueError('unknown policy')


def select(places, policy, seeds=None, params=Parameters(), budget_bytes=None, country_limits=None, comparison_count=None):
    """Simultaneous country rounds prevent alphabetical cross-border advantage.

    One proposal per source-country pool per round; scores use the same selected snapshot.
    Ties use population then lexicographic source ID, never names or input order.
    Byte budget uses explicit illustrative 32-byte+UTF8-name records, never JSON size/RAM.
    country_limits is ONLY an equal-count experiment, not an editorial quota.
    """
    params.validate()
    if policy not in ('population', 'inverse-square', 'nearest'):
        raise ValueError('unknown policy')
    lookup = {}
    for p in places:
        if p.key in lookup:
            raise ValueError('repeated source ID; inspect duplicate audit before selection')
        if not (math.isfinite(p.lon) and math.isfinite(p.lat) and abs(p.lon) <= 180 and abs(p.lat) <= 90):
            raise ValueError('invalid coordinate')
        if p.population is not None and (not math.isfinite(p.population) or p.population <= 0):
            raise ValueError('population must be positive finite or None')
        lookup[p.key] = p
    seeds = seeds or {}
    if set(seeds) - lookup.keys():
        raise ValueError('missing seed')
    cost = lambda p: 32 + len(p.name.encode('utf-8')) + 1
    selected = {k: {'reason': v, 'score': None, 'round': 0} for k, v in sorted(seeds.items())}
    if comparison_count is not None and comparison_count < len(selected):
        raise ValueError('comparison count cannot preserve seeds')
    used = sum(cost(lookup[k]) for k in selected)
    if budget_bytes is not None and used > budget_bytes:
        raise ValueError('budget cannot preserve mandatory seeds')
    pools = defaultdict(list)
    for p in sorted(places, key=lambda p: p.key):
        if p.key not in selected and p.population is not None and p.population >= params.min_population:
            pools[p.country].append(p)
    # Incremental competition: each selected/candidate pair is evaluated once.
    # Research workspace only; no implied runtime storage/algorithm contract.
    penalty = defaultdict(float)
    nearest = defaultdict(lambda: math.inf)
    pending = [lookup[k] for k in sorted(selected)]
    iteration = 0
    while True:
        iteration += 1
        snapshot = [lookup[k] for k in sorted(selected)]
        counts = Counter(p.country for p in snapshot)
        if policy != 'population':
            for country, pool in sorted(pools.items()):
                competition = [s for s in pending if s.country == country or
                               (params.cross_border and (s.population or 0) >= params.major_population)]
                for p in pool:
                    if p.key in selected:
                        continue
                    for s in competition:
                        km = distance(p, s)
                        if policy == 'inverse-square':
                            penalty[p.key] += ((s.population or 0)/1000000)**params.beta * params.spacing_km**2 / (km**2 + params.smoothing_km**2)
                        else:
                            nearest[p.key] = min(nearest[p.key], km)
        proposals = []
        for country, pool in sorted(pools.items()):
            if country_limits is not None and counts[country] >= country_limits.get(country, 0):
                continue
            def marginal(p):
                intrinsic = (p.population/1000000)**params.alpha
                if policy == 'inverse-square':
                    return intrinsic / (1 + params.strength*penalty[p.key])
                if policy == 'nearest':
                    return intrinsic * min(1, nearest[p.key]/params.spacing_km)
                return intrinsic
            ranked = [(marginal(p), p) for p in pool if p.key not in selected]
            ranked = [(s, p) for s, p in ranked if s >= params.min_score and
                      (budget_bytes is None or used + cost(p) <= budget_bytes)]
            if ranked:
                s, p = min(ranked, key=lambda item: (-item[0], -item[1].population, item[1].key))
                proposals.append((s, p))
        added = 0
        pending = []
        for s, p in sorted(proposals, key=lambda item: (-item[0], -item[1].population, item[1].key)):
            if comparison_count is not None and len(selected) >= comparison_count:
                break
            if budget_bytes is not None and used + cost(p) > budget_bytes:
                continue
            selected[p.key] = {'reason': policy, 'score': round(s, 12), 'round': iteration}
            used += cost(p)
            pending.append(p)
            added += 1
        if not added:
            break
    return selected
