#!/usr/bin/env python3
"""Bounded X11 startup test using an isolated Xvfb we own."""
import os
import subprocess
import time

depth = os.environ.get('X11_DEPTH', '16')
assert depth in ('16', '24')
read_fd, write_fd = os.pipe()
server = subprocess.Popen(['Xvfb', '-displayfd', str(write_fd), '-screen', '0', '800x600x' + depth, '-nolisten', 'tcp'], pass_fds=(write_fd,), stderr=subprocess.DEVNULL)
os.close(write_fd)
app = None
try:
    import select
    if not select.select([read_fd], [], [], 5)[0]:
        raise RuntimeError('Xvfb startup timed out')
    display = os.read(read_fd, 64).decode().strip()
    if not display or server.poll() is not None:
        raise RuntimeError('Xvfb could not open a display')
    app = subprocess.Popen([os.environ.get('PLANET_X11', './planet-x11')], env={**os.environ, 'DISPLAY': ':' + display})
    time.sleep(2)
    assert app.poll() is None, 'X11 app exited during startup'
    if os.environ.get('X11_INPUT_TEST'):
        subprocess.run([os.environ['X11_INPUT_TEST']], env={**os.environ, 'DISPLAY': ':' + display}, check=True, timeout=15)
    if os.environ.get('X11_BENCH_RESULT'):
        with open(os.environ['X11_BENCH_RESULT'], 'w') as output:
            subprocess.run(['./planet-x11-bench', '320'], env={**os.environ, 'DISPLAY': ':' + display}, stdout=output, check=True, timeout=50)
    print('X11 ' + depth + '-bit startup smoke test passed')
finally:
    os.close(read_fd)
    for process in (app, server):
        if process is not None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
