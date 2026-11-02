# Remaining work execution log

Started 2026-11-01, authorized by Jack: complete all open handoff items. Work on v2; keep
HANDOFF-codex.md uncommitted. Follow ADR 0001 including the G4 terrain performance stop condition.

- [x] Ordered 2: derived global quality, bounded refinement bands, cancellation; tests/mutants,
      host suites, Retro68 compile and G4 quality/band measurements.
- [x] Ordered 3: rehash persisted ETOPO; GELV spec, builder, zero-copy decoder, fixtures/mutations,
      reproducible 1-degree pack and cross-endian tests.
- [ ] Ordered 4: cached forward cell terrain, hypsometric colors, black borders; bounded refinement,
      G4 before/after metric and deliberate pixel pins. Stop if measured settled rendering is too slow.
- [ ] Ordered 5: local elevation import and global resolution selection; multi-resolution builder.
- [ ] Ordered 6: pure cursor intent and toolbar hit testing; X11 utility toolbar and cursor mapping.
- [ ] Ordered 7: GPOL country data, spherical picking with holes/poles/antimeridian, core InfoCard,
      scrolling X11 popup; GPLC cities afterwards.
- [ ] Ordered 8: Retro68 OS 9 application proof of life, wake-driven sleep, GWorld conversion;
      built, not run (owner hardware acceptance).
- [ ] Ordered 9: designated floppy build/packaging size report and CI report/budget.
- [ ] Independent 5: four-size host/G4 scaling report.

No CI execution or real G4 display access can be claimed. Keep commits narrow, write meaningful
checks before implementation and mutation-check new behavior. Append dated handoff notes at stop.

Completed so far: b7d5160 adds bounded refinement (G4 157.4420 ms total, 0.5975 ms maximum
continuation), with host/G4 optional-refine checksum equality. GELV v1 parser/encoder, all-byte
fixture corruption tests, three decoder mutants and quantizer tie mutant verified. The first
exact-size mutant survived because CRC rejected the extra byte; a repaired-CRC trailing-byte
case now rejects it. Persisted source hash rechecked; 1-degree pack is reproducible. G4 GELV tests
print ok; PowerPC/68k compile probe includes the decoder. Terrain is next and not yet implemented.
