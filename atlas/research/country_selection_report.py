#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Reproduce #91 experiments from verified local archives, without network access."""
import argparse
from collections import Counter
from dataclasses import asdict, replace
import gzip
import hashlib
import json
from pathlib import Path
import platform
import zlib

from audit import select_places
from country_selection import Parameters, adapt, basis, duplicate_audit, reviewed_seeds, select
from selection import metrics
from sources import ATLAS, locked_sources, read_source


def write(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False)+'\n')


def analyze(cache, output, seed_path=None):
    output.mkdir(parents=True, exist_ok=True)
    locks = {s['id']: s for s in locked_sources()}
    lock = locks['ne_10m_populated_places']
    _, rows = read_source(lock, cache)
    _, countries = read_source(locks['ne_10m_admin_0_countries'], cache)
    places = adapt(rows)
    # Always materialize QA before scoring. Never silently collapse source IDs.
    qa = duplicate_audit(places, lock['sha256'])
    prior_path = ATLAS/'reports/concordance.json'
    prior = json.loads(prior_path.read_text())
    qa['prior_concordance_report_sha256'] = hashlib.sha256(prior_path.read_bytes()).hexdigest()
    qa['prior_concordance_cases'] = [case for case in prior['wikidata_cases']
                                     if len(case['natural_earth_records']) > 1 or
                                     any(r['coordinate_review_flag'] for r in case['natural_earth_records'])]
    qa['prior_evidence_limit'] = 'Attributed existing revision-pinned research, not newly fetched verification. Includes separately scoped map-unit records absent from settlement input.'
    write(output/'duplicate-qa.json', qa)
    approved = reviewed_seeds(json.loads(seed_path.read_text()), places, lock['sha256']) if seed_path else {}
    provisional = {p.key: 'provisional-source-capital-flag' for p in places if p.national or p.alternate}
    seeds = {**provisional, **{k: 'approved-'+v for k, v in approved.items()}}
    roles = [{'source_id': p.key, 'source_record_1_based': p.ordinal, 'country_source_code': p.country,
              'name': p.name, 'classification': p.role, 'national_flag': p.national,
              'alternate_flag': p.alternate, 'approved_role': approved.get(p.key),
              'basis_sha256': basis(p), 'review_status': 'approved' if p.key in approved else 'provisional-unreviewed',
              'interpretation': 'Source flags/class do not distinguish current national, territorial, multiple-function or historical roles.'}
             for p in places if p.national or p.alternate or p.role.startswith('Admin-0') or p.key in approved]
    write(output/'capital-inventory.json', {'source_sha256': lock['sha256'], 'records': roles})
    recipe = json.loads((ATLAS/'editions/recipes.json').read_text())['compact']
    baseline = select_places(rows, recipe)
    base_ids = {r['NE_ID'] for r, _, _ in baseline}
    codes = {r['ADM0_A3'] for r, _ in countries}
    params = Parameters()
    variants = {'A-compact-baseline': ({r['NE_ID']: {'reason': sorted(why), 'score': None, 'round': None}
                                      for r, _, why in baseline}, None)}
    for policy in ('population', 'inverse-square', 'nearest'):
        variants[policy] = (select(places, policy, seeds, params), params)
    # Same eligible pool, mandatory seeds AND per-country counts as C; no threshold advantage.
    reference = variants['inverse-square'][0]
    country_counts = Counter(p.country for p in places if p.key in reference)
    for policy in ('population', 'nearest'):
        equal = replace(params, min_score=0)
        variants[policy+'-C-country-counts'] = (select(places, policy, seeds, equal, country_limits=country_counts), equal)
    for spacing in (400, 1000):
        setting = replace(params, min_score=0, spacing_km=spacing)
        variants['nearest-'+str(spacing)+'km-C-country-counts'] = (select(places, 'nearest', seeds, setting, country_limits=country_counts), setting)
    for policy in ('population', 'inverse-square', 'nearest'):
        setting = replace(params, min_score=0)
        variants[policy+'-baseline-count'] = (select(places, policy, seeds, setting, comparison_count=len(base_ids)), setting)
    broader = {p.key: 'provisional-admin0-class-or-flag' for p in places if p.role.startswith('Admin-0') or p.national or p.alternate}
    broader.update({k: 'approved-'+v for k, v in approved.items()})
    variants['inverse-square-broader-roles'] = (select(places, 'inverse-square', broader, params), params)
    variants['inverse-square-cross-border'] = (select(places, 'inverse-square', seeds, replace(params, cross_border=True)), replace(params, cross_border=True))
    variants['inverse-square-32KiB-scenario'] = (select(places, 'inverse-square', seeds, params, budget_bytes=32768), params)
    summary = {}
    for label, (chosen, setting) in variants.items():
        chosen_rows = [(r, g, {'country-selection-research'}) for r, g in rows if r['NE_ID'] in chosen]
        result = metrics(rows, chosen_rows, codes)
        records = [{**asdict(p), **chosen[p.key]} for p in sorted(places, key=lambda p: (p.country, p.key)) if p.key in chosen]
        raw = ''.join(json.dumps(r, ensure_ascii=False, sort_keys=True, separators=(',', ':'), allow_nan=False)+'\n' for r in records).encode()
        packed = gzip.compress(raw, mtime=0)
        (output/(label+'.jsonl')).write_bytes(raw)
        (output/(label+'.jsonl.gz')).write_bytes(packed)
        result.update({'parameters': asdict(setting) if setting else None,
                       'approved_capital_recall': {'selected': len(set(chosen)&approved.keys()), 'total': len(approved),
                                                   'fraction': len(set(chosen)&approved.keys())/len(approved) if approved else None},
                       'provisional_flag_recall': {'selected': len(set(chosen)&provisional.keys()), 'total': len(provisional)},
                       'added_vs_baseline': [{'source_id': p.key, 'name': p.name, 'country': p.country} for p in places if p.key in chosen and p.key not in base_ids],
                       'removed_vs_baseline': [{'source_id': p.key, 'name': p.name, 'country': p.country} for p in places if p.key in base_ids and p.key not in chosen],
                       'serialized_research_records': {'jsonl_bytes': len(raw), 'gzip_bytes': len(packed),
                                                       'sha256': hashlib.sha256(raw).hexdigest()},
                       'largest_country_share': max(result['country_distribution'].values(), default=0)/len(chosen) if chosen else None})
        changes = {k: result[k] for k in ('added_vs_baseline', 'removed_vs_baseline')}
        write(output/(label+'-changes.json'), changes)
        for key in changes:
            result[key+'_count'] = len(result[key])
            result[key] = result[key][:20]
        result['full_changes_file'] = label+'-changes.json'
        summary[label] = result
    sensitivity = []
    settings = [('default', params)]
    for field, values in {'alpha': (.5, 1.5), 'beta': (.5, 1.5), 'strength': (.25, 4),
                          'spacing_km': (50, 200), 'smoothing_km': (10, 100),
                          'min_population': (10000, 100000), 'min_score': (.01, .1)}.items():
        settings.extend((field+'='+str(value), replace(params, **{field: value})) for value in values)
    for label, setting in settings:
        chosen = select(places, 'inverse-square', seeds, setting)
        chosen_rows = [(r, g, {'sensitivity'}) for r, g in rows if r['NE_ID'] in chosen]
        result = metrics(rows, chosen_rows, codes)
        sensitivity.append({'setting': label, 'parameters': asdict(setting), 'records': len(chosen),
                            'p95_km': result['source_settlement_distance']['p95_km'],
                            'median_km': result['source_settlement_distance']['median_km'],
                            'changed_vs_default': len(set(chosen)^set(reference)),
                            'source_ids_sha256': hashlib.sha256(('\n'.join(sorted(chosen))+'\n').encode()).hexdigest()})
    report = {'schema': 1, 'status': 'EXPERIMENTAL; must not merge without owner review; no production policy change',
              'baseline_revision': 'a0e01f3be96f6556502d62d51320a0e9c1ccc75d',
              'implementation_sha256': {name: hashlib.sha256((ATLAS/'research'/name).read_bytes()).hexdigest() for name in ('country_selection.py', 'country_selection_report.py')},
              'source': {k: {f: locks[k][f] for f in ('version', 'sha256', 'bytes')} for k in ('ne_10m_populated_places', 'ne_10m_admin_0_countries')},
              'toolchain': {'python': platform.python_version(), 'zlib': zlib.ZLIB_VERSION},
              'source_records': len(places), 'source_country_pools': len({p.country for p in places}),
              'missing_or_invalid_population': sum(p.population is None for p in places),
              'capital_inventory_records': len(roles), 'approved_seeds': len(approved),
              'duplicate_pairs': len(qa['pair_evidence']), 'duplicate_reason_counts': qa['reason_counts'],
              'comparisons': summary, 'sensitivity': sensitivity,
              'limits': ['ADM0_A3 source pools are not reviewed sovereign identities; SSD/SDS is not silently joined.',
                         'No approved capital inventory supplied; null verified recall is not 100 percent.',
                         'Source POP_MAX mixes dates and metro/city definitions. No temporal repair or population imputation.',
                         'Nearest source-settlement gaps and occupied reference cells do not measure land or population coverage.',
                         'JSONL/gzip and 32-byte+name scenario are not runtime resident or G4 performance measurements.',
                         'Cross-border experiments use simultaneous rounds with previous-round selected major cities only; no future lookahead.',
                         'Duplicate review flags are not adjudications or production exclusions.']}
    write(output/'comparison.json', report)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('cache', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--reviewed-seeds', type=Path)
    args = parser.parse_args()
    analyze(args.cache, args.output, args.reviewed_seeds)
