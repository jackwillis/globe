# PE-AA-01 one coarse full-fixture feasibility research

Separately versioned host-only adapter around immutable scale-v1 numerical code.
See EXECUTION.md for pre-execution limits and REPORT.md for measured results.
No candidates, renderer changes, quality scores or protocol amendment.

Commands (fresh output directory only; resource envelope covers preparation too):

```sh
python3 tools/aa-enclosure-scale-v1/controls_original.py
python3 tools/aa-enclosure-scale-v1/reliability.py /tmp/aa-full-coarse-evidence
python3 tools/aa-enclosure-full-coarse/verify.py /tmp/aa-full-coarse-evidence
ulimit -v 393216
python3 tools/aa-enclosure-full-coarse/supervise.py /tmp/aa-full-coarse-evidence
python3 tools/aa-enclosure-full-coarse/analyze.py /tmp/aa-full-coarse-evidence /tmp/aa-enclosure-scale-v1-evidence
```

The evidence includes budget-start.json, source-freeze.json and prior model files.
verify.py uses preserved original-anchor outputs, not candidate images. Full run reads
only view5-r220-relief.geometry and its background.rgb565. Reproduction is a new run
requiring authorization; commands do not authorize resume/retry.
