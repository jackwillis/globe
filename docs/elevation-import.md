# Local elevation packs

Both front ends accept `--elevation FILE`, repeated up to three times in addition to the embedded
one-degree pack. Files are read only at startup. The GELV decoder validates them; import currently
requires a global 2:1 grid with equal angular cell sizes. There is no network access.

The combined logical terrain cache payload is limited to 32 MiB. This excludes source bytes,
framebuffers, allocator overhead and other process memory. Input file size is checked before
allocation; invalid packs fail startup with an error. Imported bytes outlive their cached views.

`quality_for(Model)` chooses the coarsest registered grid whose equatorial cell spans at most four
screen pixels. If none meets that target it uses the finest available. Later equal-resolution
packs override earlier ones. Dimensions enter the pure model through `RegisterElevation` (trace
verb `elevation LEVEL WIDTH HEIGHT`); actual file paths and reads stay in the platform.

Build another resolution on a modern machine with the elevation builder's `--cell-degrees` option.
For the verified ETOPO source, a half-degree build produced 720 x 360 cells, 259,752 bytes, SHA-256
`bd10f2807416bed11e92d9fe2ef89481f1e207bb826b66d9fabb6005f6674ee8`.
The builder uses a bounded 64 MiB HDF5 chunk cache; this is builder memory, not target runtime memory.
A real half-degree import selected level 1 at high zoom on the host. Its data is optional and is
not embedded or committed.

Validation: GCC sanitizer suite 22/22 before the final additional malformed-trace/corruption cases;
Retro68 PowerPC and 68k compile probes pass. Selection-threshold, aspect-validation and aggregate
budget mutants are rejected. Source bytes, CRC failures, missing files and oversized files are
covered by the import test. Legacy moving/refined checksum pins are unchanged.
