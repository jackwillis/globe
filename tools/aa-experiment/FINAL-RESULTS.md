# Final readiness evidence — 2026-11-18

C1–C4 and the requested preparation are complete. Next step: owner authorization for
geographic **quality-only** scoring and its positive reference decision margin. No further
general design review is requested. No geographic candidate scoring or performance measurement
was executed; B and production remain unchanged.

| Verification | Result |
|---|---|
| Peak retention analytical test | 0x8410 / white = 394/765; distinct source/reference ratios and null zero-reference behavior pass |
| Floor-limited bootstrap | Deterministic per-style exclusion/counts; >5% undefined or undefined point -> inconclusive; mixed and rare tests pass |
| Reader parity | 32/32; full per-feature/chain metadata and segment/join counts agree with C++ extraction; frozen bytes unchanged |
| Oracle analytical controls | Horizontal/vertical/butt/clip/tangent/degenerate exact; disc area error 0.00005880793, exact symmetry |
| Complete slow/row frames | 128/128 pixels exact continuous and packed RGB565; stated tie tolerance unused in these frames |
| Isolated-feature sidecars | Synthetic A/B/reference, other-feature exclusion, probes and malformed input pass; Release and ASan/UBSan |
| Blind package | 64 neutral PNGs, no metadata/identity key, 32 blank rows x8 columns |
| Unchanged source hashes | aa.cpp, aa.hpp, fixtures.cpp and images.py verified against baseline freeze |

## Candidate-blind real oracle preflight

Fixed original anchor view0-r220-map; inputs only geometry and background, enforced by an
input audit. 1617 affected tiles, 26176 tile-index entries, 19064 primitives. Final convergence
max delta 0.000973450904 <=1/1024, maximum sample grid 512x512. Operational resource
usage: 123.071s elapsed, 54116 KiB peak RSS, with 512 MiB address-space,
295/300s CPU and 360s elapsed caps. This accounts oracle feasibility; it is not candidate timing.
No candidate outputs were opened. Margin was None for this authorized tolerance-only diagnostic;
references must be regenerated at the positive owner-approved margin before quality scoring.

## Provenance and hashes

Python 3.14.8, NumPy 2.5.3; Linux x86-64. GCC16.2.1, C++20,
exceptions/RTTI disabled. Imported core uninstrumented; LeakSanitizer disabled. Full exact commands
and restrictions: FINAL-PRE-SCORING.md. New source hashes and exact payload commit/tree are in the
following metadata commit's final-source-freeze.json. Old unrecovered instrument versions remain
an explicitly documented historical gap; no invented diff or unchanged-semantics assertion.

Geometry parity evidence SHA256 `9c3a30d5fe8b2baf14e33877d60ed5c0a5776bbc1c6c895f285f33ab3cba5d38`.
Preflight evidence SHA256 `384c379feb9c4d8ac745af00097a9583a03e82adcb99290f67d81d19b51f6d38`.
B source SHA256 `57bba35a92042e8a324f48ba65022ca4c1b81ec255261c417e77ca6c34db8f71`.
B header SHA256 `3370db5572dc9aa47990b20cc412b993b2df77c8c67303ab9d8e5f0d2830af9e`.

Remaining decisions: explicit quality-only authorization and finite positive normalized absolute
reference decision margin. Native blind judgments must be recorded before identity disclosure.
Performance runner, adaptive millisecond budgets, Retro68/G4, production adoption, merge and
release remain separate; none blocks this quality-only authorization. No performance runner was
expanded. Full production tests/CI and Retro68 were not run; no hardware acceptance is claimed.
