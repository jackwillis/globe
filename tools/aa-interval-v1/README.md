# PE-AA-01 first interval vertical slice v1

2026-11-18. Host-only instrumentation, Refs #25. Parent certification amendment:
`9a2214c6286c54f3d451956abe105b9d33877845`. **No geographic scoring authorization.**

This directory is separate from historical `quality_only.py`. It neither changes
historical evidence nor emits `continuous.npy`, `convergence.json` or point controls.

## Versioned read-only adapter contract

`adapter.py` identifies `PE-AA-01-coarse-certificate-adapter-v1`; `provenance.json`
is the reviewed-input allowlist, not a manifest to regenerate for arbitrary data.
It pins the amendment, documentary dependencies, engine/retained adapter/verifier,
independent historical control evidence, source freezes, manifest and input hashes.
The only admitted geographic identity is `view5-r220-relief`, geometry ID 10
(`2*view + (radius==440)`), Relief, 640×480. Pixel areas are [x,x+1]×[y,y+1];
original certificates are row-major 8×8 tiles. Geometry bytes bind endpoints,
colors, feature/chain ordering and closure. A separate digest of ordered raw
feature records explicitly binds source order. Background is little-endian RGB565.

External evidence roles are `fixtures`, `prefix`, `tail`, `audit`. The prefix holds
indices 0..4137, tail 4138..4799. No directory discovery or candidate-file argument
exists. Every payload is checked against the pinned combined-manifest digest;
there is no trusting a caller-replaced manifest. Original geometry is parsed
independently for finite/domain/count/dyadic constraints. No integrator is imported.

Each tile requires exact identity, complete 64-pixel coverage, unique required
pixel positions and integer bounds 0≤L≤U≤denominator, with shared canonical
positive denominator 255×4^16. JSON integer tokens are bounded decimal integers:
no -0, booleans, floats, overlong encodings or duplicate keys in integer fields.
Files and gzip expansion are size-bounded; nonfinite floats/constants are rejected.
Missing/partial/unknown/failed records are rejected as unavailable, never filled
with background. Pixel precision is checked by exact integer width inequalities;
wide valid intervals remain valid with their budget stop/precision status.
Quota/depth/unknown-area fields and exact half-up RGB565 code ranges are checked.
Floating resource/radius fields are telemetry and never arithmetic authority.
The output reports structural_admission="passed", geometric_enclosure_reproved=false,
PRECISE at the original operational 1/2560, and unevaluated DECISION-RESOLVED
separately. Structural admission under pinned provenance is not an independently
proven L≤R≤U enclosure. Engine/verifier version labels in provenance.json are
descriptive metadata; pinned source/evidence hashes establish identity, not
independent verification of those labels or of the producer. The fine target 1/10240 is
neither generated nor imposed as a validity gate.

Enclosure soundness is conditional on the pinned reviewed exact-dyadic producer,
its construction argument and existing independent controls, CPython integer
semantics and IEEE binary64 decoding. This adapter independently validates
records/provenance, not every geometric integration operation. Hashes establish
identity, not a new proof of enclosure. No upstream geographic uncertainty or
formal software verification is claimed. Existing review evidence is pinned and
cited, not represented as rerun in this slice.

The mask is the existing authoritative 307200-byte uint8 0/1 array, count 74796,
SHA256 `68c4c2ff39983467819eb8f4d06a6e04713c946431b1f7e08b5bc6f0bb23fcd7`.
Its full reviewed audit hash, source/dependency hashes, geometry hash and original
Python/NumPy/platform/floating provenance are pinned. This slice reconciles bytes
and uncertainty with that freeze; it does not regenerate or idealize the mask.
Current admission runtime may differ because mask construction is not rerun.

## Exact in-memory metric contract

`metrics.py` identifies `PE-AA-01-fixed-mask-interval-v1`. `fixed_mask` consumes
(index, RGB lower integers, RGB upper integers, denominator) rows, literal mask
bytes and expanded candidate RGB uint8 bytes. It normalizes exactly by 255 and
sums/divides using `fractions.Fraction`. Complete unique frame coverage is checked
even outside the mask. An empty mask is undefined/rejected. It returns rational
lower/upper endpoints for E_A, E_B and E_A−E_B over 3×included-pixel-count.

Each scalar absolute loss is piecewise linear. Endpoints and candidate-value
kinks contain all extrema. For E_A−E_B, evaluate BOTH losses at the SAME reference
value before taking extrema. Thus identical candidates cancel to [0,0] even with
wide reference intervals. Sum extrema over the fixed included pixels/channels.
These bounds are exact for the Cartesian product of scalar reference intervals;
geometry-induced inter-pixel/channel correlations are conservatively relaxed.
They are never claimed to be jointly achievable geographic extrema.

The adapter's `bounds()` returns a single-pass `BoundsStream`. Admission runs on
the first `next()`, before yielding rows; payload hashes are rechecked on the
second streaming pass. `mask()` returns pinned
literal bytes. Those methods are the future in-memory bridge to `fixed_mask`;
**the geographic bridge is not scored or invoked by the admission CLI**. A future authorized caller must exhaust the stream and obtain
`stream.require_complete()` before using a result. It returns an immutable receipt
with adapter version, pinned manifest SHA256 and 307200-pixel count only after
successful StopIteration. Partial iteration (even stopping at the last yielded
row), close, short completion or an exception cannot establish completion.
A normal for-loop or fixed_mask drains through StopIteration; fixed_mask still
independently checks unique complete coverage. The receipt adds constant-size
state, not a frame buffer. It is an API contract, not an unforgeable admission
token; fixed_mask remains a general synthetic/in-memory utility and callers must
use the matching pinned mask and stream. No geographic consumer is added. This session tests
the certificate-to-metric connection only with synthetic certificates. There is
no geographic candidate reader or scorer CLI.

`round_codes` independently exposes exact half-up boundary ranges for controls;
it does not compute coupled Q(R) losses. `at_least` is a generic synthetic
threshold classifier (equality passes; crossing is inconclusive), not an H/J or
geographic adoption criterion. All displayed metric endpoints are exact rational
strings; no float conversion/epsilon can turn a crossing into a pass.

## Reproduction

From this worktree, Python standard library only; no NumPy import, build or engine
execution. All evidence inputs are read-only and must already exist with their
pinned hashes. Missing evidence is an error, not permission to regenerate it.

```sh
sha256sum -c tools/aa-interval-v1/source-freeze.sha256
/usr/bin/time -v timeout 120s python3 -B -m unittest discover \
  -s tools/aa-interval-v1 -p 'test_*.py' -v
/usr/bin/time -v timeout 120s python3 -B tools/aa-interval-v1/check_mutations.py
/usr/bin/time -v timeout 90s python3 -B tools/aa-interval-v1/admit.py \
  --fixtures /tmp/pe-aa-phase3b-fixtures \
  --prefix /tmp/aa-full-coarse-evidence \
  --tail /tmp/aa-coarse-tail-evidence \
  --audit /tmp/aa-certification-mask-review-evidence
```

The CLI enforces 384 MiB address space, 60/65 CPU seconds and a strict evidence-only
open audit hook. The hook blocks Python-audited open calls for non-allowlisted
paths and write modes/flags; it does not block unlink, rename, mkdir, truncate,
chmod, symlink, subprocess, os.system or ctypes. It is installed only by admit.py,
not by the bounds() library path. It is not a filesystem/process sandbox or a
general write-prevention mechanism. The actual program uses read-only evidence
opens and has no evidence-mutation code. Pinned hashes and the reviewed code are
the trust boundary. Output open_write_denied describes this limited hook;
unique_open_paths_allowed counts distinct allowed open attempts, not successful
file reads. `timeout` supplies the outer wall guard. Shell
redirection can save stdout/stderr outside the evidence inputs. These are verifier
resource observations, not rendering benchmarks. Tests use synthetic in-memory evidence (one manifest, two cached tile payloads)
and temporary synthetic files only. No geographic evidence is needed for the
integration or mutation suite; no admission/hash/parser/validation loop is mocked.
Only the bounded file-read boundary is replaced, with test-only synthetic pins. The historical evidence packages supply the four input roles; exact
role file names and hashes are in `provenance.json`. Input source packages:
coarse original ZIP `d198144721dc036020ac0332c25499eececb312e33fce76fd88d7eb58555649b`,
completion ZIP `868438999b7dd9971a36cb8494584f3ecbd4bf4656a8122dcc556777156b6bdf`,
masked review ZIP `29b45847087df865f45db459a865ba969a347daa3322d14d630a8293ab98ed51`.
Archives need not be opened by this verifier: extracted records are individually
pinned, as is the completed manifest. The historical accounting gaps stay intact.

See [REPORT.md](REPORT.md) for results, omissions and the steering stop.


CI limitation: the inherited Checks job is failing at clang-format 22.1.8 in
`tools/aa-experiment/synthetic_driver.cpp`; see REPORT.md. Local Python controls
passing does not make the PR CI-green. This follow-up changes no ancestor C++ or
workflow file and does not add these research tests to CI.
