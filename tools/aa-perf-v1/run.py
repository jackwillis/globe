# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Command line for the PE-AA-01 performance pilot. Standard library only.

  run.py selftest                    PRNG vectors, decision.py hash, stage A digests
  run.py preflight                   environment, load and swap report; no workload
  run.py ladder --label NAME ...     bounded ladder; one child process per cell
  run.py cell ...                    internal: one cell, one JSON line on stdout

Each cell runs in a fresh child so its peak resident memory is its own. Limits are
enforced three ways: a 1 Hz in-child guard, RLIMIT_CPU, and a parent wall timeout.
Abort: Ctrl-C, `touch <out>.ABORT`, or kill the parent; completed cells stay in the JSONL.
"""
import argparse
import json
import os
import subprocess
import sys
import time

sys.dont_write_bytecode = True
import perf  # noqa: E402

HERE = perf.HERE
EXPECTED = os.path.join(HERE, 'expected.json')
SAFETY = 1.25            # projection margin against super-linear growth


def load_expected(path):
    try:
        with open(path) as f:
            return json.load(f)
    except (OSError, ValueError):
        return None


def parse_stages(text):
    stages = [s.strip().upper() for s in text.split(',') if s.strip()]
    if not stages or any(s not in perf.STAGES for s in stages):
        raise SystemExit('stages must be drawn from A,B,C,D')
    return stages


def parse_classes(text):
    classes = [c.strip() for c in text.split(',') if c.strip()]
    if not classes or any(c not in perf.CLASSES for c in classes):
        raise SystemExit('classes must be drawn from ' + ','.join(perf.CLASSES))
    return classes


def cell_wall(rec):
    """Total wall seconds a cell used: build plus every repetition."""
    return (rec['input']['build_wall_s'] + rec['input']['digest_wall_s']
            + sum(e['wall_s'] for e in rec['evals']) + sum(g['wall_s'] for g in rec['aggregates']))


def project_next(prev_cells, next_masked, reps, limits, elapsed):
    """Linear projection with a safety margin. Returns (advance, reason, projected_total_s)."""
    good = [c for c in prev_cells if c['status'] == 'ok' and c['evals']]
    if not good or len(good) != len(prev_cells):
        return False, 'previous stage has a cell that is not ok', 0.0
    total = 0.0
    for c in good:
        done = len(c['evals'])
        per_px_build = (c['input']['build_wall_s'] + c['input']['digest_wall_s']) / c['masked']
        per_px_rep = sum(e['wall_s'] + g['wall_s'] for e, g in zip(c['evals'], c['aggregates'])) / done / c['masked']
        projected = (per_px_build + per_px_rep * reps) * next_masked * SAFETY
        total += projected
        if limits['cell_wall_s'] and projected > limits['cell_wall_s']:
            return False, 'projected %s cell %.0f s exceeds the %d s cell limit' % (
                c['class'], projected, limits['cell_wall_s']), total
        if limits['mem_kib'] and c.get('peak_rss_kib') and c.get('baseline_rss_kib') is not None:
            grown = (c['peak_rss_kib'] - c['baseline_rss_kib']) / c['masked'] * next_masked * SAFETY
            if c['baseline_rss_kib'] + grown > limits['mem_kib']:
                return False, 'projected %s peak %.0f MiB exceeds the %d MiB limit' % (
                    c['class'], (c['baseline_rss_kib'] + grown) / 1024, limits['mem_kib'] // 1024), total
    if limits['total_wall_s'] and elapsed + total > limits['total_wall_s']:
        return False, 'projected total %.0f s exceeds the %d s ladder budget' % (
            elapsed + total, limits['total_wall_s']), total
    return True, 'within limits', total


def run_child(args_list, wall_s):
    """Run one cell in a fresh interpreter; the parent timeout is a backstop."""
    cmd = [sys.executable, '-B', os.path.join(HERE, 'run.py'), 'cell'] + args_list
    try:
        done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                              timeout=(wall_s + 30) if wall_s else None)
    except subprocess.TimeoutExpired:
        return {'type': 'cell', 'status': 'parent_timeout'}
    out = done.stdout.decode('ascii', 'replace').strip().splitlines()
    if done.returncode == 0 and out:
        try:
            return json.loads(out[-1])
        except ValueError:
            pass
    return {'type': 'cell', 'status': 'child_failed', 'returncode': done.returncode,
            'stderr_tail': done.stderr.decode('ascii', 'replace')[-400:]}


def swap_used(info):
    return None if not info else info.get('used_kib')


def swap_problem(base, args):
    """Reason to stop, or None. An unreadable swap figure is a stop, never a pass."""
    now_used = swap_used(perf.swap_info())
    if base is None:  # only reachable with --allow-unknown-swap: the owner checks by hand
        return None
    if now_used is None:
        return 'swap reading became unavailable'
    if now_used - base > args.swap_mib * 1024:
        return 'swap use grew %d KiB' % (now_used - base)
    return None


def ladder(args, runner=None, now=time.monotonic):
    runner = runner or run_child
    stages, classes = parse_stages(args.stages), parse_classes(args.classes)
    limits = {'cell_wall_s': args.cell_wall_s, 'mem_kib': args.mem_mib * 1024, 'total_wall_s': args.total_wall_s}
    env = perf.capture_env(args.label)
    load = env['loadavg']
    if load and load[0] > args.max_load and not args.allow_busy:
        print('host is busy (1-min load %.2f > %.2f); wait, or pass --allow-busy' % (load[0], args.max_load))
        return 2
    os.makedirs(os.path.dirname(os.path.abspath(args.out)) or '.', exist_ok=True)
    abort_file = args.out + '.ABORT'
    swap0 = swap_used(env['swap'])
    if swap0 is None and not args.allow_unknown_swap:
        print('swap in use cannot be read here (swap: %r); refusing to run unmonitored. '
              'Check swap by hand and pass --allow-unknown-swap to override.' % (env['swap'],))
        return 2
    start = now()
    exit_code, stopped, completed = 0, None, []
    with open(args.out, 'a') as out:
        def emit(rec):
            out.write(json.dumps(rec, sort_keys=True) + '\n')
            out.flush()
        emit({'type': 'env', 'env': env, 'params': {'stages': stages, 'classes': classes, 'reps': args.reps,
              'seed': args.seed, 'limits': limits, 'frame': args.frame, 'cpu_limit_s': args.cpu_s,
              'swap_monitored': swap0 is not None, 'swap_mib': args.swap_mib,
              'parent_pid': os.getpid()}})
        print('ladder pid %d -> %s   abort: Ctrl-C or touch %s' % (os.getpid(), args.out, abort_file))
        prev = []
        for si, stage in enumerate(stages):
            masked = perf.STAGES[stage]
            frame = perf.auto_frame(masked) if args.frame == 'auto' else int(args.frame)
            if si:
                ok, why, projected = project_next(prev, masked, args.reps, limits, now() - start)
                emit({'type': 'gate', 'stage': stage, 'advance': ok, 'reason': why,
                      'projected_stage_s': round(projected, 1)})
                print('gate %s: %s (%s, projected %.0f s)' % (stage, 'advance' if ok else 'STOP', why, projected))
                if not ok:
                    stopped = 'gate before stage %s: %s' % (stage, why)
                    break
            prev = []
            for cls in classes:
                if os.path.exists(abort_file):
                    stopped, exit_code = 'abort file present', 3
                    break
                if limits['total_wall_s'] and now() - start > limits['total_wall_s']:
                    stopped, exit_code = 'ladder wall budget %d s exhausted' % limits['total_wall_s'], 1
                    break
                cell_args = ['--class', cls, '--masked', str(masked), '--frame', str(frame), '--seed',
                             str(args.seed), '--reps', str(args.reps), '--wall-s', str(args.cell_wall_s),
                             '--cpu-s', str(args.cpu_s), '--mem-mib', str(args.mem_mib),
                             '--expected', args.expected]
                try:
                    rec = runner(cell_args, args.cell_wall_s)
                except KeyboardInterrupt:
                    stopped, exit_code = 'interrupted', 130
                    break
                rec.update({'stage': stage, 'class': cls, 'masked': masked, 'frame': frame})
                emit(rec)
                prev.append(rec)
                best = [e['wall_s'] for e in rec.get('evals', [])]
                print('%s %-8s %-7s %s' % (stage, cls, rec['status'], ' '.join('%.2fs' % w for w in best)))
                if rec['status'] != 'ok':
                    stopped, exit_code = 'cell %s/%s: %s' % (stage, cls, rec['status']), 1
                    break
                why = swap_problem(swap0, args)
                if why:
                    stopped, exit_code = why, 1
                    emit({'type': 'gate', 'stage': stage, 'advance': False, 'reason': why})
                    break
            if stopped:
                break
            completed.append(stage)
        emit({'type': 'end', 'elapsed_s': now() - start, 'stopped': stopped, 'loadavg': perf.loadavg(),
              'swap': perf.swap_info(), 'completed_stages': completed})
    print('done: %s' % (stopped or 'ladder complete'))
    return exit_code


def cell_main(args):
    if perf.resource is not None and args.cpu_s:
        try:
            perf.resource.setrlimit(perf.resource.RLIMIT_CPU, (args.cpu_s, args.cpu_s + 30))
        except (ValueError, OSError):
            pass
    if perf.resource is not None and args.rlimit_data_mib:
        try:
            lim = args.rlimit_data_mib * 1024 * 1024
            perf.resource.setrlimit(perf.resource.RLIMIT_DATA, (lim, lim))
        except (ValueError, OSError, AttributeError):
            pass
    rec = perf.run_cell(args.cls, args.masked, args.frame, args.seed, args.reps,
                        wall_s=args.wall_s, mem_kib=args.mem_mib * 1024, expected=load_expected(args.expected))
    sys.stdout.write(json.dumps(rec, sort_keys=True) + '\n')
    return 0


def selftest(_args):
    failures = []
    rng = perf.SplitMix64(0)
    got = [rng.next() for _ in range(3)]
    if got != [0xE220A8397B1DCDAF, 0x6E789E6AA1B965F4, 0x06C45D188009454F]:
        failures.append('SplitMix64 vectors differ: %r' % [hex(g) for g in got])
    pin = load_expected(EXPECTED)
    if pin is None:
        failures.append('expected.json missing')
    else:
        env = perf.capture_env('selftest')
        if env['decision_sha256'] != pin['decision_sha256']:
            failures.append('decision.py is not the reviewed file')
        for cls in perf.CLASSES:
            frame = perf.auto_frame(perf.STAGES['A'])
            rec = perf.run_cell(cls, perf.STAGES['A'], frame, 1, 1, expected=pin)
            if rec['status'] != 'ok' or rec['digest_check'] != 'match':
                failures.append('stage A %s: status %s, digests %s' % (cls, rec['status'], rec['digest_check']))
    print('selftest: %s' % ('FAIL ' + '; '.join(failures) if failures else 'ok'))
    return 1 if failures else 0


def preflight(_args):
    env = perf.capture_env('preflight')
    print(json.dumps({k: env[k] for k in ('python', 'implementation', 'byteorder', 'maxsize', 'int_info',
                      'system', 'release', 'machine', 'cpu_model', 'cpu_count', 'physical_memory_kib',
                      'swap', 'loadavg', 'clocks')}, indent=1, sort_keys=True))
    print('peak_rss_kib now:', perf.peak_rss_kib(), '  resource module:', perf.resource is not None,
          '  setitimer guard:', perf.signal is not None and hasattr(perf.signal, 'setitimer'))
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest='cmd', required=True)
    sub.add_parser('selftest').set_defaults(fn=selftest)
    sub.add_parser('preflight').set_defaults(fn=preflight)
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument('--reps', type=int, default=3)
    common.add_argument('--seed', type=int, default=1)
    common.add_argument('--cell-wall-s', type=int, default=1800, help='wall limit per cell (build + reps)')
    common.add_argument('--cpu-s', type=int, default=2400, help='RLIMIT_CPU per cell')
    common.add_argument('--mem-mib', type=int, default=128, help='peak resident-set limit per cell')
    common.add_argument('--expected', default=EXPECTED)
    lad = sub.add_parser('ladder', parents=[common])
    lad.add_argument('--label', required=True, help='host label, e.g. linux-x86_64 or netbsd-macppc')
    lad.add_argument('--out', required=True, help='JSONL results file (appended)')
    lad.add_argument('--stages', default='A,B,C')
    lad.add_argument('--classes', default=','.join(perf.CLASSES))
    lad.add_argument('--frame', default='auto', help="'auto' keeps the certified mask density, or a pixel count")
    lad.add_argument('--total-wall-s', type=int, default=5400, help='ladder budget; no new stage is projected past it')
    lad.add_argument('--swap-mib', type=int, default=8, help='stop if swap in use grows by more than this')
    lad.add_argument('--max-load', type=float, default=1.0)
    lad.add_argument('--allow-busy', action='store_true')
    lad.add_argument('--allow-unknown-swap', action='store_true',
                     help='run although swap use cannot be read (check it by hand)')
    lad.set_defaults(fn=lambda a: ladder(a))
    cell = sub.add_parser('cell', parents=[common])
    cell.add_argument('--class', dest='cls', required=True)
    cell.add_argument('--masked', type=int, required=True)
    cell.add_argument('--frame', type=int, required=True)
    cell.add_argument('--wall-s', type=int, default=1800)
    cell.add_argument('--rlimit-data-mib', type=int, default=0, help='optional RLIMIT_DATA; off by default')
    cell.set_defaults(fn=cell_main)
    args = p.parse_args(argv)
    return args.fn(args)


if __name__ == '__main__':
    sys.exit(main())
