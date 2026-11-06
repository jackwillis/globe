# Issue #7: CI verification

Verification started 2026-11-05 (America/Chicago).

## Failure and fix

The [latest reviewed Checks run](https://github.com/jackwillis/globe/actions/runs/42133611387)
passed the GCC sanitizer tests, Clang tests, PowerPC/68k compile probes and OS 9
floppy packaging. The host job failed clang-tidy 22.1.8 with
`performance-enum-size` on `Framebuffer::Coverage`; full-edition packaging and
artifact upload were consequently skipped.

Give `Coverage` the explicit underlying type `std::uint8_t`. Its three values
and rendering behavior are unchanged. This changes no geographic data, update
semantics, allocation strategy or offline behavior. No performance improvement
is claimed; existing rendering tests cover the affected code.

## Local verification

- Repository policy and clang-format 22.1.8 checks pass.
- clang-tidy 22.1.8 passes for all 39 non-OS-9 source files using the
  repository's compile database and warnings-as-errors configuration.
- Fresh GCC ASan/UBSan build passes all 47 tests outside the sandbox.
- Fresh Clang build passes all 38 non-GUI tests; all nine GUI tests pass
  when rerun outside the sandbox (47 tests verified in total).
- Full-edition Release build and size report pass: executable 663,528 bytes,
  documents 35,842 bytes, payload 699,370 bytes. System libraries and optional
  imported packs are excluded; this is not a floppy-fit claim.
- Both Retro68 compilers pass all 26 probe translation units (52 successful
  compilations, no `FAIL` entries).
- OS 9 floppy build and packaging pass: payload 570,767 bytes against a
  1,200,000-byte budget, leaving 629,233 bytes. HFS image is 1,474,560 bytes;
  extracted application forks match byte for byte. The application was not run
  on OS 9 hardware.

The sandbox prevents LeakSanitizer process inspection and access to the test
X servers. Sanitizer and GUI checks must run outside that sandbox; these
environmental failures are not accepted as passing tests.

## Actions acceptance still required

The reviewed run's [OS 9 artifact](https://github.com/jackwillis/globe/actions/runs/42133611387/artifacts/13464428045)
is prior evidence, not an artifact for this fix. After an authorized push, record
a complete successful Checks run with all three required jobs and links to both
`full-edition-size-report` and `os9-floppy-build-and-size-report`. Do not close
issue #7 based only on local checks or the prior run.
