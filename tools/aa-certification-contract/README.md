# PE-AA-01 metric-uncertainty/discrete-outcome contract v1

Proposal only; no scoring entry point. Baseline63016edb9b5c191c542b11423828cf94f06aa8e4,
original scored protocol5dd566d942b01b8b8e92e0cb1cc3507910681532.
Read PROPOSAL.md and INVENTORY.md together. Original code/protocol/evidence is unchanged.

## Validation

Python3.14.8/Linux x86-64; stdlib only,384MiB address-space cap. Actual commands:

```sh
ulimit -v 393216
python3 tools/aa-certification-contract/test_contract.py /tmp/aa-certification-contract-evidence
python3 tools/aa-certification-contract/existing_evidence.py /tmp/aa-certification-contract-evidence
```

Synthetic controls pass:
-8256 independent exact point-loss values validate shared-reference differences, including exact
 cancellation for identical candidates versus a loose independent-error subtraction.
-349 half-up tie thresholds across5bit/6bit/8bit quantizers, endpoints and one-sided cases.
-2878 independently evaluated shared reference/quantized-loss cases.
-9216 ordered integer blends checked by independent rational averaging and bit-string decoding.
- Ambiguous alpha127/128 yields two packed outputs, not eight independent-channel combinations.
- Four independently known subpixel atoms validate different continuous overlaps with identical
 feature coverages; feature ordering changes sequential output.
- Zero contrast,394/765 peak retention, near-zero floor and uncertain support/visibility boundaries.
- Shared-reference radius10^-6 gives a midpoint/zero-width-bootstrap false20% pass; interval rule
 returns inconclusive. A midpoint local-regression pass also hides a feasible violation.
- Exactly5% undefined replicates allowed; above5% inconclusive; uncertain definedness never guessed.
-16heterogeneous paired geometry IDs×2styles,64explicit toy schedules,512joint-reference-world
 checks and64exhaustive quantile-endpoint worlds. These are not the frozen100000PCG draws and do not
 certify population coverage.

Synthetic control CPU~0.7s, peak recorded RSS14800KiB; these are research-tool resource observations,
not rendering benchmarks. The existing-evidence checker rereads all4800 already computed reference
certificates, verifies their hashes and quantizer endpoints, and sums exact widths. CPU~9.7s.
It performs no integration and rejects candidate-image inputs after imports. The combined manifest
is read before that audit; its hash/provenance is recorded. No geographic metric is calculated.

## Limitations and stop

Mathematical interval soundness, independent regression controls, and scientific/statistical
acceptance are different claims. There is no formal verification, geographic candidate comparison,
full sequential scorer, new reference or production test here. Original source masks use floating
geometry and the original reference-support semantics require explicit reconciliation. Exhausted
state bounds and unresolved denominators/eligibility remain inconclusive. Peak/visibility diagnostics
are not new acceptance gates; motion scoring remains deferred and visual judgment remains human.

Exactly one next step is recommended in PROPOSAL.md: steering review of a narrow versioned
certification amendment. Fine-reference feasibility remains conditional on that decision. No next
run, threshold change, protocol adoption, production change, G4 access, merge or release is authorized.
