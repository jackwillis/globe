#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Package existing headless benchmarks on the development host; run offline on NetBSD.

Python standard library only. No network, installation, cache flushing, or hardware control.
Run --help, prepare --help, or run --help for the two explicit opt-in operations.
"""
import argparse
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import re
import shutil
import statistics
import subprocess
import tempfile
import time


BINARIES = ('globe-bench', 'globe-line-layer-bench', 'globe-shade-bench')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture(args, cwd=None):
    return subprocess.check_output(args, cwd=cwd, text=True).strip()


def write_json(path, value):
    # Exclusive creation prevents accidentally overwriting another run or baseline.
    with path.open('x') as out:
        json.dump(value, out, indent=2, allow_nan=False)
        out.write('\n')


def prepare(args):
    root = Path(__file__).resolve().parents[1]
    build = args.build_dir.resolve()
    cache = {}
    for line in (build / 'CMakeCache.txt').read_text().splitlines():
        if line and not line.startswith(('#', '//')) and '=' in line:
            key, value = line.split('=', 1)
            cache[key.split(':', 1)[0]] = value
    if Path(cache['CMAKE_HOME_DIRECTORY']).resolve() != root:
        raise ValueError('build directory belongs to another source checkout')
    if cache.get('GLOBE_SANITIZE') != 'OFF':
        raise ValueError('use an uninstrumented build for timing')
    # Rebuild before recording provenance, including all generated embedded data.
    subprocess.run(['cmake', '--build', str(build), '--target', *BINARIES, '-j2'], check=True)
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    for name in BINARIES:
        shutil.copy2(build / name, output / name)
    shutil.copy2(root / 'tests/data/bench_drag.txt', output / 'bench_drag.txt')
    (output / 'settle.txt').write_text('expose\ntick 1000\n')
    shutil.copy2(Path(__file__), output / 'g4_bench.py')
    shutil.copy2(build / 'compile_commands.json', output / 'compile_commands.json')
    systems = list((build / 'CMakeFiles').glob('*/CMakeSystem.cmake'))
    compiler_files = list((build / 'CMakeFiles').glob('*/CMakeCXXCompiler.cmake'))
    if not compiler_files:
        raise ValueError('missing CMake compiler configuration; configure the build first')
    compiler_config = compiler_files[0].read_text()
    compiler_match = re.search(r'set\(CMAKE_CXX_COMPILER "([^"]+)"\)', compiler_config)
    compiler = cache.get('CMAKE_CXX_COMPILER') or (compiler_match.group(1) if compiler_match else None)
    if not compiler:
        raise ValueError('CMake compiler path is missing')
    source_status = capture(['git', 'status', '--porcelain', '--untracked-files=normal'], root)
    manifest = {
        'schema': 1,
        'git_sha': capture(['git', 'rev-parse', 'HEAD'], root),
        'source_status': source_status,
        'tracked_diff_sha256': hashlib.sha256(subprocess.check_output(
            ['git', 'diff', 'HEAD', '--binary'], cwd=root)).hexdigest(),
        'build_host': {'os': platform.system(), 'release': platform.release(),
                       'architecture': platform.machine()},
        'compiler_version': capture([compiler, '--version']),
        'cmake': {k: v for k, v in cache.items() if k.startswith((
            'CMAKE_CXX_', 'CMAKE_BUILD_TYPE', 'CMAKE_TOOLCHAIN_FILE',
            'CMAKE_EXE_LINKER_FLAGS', 'GLOBE_'))},
        'compiler_configuration': compiler_config,
        'cmake_system': systems[0].read_text() if systems else None,
        'data_edition': 'embedded atlas + 1-degree elevation + land classes; no external packs',
        'data_sha256': {p.name: digest(p) for p in (
            root / 'data/atlas.gatl', root / 'data/elevation-1deg.gelv',
            root / 'data/landclasses-1deg.ggrd')},
        'files_sha256': {p.name: digest(p) for p in sorted(output.iterdir()) if p.is_file()},
        'limits': 'Build flags are also in compile_commands.json. Dirty sources are marked; '
                  'use a clean committed checkout for published comparisons. No claim of '
                  'bit-reproducible compiler output or target execution.',
    }
    write_json(output / 'manifest.json', manifest)
    print(output)


def summary(values):
    values = sorted(values)
    return {'count': len(values), 'min': values[0], 'median': statistics.median(values),
            'p95': values[math.ceil(.95 * len(values)) - 1], 'max': values[-1]}


def child(command):
    # wait4 gives this child's high-water RSS, rather than the cumulative children maximum.
    # Temporary files prevent pipe backpressure, and all output stays outside timed renderer work.
    with tempfile.TemporaryFile() as out, tempfile.TemporaryFile() as err:
        start = time.perf_counter_ns()
        with subprocess.Popen(command, stdout=out, stderr=err) as proc:
            try:
                _, status, usage = os.wait4(proc.pid, 0)
            except KeyboardInterrupt:
                proc.terminate()
                proc.wait()
                raise
            proc.returncode = os.waitstatus_to_exitcode(status)
            elapsed = (time.perf_counter_ns() - start) / 1e6
            out.seek(0)
            err.seek(0)
            if proc.returncode:
                raise RuntimeError(f'{command[0]} exited {proc.returncode}: '
                                   + err.read().decode(errors='replace'))
            raw = json.load(out)
        return {'process_wall_ms': elapsed, 'peak_rss_kib': usage.ru_maxrss,
                'user_cpu_ms': usage.ru_utime * 1000, 'system_cpu_ms': usage.ru_stime * 1000,
                'benchmark': raw}


def metrics(sample):
    result = {k: sample[k] for k in ('process_wall_ms', 'peak_rss_kib',
                                    'user_cpu_ms', 'system_cpu_ms')}
    raw = sample['benchmark']
    for key, value in raw.items():
        if (key.endswith('_ms') or key == 'first_frame_ms_since_main') and isinstance(value, (int, float)):
            result[key] = value
        elif key in ('render_ms', 'convert_ms') and isinstance(value, dict):
            for stat in ('p50', 'p95', 'max'):
                result[f'{key}.{stat}'] = value[stat]
    for i, case in enumerate(raw.get('results', [])):
        prefix = f"case{i}.{case.get('mode', case.get('case', ''))}.r{case.get('radius', case.get('radius_px', ''))}"
        for key, value in case.items():
            if key.endswith('_ms'):
                result[f'{prefix}.{key}'] = value
    return result


def checksums(raw):
    if 'frame_checksum' in raw:
        return [raw['frame_checksum']]
    return [case['checksum'] for case in raw.get('results', []) if 'checksum' in case]


def run(args):
    if platform.system() not in ('Linux', 'NetBSD'):
        raise ValueError('RSS units are validated only for Linux and NetBSD')
    bundle = args.bundle.resolve()
    manifest = json.loads((bundle / 'manifest.json').read_text())
    for name, expected in manifest['files_sha256'].items():
        if digest(bundle / name) != expected:
            raise ValueError(f'bundle hash mismatch: {name}')
    if args.output.exists():
        raise ValueError('output already exists; choose a distinctive new run name')
    commands = {
        'line-layer': [str(bundle / 'globe-line-layer-bench'), '--repeat', '1'],
        'drag': [str(bundle / 'globe-bench'), '--repeat', '1', '--size', '640x480',
                 str(bundle / 'bench_drag.txt')],
        'settle': [str(bundle / 'globe-bench'), '--repeat', '1', '--size', '640x480',
                   '--refine', str(bundle / 'settle.txt')],
        'shade': [str(bundle / 'globe-shade-bench'), '3'],
    }
    workloads = list(dict.fromkeys(args.workload)) if args.workload else ['line-layer', 'drag', 'settle']
    samples = {name: [] for name in workloads}
    for _ in range(args.warmup):
        for name in workloads:
            child(commands[name])
    # Round robin, so a workload is not measured only at one end of a long run.
    for _ in range(args.repeat):
        for name in workloads:
            samples[name].append(child(commands[name]))
    results = {}
    for name, rows in samples.items():
        expected = checksums(rows[0]['benchmark'])
        if not expected or any(checksums(row['benchmark']) != expected for row in rows):
            raise ValueError(f'unstable or missing checksums: {name}')
        values = [metrics(row) for row in rows]
        results[name] = {
            'command': commands[name], 'viewport': [640, 480],
            'summary': {key: summary([row[key] for row in values]) for key in values[0]},
            'samples': rows,
        }
    write_json(args.output, {
        'schema': 1, 'utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'machine_label': args.label,
        'runtime': {'os': platform.system(), 'release': platform.release(),
                    'architecture': platform.machine(), 'python': platform.python_version()},
        'build': manifest, 'repeat': args.repeat, 'warmup_processes_per_workload': args.warmup,
        'cache_condition': 'uncontrolled OS cache; fresh process per sample; '
                           + ('explicit workload warmups' if args.warmup else 'no explicit warmup'),
        'clock': 'CLOCK_MONOTONIC in binaries; Python perf_counter_ns for process wall time',
        'statistics': 'median and nearest-rank p95 across processes; n=9 p95 is the maximum; '
                      'nested render p95 is per-frame, not a pooled percentile',
        'limits': 'Headless only: first band is CPU-ready, not visible response; no X11 or '
                  'QuickDraw presentation, input latency, real cold-cache claim, per-phase '
                  'current-job timings or Classic partition acceptance. RSS is wait4 ru_maxrss '
                  'in KiB; may include an inherited pre-exec wrapper RSS floor (observed on NetBSD), '
                  'so it cannot resolve small benchmark allocation differences. Includes '
                  'libraries/static data/allocator and OS accounting; not '
                  'heap payload. Python and compiler memory excluded. Shade has 3 internal '
                  'repetitions per process. No performance threshold or G4 gain is inferred.',
        'results': results,
    })
    print(args.output)


def positive(value):
    number = int(value)
    if not 1 <= number <= 100:
        raise argparse.ArgumentTypeError('expected 1..100')
    return number


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='operation', required=True)
    prep = sub.add_parser('prepare', help='development host only: rebuild and package')
    prep.add_argument('--build-dir', type=Path, required=True)
    prep.add_argument('--output', type=Path, required=True)
    prep.set_defaults(action=prepare)
    execute = sub.add_parser('run', help='offline execution; never builds or installs')
    execute.add_argument('--bundle', type=Path, required=True)
    execute.add_argument('--output', type=Path, required=True)
    execute.add_argument('--label', default='unspecified')
    execute.add_argument('--repeat', type=positive, default=9)
    execute.add_argument('--warmup', type=int, choices=range(0, 11), default=1)
    execute.add_argument('--workload', choices=('line-layer', 'drag', 'settle', 'shade'),
                         action='append')
    execute.set_defaults(action=run)
    args = parser.parse_args()
    try:
        args.action(args)
    except (OSError, ValueError, KeyError, RuntimeError, subprocess.CalledProcessError) as error:
        parser.exit(1, f'g4-bench: {error}\n')


if __name__ == '__main__':
    main()
