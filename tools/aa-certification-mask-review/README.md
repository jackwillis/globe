# PE-AA-01 certification review corrections / masked uncertainty audit

Versioned follow-up to4c8bfc74; completed coarse reference63016edb, original protocol5dd566d9 unchanged.
Read REPORT.md, AMENDMENT-DECISION.md and SUBSET-METHOD.md. Claude's complete review was read from
PE-AA-01-Claude-Certification-Review.md (Library libfile_40de1c90f54481919817755526e77ae6),100lines.
It is review evidence, not approved policy. No original proposal/source/evidence is overwritten.

Exact commands, Linuxx86-64/Python3.14.8:
```sh
ulimit -v 393216
python3 tools/aa-certification-mask-review/test_review.py /tmp/aa-certification-mask-review-evidence
python3 tools/aa-certification-contract/test_contract.py /tmp/aa-certification-mask-review-evidence
OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python3 tools/aa-certification-mask-review/masked_audit.py /tmp/aa-certification-mask-review-evidence
```
Synthetic tests need only stdlib. The literal mask requires pinned NumPy2.5.3, frozen geometry and
existing certificate paths recorded by earlier evidence. The audit invokes the original neighborhood
function directly, twice for byte identity, not a port. Strict audit hook permits only that geometry
and existing reference gzip files after imports; reference manifest/source metadata are read first.
A first audit failed after arithmetic when platform.platform lazily imported subprocess through the
read guard. Its source/error/initial-mask hash are retained. Platform metadata is now resolved before
the read guard; mask bytes are unchanged. No candidate image was opened in either attempt.

Evidence includes mask rawbytes (307200bytes, uint8, row-major,1included), exact aggregate JSON,
per-mask-pixel integer width CSV,4800tile uncertainty/cost records, distributions/concentration,
mutant sources/results and original suite output. Cost fields are retained reference-computation
observations from earlier certificates, not a new rendering benchmark. No geographic integration,
bootstrap, candidate score, G4, production, merge or release. Proposed amendment stays unadopted.
