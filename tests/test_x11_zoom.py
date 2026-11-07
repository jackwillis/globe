# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""The zoom preview stands in for a render once a frame has been shown, also mid-refinement."""
import json
import subprocess
import sys

binary, data = sys.argv[1], sys.argv[2]


def run(trace, *extra):
    out = subprocess.run([binary, '--size', '320x240', '--script', f'{data}/{trace}', '--exit-when-done',
                          '--verify-pixels', '--stats', *extra], capture_output=True, text=True, check=True)
    return json.loads(out.stdout)


# Each refinement runs to its end before the wheel: all three zoom steps are previews.
settled = run('zoom_settled.txt', '--finish-refinement')
assert settled['zoom_previews'] == 3, settled
# The wheel arrives with the first refinement only just started (no finished frame to cache): the
# half-refined frame on screen is kept as the source, so both steps are still previews.
midjob = run('zoom_midjob.txt')
assert midjob['zoom_previews'] == 2, midjob
# And the switch turns it off.
off = run('zoom_settled.txt', '--finish-refinement', '--no-zoom-preview')
assert off['zoom_previews'] == 0, off
print(json.dumps(midjob))
