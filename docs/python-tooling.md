# Python tooling and dependencies

Python is used **only on development/build hosts**, not on NetBSD/macppc or Classic Mac OS 9 at runtime.

Most builders and atlas audits use Python's standard library; do not add unnecessary third-party installations to those tasks. The normal CMake build uses the discovered Python 3 interpreter to embed data.

## Pinned optional scientific environment

The existing [hash-pinned research lock](../atlas/research/requirements-research.lock.txt) contains NumPy 2.5.4, h5py 3.16.0 and Shapely 2.1.2. It was verified for **CPython 3.14 on Linux x86-64**. It is not a general cross-platform lock.

```sh
python3.14 -m venv .venv-atlas
.venv-atlas/bin/python -m pip install --require-hashes \
  -r atlas/research/requirements-research.lock.txt
.venv-atlas/bin/python atlas/research/test_geometry_qa.py -v
```

Optional source-data conversion requires the scientific environment; standalone atlas/catalog validation and many builders do not. See [atlas/README.md](../atlas/README.md) for exact reproduction commands and source archives.

The minimum supported minor version for standard-library-only scripts has **not been formally tested**. To support a new Python version or OS, validate its wheel hashes, run byte-comparison data fixtures and record Python/zlib/GEOS versions. Do not replace the hash lock with loose version constraints.

A declarative `pyproject.toml` and portable lock workflow can follow once a tested Python/platform matrix exists; don't introduce a second contradictory lock prematurely.
