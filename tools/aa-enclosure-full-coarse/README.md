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

Actual execution history: initial supervisor/run source is preserved as initial-source in
archive and local commit6cc43a9. After its recorded atomic-temp stat failure, the revised
supervisor/run source continued the SAME output directory, with a reduced1000s remaining
worker cap. Terminal clean stop at4138completed/662unstarted is not a successful traversal.
Use each archived freeze/source version for its respective historical command. The race's
exact timing is nondeterministic; reproduction cannot promise the same interruption index.

Postprocessing (no reference execution):
```sh
python3 tools/aa-enclosure-full-coarse/geometry_diagnostics.py /tmp/aa-full-coarse-evidence
python3 tools/aa-enclosure-full-coarse/check_recovery.py /tmp/aa-full-coarse-evidence
python3 tools/aa-enclosure-full-coarse/cost_diagnostics.py /tmp/aa-full-coarse-evidence
python3 tools/aa-enclosure-full-coarse/revise_projection.py /tmp/aa-full-coarse-evidence /tmp/aa-enclosure-scale-v1-evidence
python3 tools/aa-enclosure-full-coarse/account_resources.py /tmp/aa-full-coarse-evidence
python3 tools/aa-enclosure-full-coarse/write_report.py /tmp/aa-full-coarse-evidence
```
