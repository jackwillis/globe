# Two edition size report, 2026-11-01

These are local builds and measurements. CI steps now reproduce reports and upload build artifacts;
CI has not run (no remote). Nothing was deployed or published. OS 9 was built and packaged, not run.

Exactly two release entry points:

- `tools/build-edition.sh floppy`: classic PowerPC OS 9 app, baked one-degree GELV, 110m atlas.
- `tools/build-edition.sh full`: native X11 app, embedded atlas/elevation/countries/places, optional
  imported elevation levels. A native size report has no floppy budget. System libraries and
  optional external packs are excluded.

Both share the portable core and raw v1 pack formats. The current floppy is the D8 measurement
baseline, with raw embedded terrain; the newly decided compressed terrain is follow-up work,
before final release. No runtime codec or v1 layout was silently changed to improve these numbers.

| Floppy component | Bytes |
| --- | ---: |
| Application data fork | 233,020 |
| Application resource fork | 432 |
| MacBinary transfer file (including fork padding/header) | 233,728 |
| ReadMe and GPL license | 36,112 |
| Installed file payload, excluding HFS allocation overhead | 269,564 |
| Conservative payload budget | 1,200,000 |
| Payload budget headroom | 930,436 |
| Raw HFS image (80 x 2 x 18 x 512) | 1,474,560 |
| HFS free space after copies | 1,178,624 |

Retro68 GNU 16.1.0, `-Os`, no exceptions/RTTI, classic non-Carbon PowerPC. Resources request
24 MiB preferred and 12 MiB minimum application memory; disk fit is not a runtime memory claim.
The formatter created an actual 1.44 MB HFS image; hcopy installed the application and documents.
The application was extracted as MacBinary again; both forks match byte for byte. Header/fork
bounds, image geometry/signature, roundtrip and payload budget are checked by `floppy_report.py`.
HFS tools enforce actual disk fit; the Python signature check is not a general filesystem validator.
Budget, fork-roundtrip and image-geometry mutants all fail the reporting test.

The full edition's native, dynamically linked release executable is 330,360 bytes, plus the
35,149-byte GPL license (365,509 total). This is a different executable format/platform from the
OS 9 artifact, so these sizes do not compare runtime footprint. Both raw reports include hashes.
The build products live in ignored build directories; binaries and disk images are not committed.

The quarter-degree source pack was actually built: 1,037,352 bytes, SHA-256
`b6d9c8a4ae4cf66c67d6c247057c15e23416f6341a734965b33e07cf859b2241`.
Its raw size and the half-degree 259,752-byte pack are measured, not extrapolated. The quarter-degree
pack exceeds the current whole-grid cache budget; no quarter-degree application variant was built.
[Compression measurements](perf/compression.md) compare plain DEFLATE (500,301 cell bytes), gradient
predictor plus DEFLATE (428,179), adaptive range coding (804,716) and predictor plus range coding
(392,593), excluding the 552-byte metadata and future container. Every candidate roundtrips.
Atlas delta-varint points measure 26,706 bytes versus 32,480 raw. These modern Python sizes do not
choose the eventual target decoder; target speed and decoder memory remain unmeasured.

See [floppy JSON](floppy-size-20261101.json), [full JSON](full-size-20261101.json),
[OS 9 scope and acceptance](os9-proof.md) and [remaining execution log](open-work.md).
