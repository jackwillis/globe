#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Candidate-blind existing-certificate admission/mask reconciliation ONLY."""
import argparse
import json
import os
import platform
import resource
import sys
import time
from pathlib import Path
from adapter import CoarseAdapter, Paths, digest, bounded_read


def install_read_guard(allowed, reads):
    """Restrict audited open calls only; not a filesystem/process sandbox."""
    def guard(event, arguments):
        if event == 'open':
            name, mode, flags = arguments
            if not isinstance(name, (str, bytes, os.PathLike)):
                raise PermissionError('non-path open prohibited')
            path = str(Path(os.fsdecode(name)).absolute())
            write = (isinstance(mode, str) and any(c in mode for c in 'wax+')) or bool(flags & (os.O_WRONLY | os.O_RDWR | os.O_CREAT | os.O_TRUNC | os.O_APPEND))
            if write or path not in allowed:
                raise PermissionError('evidence-only read guard: '+path)
            reads.add(path)
    sys.addaudithook(guard)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('fixtures', 'prefix', 'tail', 'audit'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    # Preload platform metadata before the strict evidence-only read guard.
    runtime = {'python': sys.version, 'platform': platform.platform()}
    resource.setrlimit(resource.RLIMIT_AS, (384*1024*1024, 384*1024*1024))
    resource.setrlimit(resource.RLIMIT_CPU, (60, 65))
    start, cpu = time.monotonic(), time.process_time()
    adapter = CoarseAdapter(Paths(**vars(args)))
    allowed = adapter.allowed_reads()
    reads = set()
    install_read_guard(allowed, reads)
    result = adapter.admit()
    result['provenance_sha256'] = digest(bounded_read(Path(__file__).with_name('provenance.json'), 128*1024))
    result['runtime'] = runtime
    result['resources'] = {'cpu_seconds': time.process_time()-cpu,
                           'elapsed_seconds': time.monotonic()-start,
                           'peak_rss_kib': resource.getrusage(resource.RUSAGE_SELF).ru_maxrss,
                           'address_space_cap_bytes': 384*1024*1024, 'cpu_soft_cap_seconds': 60}
    result['read_guard'] = {'unique_open_paths_allowed': len(reads), 'open_write_denied': True,
                            'scope': 'Python audited open operations only',
                            'does_not_block': ['unlink', 'rename', 'mkdir', 'truncate', 'chmod', 'symlink',
                                               'subprocess', 'ctypes'],
                            'installed_in': 'admit.py only; bounds() API is unguarded',
                            'allowlist': 'pinned sources, geometry/background, evidence records, mask, 4800 tiles'}
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
