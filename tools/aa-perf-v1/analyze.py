# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Summaries of PE-AA-01 performance JSONL files. Standard library only.

  analyze.py summary FILE... [--md]    per-cell statistics, scaling, extrapolation, cross-host
  analyze.py expected FILE...          write expected.json pins from a trusted run to stdout

Cross-host ratios are produced only for cells whose input AND result digests agree, and
are descriptive: different CPUs, clocks and Python builds are not comparable by clock speed.
"""
import argparse
import json
import math
import os
import statistics
import sys

sys.dont_write_bytecode = True
import perf  # noqa: E402

FULL_MASK = perf.FULL_MASK


def read(paths):
    envs, cells, gates, ends = {}, [], [], []
    current = None
    for path in paths:
        with open(path) as f:
            for line in f:
                rec = json.loads(line)
                if rec['type'] == 'env':
                    current = rec['env']['label']
                    envs[current] = rec
                elif rec['type'] == 'cell':
                    rec['label'] = current
                    cells.append(rec)
                elif rec['type'] == 'gate':
                    gates.append(dict(rec, label=current))
                elif rec['type'] == 'end':
                    ends.append(dict(rec, label=current))
    return envs, cells, gates, ends


def stats(values):
    v = sorted(values)
    return {'n': len(v), 'min': v[0], 'median': statistics.median(v), 'mean': statistics.mean(v),
            'max': v[-1], 'stdev': statistics.pstdev(v) if len(v) > 1 else 0.0, 'values': values}


def fit(xs, ys):
    """Least squares y = a + b x, with r^2. None for fewer than two points."""
    if len(xs) < 2:
        return None
    mx, my = statistics.mean(xs), statistics.mean(ys)
    sxx = sum((x - mx) ** 2 for x in xs)
    if sxx == 0:
        return None
    b = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sxx
    a = my - b * mx
    sst = sum((y - my) ** 2 for y in ys)
    sse = sum((y - a - b * x) ** 2 for x, y in zip(xs, ys))
    return {'intercept': a, 'slope': b, 'r2': 1.0 if sst == 0 else 1 - sse / sst}


def summarize_cell(c):
    inp = c['input']
    wall = [e['wall_s'] for e in c['evals']]
    cpu = [e['cpu_s'] for e in c['evals']]
    agg = [g['wall_s'] for g in c['aggregates']]
    med = statistics.median(wall)
    base = c.get('baseline_rss_kib')
    return {'label': c['label'], 'class': c['class'], 'masked': c['masked'], 'frame': c['frame'],
            'seed': c['seed'], 'status': c['status'], 'reps': len(wall),
            'uncertain_masked': inp['uncertain_masked'], 'input_digest': inp['digest'],
            'result_digest': c['evals'][0]['digest'], 'aggregate_digest': c['aggregates'][0]['digest'],
            'build_wall_s': inp['build_wall_s'], 'build_cpu_s': inp['build_cpu_s'],
            'eval_wall_s': stats(wall), 'eval_cpu_s': stats(cpu), 'aggregate_wall_s': stats(agg),
            'ms_per_masked_px': 1000 * med / c['masked'],
            'ms_per_uncertain_px': (1000 * med / inp['uncertain_masked']) if inp['uncertain_masked'] else None,
            'masked_px_per_s': c['masked'] / med,
            'baseline_rss_kib': base, 'peak_rss_kib': c['peak_rss_kib'],
            'rss_growth_kib': (c['peak_rss_kib'] - base) if base is not None and c['peak_rss_kib'] else None,
            'rss_growth_bytes_per_frame_row': (1024 * (c['peak_rss_kib'] - base) / c['frame'])
            if base is not None and c['peak_rss_kib'] else None,
            'major_faults': (c.get('rusage_delta') or {}).get('ru_majflt'),
            'swaps': (c.get('rusage_delta') or {}).get('ru_nswap'),
            'cpu_over_wall': (statistics.median(cpu) / med) if med else None,
            'digest_check': c.get('digest_check')}


def scaling(rows):
    out = {}
    for label in sorted({r['label'] for r in rows}):
        for cls in perf.CLASSES:
            pts = sorted((r['masked'], r['eval_cpu_s']['median']) for r in rows
                         if r['label'] == label and r['class'] == cls and r['status'] == 'ok' and r['eval_cpu_s']['median'] > 0)
            if len(pts) < 2:
                continue
            xs, ys = [p[0] for p in pts], [p[1] for p in pts]
            slope_log = [math.log(ys[i + 1] / ys[i]) / math.log(xs[i + 1] / xs[i]) for i in range(len(xs) - 1)]
            out['%s|%s' % (label, cls)] = {
                'points': pts, 'linear_fit_cpu_s': fit(xs, ys), 'loglog_exponents_between_stages': slope_log,
                'cpu_s_per_px_by_stage': [y / x for x, y in pts]}
    return out


def extrapolate(rows):
    """Linear extrapolation to the certified mask; a labelled estimate, never a measurement."""
    out = {}
    for label in sorted({r['label'] for r in rows}):
        for cls in perf.CLASSES:
            ok = sorted((r for r in rows if r['label'] == label and r['class'] == cls and r['status'] == 'ok'),
                        key=lambda r: r['masked'])
            if not ok:
                continue
            top = ok[-1]
            per_px = top['eval_cpu_s']['median'] / top['masked']
            f = scaling(ok).get('%s|%s' % (label, cls), {}).get('linear_fit_cpu_s')
            out['%s|%s' % (label, cls)] = {
                'basis_masked_px': top['masked'], 'basis_cpu_s_per_px': per_px,
                'condition_74796_cpu_s_from_top_stage': per_px * FULL_MASK,
                'condition_74796_cpu_s_from_fit': (f['intercept'] + f['slope'] * FULL_MASK) if f else None,
                'thirty_two_conditions_h_from_top_stage': per_px * FULL_MASK * 32 / 3600,
                'note': 'extrapolation from %d masked px; not measured' % top['masked']}
    return out


def cross_host(rows):
    by = {}
    for r in rows:
        if r['status'] == 'ok':
            by.setdefault((r['class'], r['masked'], r['frame'], r['seed']), {})[r['label']] = r
    out = []
    for key in sorted(by):
        group = by[key]
        for a in sorted(group):
            for b in sorted(group):
                if a >= b:
                    continue
                ra, rb = group[a], group[b]
                same = (ra['input_digest'], ra['result_digest'], ra['aggregate_digest']) == \
                       (rb['input_digest'], rb['result_digest'], rb['aggregate_digest'])
                out.append({'class': key[0], 'masked': key[1], 'hosts': [a, b], 'digests_identical': same,
                            'cpu_ratio_second_over_first': (rb['eval_cpu_s']['median'] / ra['eval_cpu_s']['median'])
                            if same else None,
                            'rss_growth_kib': [ra['rss_growth_kib'], rb['rss_growth_kib']]})
    return out


def summary(paths):
    envs, cells, gates, ends = read(paths)
    rows = [summarize_cell(c) for c in cells if c.get('evals')]
    fixed = {k for k, v in envs.items() if v['params'].get('frame', 'auto') != 'auto'}
    ladder_rows = [r for r in rows if r['label'] not in fixed]
    bookkeeping = [{'label': r['label'], 'class': r['class'], 'frame_rows': r['frame'], 'masked': r['masked'],
                    'eval_wall_s_median': r['eval_wall_s']['median'],
                    'gross_us_per_frame_row': 1e6 * r['eval_wall_s']['median'] / r['frame'],
                    'peak_rss_kib': r['peak_rss_kib'], 'rss_growth_kib': r['rss_growth_kib'],
                    'note': 'gross: includes the cost of the %d masked pixels' % r['masked']}
                   for r in rows if r['label'] in fixed]
    bad = [{'label': c['label'], 'stage': c.get('stage'), 'class': c.get('class'), 'status': c['status']}
           for c in cells if c['status'] != 'ok']
    return {'environments': {k: {x: v['env'][x] for x in ('python', 'system', 'release', 'machine', 'byteorder',
            'maxsize', 'cpu_model', 'physical_memory_kib', 'int_info', 'loadavg', 'swap', 'decision_sha256')}
            for k, v in envs.items()}, 'params': {k: v['params'] for k, v in envs.items()},
            'cells': rows, 'not_ok': bad, 'gates': gates, 'ends': ends, 'scaling': scaling(ladder_rows),
            'extrapolation': extrapolate(ladder_rows), 'bookkeeping': bookkeeping,
            'cross_host': cross_host(ladder_rows)}


def markdown(s):
    lines = []
    for label in sorted(s['environments']):
        rows = sorted((r for r in s['cells'] if r['label'] == label), key=lambda r: (r['masked'], r['class']))
        lines += ['', '### %s' % label, '',
                  '| masked | class | uncertain | build s | eval wall s (min / median / max) | CPU s median | ms / masked px | aggregate s | peak RSS MiB | RSS growth MiB | status |',
                  '|---:|---|---:|---:|---|---:|---:|---:|---:|---:|---|']
        for r in rows:
            w = r['eval_wall_s']
            lines.append('| %d | %s | %d | %.3f | %.3f / %.3f / %.3f | %.3f | %.3f | %.3f | %s | %s | %s |' % (
                r['masked'], r['class'], r['uncertain_masked'], r['build_wall_s'], w['min'], w['median'], w['max'],
                r['eval_cpu_s']['median'], r['ms_per_masked_px'], r['aggregate_wall_s']['median'],
                '%.1f' % (r['peak_rss_kib'] / 1024) if r['peak_rss_kib'] else 'n/a',
                '%.1f' % (r['rss_growth_kib'] / 1024) if r['rss_growth_kib'] is not None else 'n/a', r['status']))
    lines += ['', '### Scaling (CPU seconds, median of repetitions)', '',
              '| host / class | CPU s per masked px by stage | log-log exponents | linear fit slope ms/px | R^2 |',
              '|---|---|---|---:|---:|']
    for k, v in sorted(s['scaling'].items()):
        f = v['linear_fit_cpu_s']
        lines.append('| %s | %s | %s | %s | %s |' % (
            k, ', '.join('%.2f ms' % (1000 * x) for x in v['cpu_s_per_px_by_stage']),
            ', '.join('%.2f' % x for x in v['loglog_exponents_between_stages']),
            '%.3f' % (1000 * f['slope']) if f else 'n/a', '%.5f' % f['r2'] if f else 'n/a'))
    lines += ['', '### Extrapolation to one 74,796-pixel condition (estimate, not measured)', '',
              '| host / class | basis px | CPU s per px | one condition s | 32 conditions h |', '|---|---:|---:|---:|---:|']
    for k, v in sorted(s['extrapolation'].items()):
        lines.append('| %s | %d | %.5f | %.0f | %.2f |' % (k, v['basis_masked_px'], v['basis_cpu_s_per_px'],
                     v['condition_74796_cpu_s_from_top_stage'], v['thirty_two_conditions_h_from_top_stage']))
    if s['bookkeeping']:
        lines += ['', '### Fixed-frame bookkeeping probes (not part of scaling or extrapolation)', '',
                  '| host | class | frame rows | masked | eval wall s | gross us / frame row | peak RSS MiB | RSS growth MiB |',
                  '|---|---|---:|---:|---:|---:|---:|---:|']
        for b in s['bookkeeping']:
            lines.append('| %s | %s | %d | %d | %.3f | %.2f | %s | %s |' % (
                b['label'], b['class'], b['frame_rows'], b['masked'], b['eval_wall_s_median'],
                b['gross_us_per_frame_row'], '%.1f' % (b['peak_rss_kib'] / 1024) if b['peak_rss_kib'] else 'n/a',
                '%.1f' % (b['rss_growth_kib'] / 1024) if b['rss_growth_kib'] is not None else 'n/a'))
    if s['cross_host']:
        lines += ['', '### Cross-host (identical digests only)', '', '| class | masked | hosts | digests | CPU ratio |', '|---|---:|---|---|---:|']
        for c in s['cross_host']:
            lines.append('| %s | %d | %s | %s | %s |' % (c['class'], c['masked'], ' vs '.join(c['hosts']),
                         'identical' if c['digests_identical'] else 'DIFFER',
                         '%.1f' % c['cpu_ratio_second_over_first'] if c['cpu_ratio_second_over_first'] else 'n/a'))
    if s['not_ok']:
        lines += ['', 'Cells not ok: ' + json.dumps(s['not_ok'])]
    return '\n'.join(lines) + '\n'


def expected(paths):
    envs, cells, _, _ = read(paths)
    pins = {}
    for c in cells:
        if c['status'] != 'ok' or not c.get('evals'):
            continue
        digests = ({e['digest'] for e in c['evals']}, {g['digest'] for g in c['aggregates']})
        if len(digests[0]) != 1 or len(digests[1]) != 1:
            raise SystemExit('repetitions disagree in %s' % cell_id(c))
        key = perf.cell_key(c['class'], c['masked'], c['frame'], c['seed'])
        pin = {'input_digest': c['input']['digest'], 'result_digest': next(iter(digests[0])),
               'aggregate_digest': next(iter(digests[1]))}
        if pins.setdefault(key, pin) != pin:
            raise SystemExit('two runs disagree for %s' % key)
    decision_hashes = {e['env']['decision_sha256'] for e in envs.values()}
    if len(decision_hashes) != 1:
        raise SystemExit('runs used different decision.py files')
    return {'schema': 'PE-AA-01-perf-expected-v1', 'decision_sha256': decision_hashes.pop(),
            'note': 'Digests pinned from the Linux x86-64 baseline; any host must reproduce them exactly.',
            'cells': dict(sorted(pins.items()))}


def cell_id(c):
    return '%s/%s' % (c.get('class'), c.get('masked'))


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest='cmd', required=True)
    s = sub.add_parser('summary')
    s.add_argument('files', nargs='+')
    s.add_argument('--md', action='store_true')
    e = sub.add_parser('expected')
    e.add_argument('files', nargs='+')
    args = p.parse_args(argv)
    if args.cmd == 'summary':
        out = summary(args.files)
        sys.stdout.write(markdown(out) if args.md else json.dumps(out, indent=1, sort_keys=True) + '\n')
    else:
        sys.stdout.write(json.dumps(expected(args.files), indent=1, sort_keys=True) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
