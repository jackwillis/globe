# EXP-WI-DATA-01: Wisconsin data architecture experiment

Research only, build-host only, Python standard library only. No runtime, format, release-scope or
ADR change. Read in this order: [preregistration](preregistration.md) (committed before any
acquisition), [report](report.md), `results-synthetic.json` and `results-real.json`.

**Licensing:** none of the three real sources is on the allowed list in
`docs/licensing-policy.md`. They are research-only. Raw bytes stay in an ignored cache; nothing
derived from a specific real record is committed, only aggregates. Committed tests use the
synthetic fixtures in `synthetic.py`.

```sh
# synthetic (no network, no cache): 39 tests, then the synthetic experiment run
python3 -B -m unittest atlas/research/wi_data/test_wi_data.py   # run from the repository root
python3 atlas/research/wi_data/experiment.py /tmp/wi-synthetic.json

# real samples: explicit acquisition into an ignored cache, then verify and run
python3 atlas/research/wi_data/acquire.py build-wi-data/sources --download
python3 atlas/research/wi_data/acquire.py build-wi-data/sources
WI_DATA_CACHE=build-wi-data/sources python3 -B -m unittest atlas/research/wi_data/test_wi_data.py
python3 atlas/research/wi_data/experiment.py /tmp/wi-real.json --real build-wi-data/sources
```

`acquire.py` reads only four allowlisted hosts, caps every response, and refuses a checksum
mismatch. The Census ACS rows come from ranged reads of the published table-based summary files
(the Data API now requires a key). `sources.lock.json` records URLs, bytes, SHA-256 and the licence
status of each file.

| File | Role |
| --- | --- |
| `model.py` | Shared identity registry, claims, time, provenance (needed by every model) |
| `store_a.py` | Model A: generic statement store, wide and lean variants |
| `store_b.py` | Model B: independent layer formats and native accessors |
| `iface_c.py`, `iface_adapters.py` | Model C: shared bounded inspector, descriptors, thin layer glue |
| `catalog_fit.py` | Model D: attempts against the real `atlas/research/catalog.py` |
| `oracle.py` | Independent brute-force oracles on raw float source records |
| `readers.py` | Source adapters; reuses `atlas/research/sources.py` for SHP/DBF |
