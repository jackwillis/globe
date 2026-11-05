# Local elevation packs

The X11 full edition normally embeds one-degree and half-degree grids. It accepts
`--elevation FILE`, repeated up to three times; explicit imports use the one-degree base plus
those user packs instead of the optional built-in half-degree pack. Files are read only at
startup, validated as global 2:1 equal-angle GELV grids. There is no network access. The current
OS 9 floppy edition uses its embedded one-degree grid and has no command-line import path.

The combined logical terrain cache payload is limited to 32 MiB. This excludes source bytes,
framebuffers, allocator overhead and other process memory. Input file size is checked before
allocation; invalid packs fail startup with an error. Imported bytes outlive their cached views.

`quality_for(Model)` chooses the coarsest registered grid whose equatorial cell spans at most four
screen pixels. If none meets that target it uses the finest available. Later equal-resolution
packs override earlier ones. Dimensions enter the pure model through `RegisterElevation` (trace
verb `elevation LEVEL WIDTH HEIGHT`); actual file paths and reads stay in the platform.

Build another resolution on a modern machine with the elevation builder's `--cell-degrees` option.
The committed half-degree pack was rebuilt twice from the verified ETOPO source with the pinned
50m lakes input: 720 x 360, 259,752 bytes, SHA-256
`b51499b5c0a4dc09dd5ac62d27b6a520033b2411c858b71728e5e5df0f5e7906`.
The builder uses a bounded 64 MiB HDF5 chunk cache on the modern build host.
The earlier `bd10f280...` half-degree pack described a historical conversion, not this current
lakes-aware build. [Progressive scheduling and final-pixel guarantees](progressive-resolution-plan.md).

Historical validation: GCC sanitizer suite 22/22 before the final additional malformed-trace/corruption cases;
Retro68 PowerPC and 68k compile probes pass. Selection-threshold, aspect-validation and aggregate
budget mutants are rejected. Source bytes, CRC failures, missing files and oversized files are
covered by the import test. Legacy moving/refined checksum pins are unchanged.
