# Digital-first packaging: findings so far (#47)

Checkpoint 2026-11-08. Research only; nothing here is a release.

## Verified on the Linux host (Fedora, `unar` 1.10.7)

- The Retro68 build's `GlobeFloppy.bin` is a MacBinary file: type `APPL`, creator `Glb2`, data fork
  546,172 bytes (the PEF), resource fork 882 bytes (SIZE, menus) at commit `18b08b5`.
- `lsar -l` lists both forks; `unar -forks visible` extracts `GlobeFloppy` and `GlobeFloppy.rsrc`
  with the right sizes. This shows the MacBinary fallback keeps both forks on a Linux host.
- Two builds of the same tree in the same build directory gave the same SHA-256
  (`dad15e1c...9ff1`). Reproducibility across fresh build directories and containers is not checked.
- `tools/package-floppy.sh` already builds an HFS image with `hformat`/`hcopy -m` and checks the
  round trip, so the disk-image fallback exists (it is sized for the paused floppy edition).

## Built (2026-11-08, owner approved trying stuffit-rs in a container)

- `tools/sit-pack` (Rust, `stuffit` pinned `=0.4.0`, `Cargo.lock` committed; 46 crates, all MIT,
  Apache-2.0, Zlib, 0BSD or BSD-3-Clause licensed) builds a `.sit` from MacBinary files (both forks,
  type, creator) and text files (LF to CR, type `TEXT`). The stuffit CLI cannot read forks from
  files on Linux (macOS xattrs only), hence the helper.
- `tools/package-release.sh BUILD_DIR` builds it in the `rust:slim` container, writes
  `Globe-<rev>.sit` (method 13 by default, `SIT_METHOD=0` to store), copies the `.bin`, round-trips
  both through `unar` comparing data and resource forks, and writes `SHA256SUMS`. Two runs gave the
  same checksums. Output stays in the ignored build directory.
- Host round trip, build `18b08b5`: method 0 (584,516 bytes) and method 13 (383,371 bytes) both
  extract with `unar` to forks byte-identical to the MacBinary; type `APPL`, creator `Glb2`.

## Not done

- **Expander on a Mac:** nothing has been expanded on Tiger Classic or bare OS 9. StuffIt Expander
  6.0 is on the `MacOS9Lives` volume. A first attempt to launch it from ssh hung and the Classic
  session froze; the cause is not established (see `docs/ux-acceptance.md`). Do not claim
  compatibility from a Linux-made archive.
- Disk-image fallback is still `tools/package-floppy.sh` (floppy-sized); a larger layout is #51.
- Decision on compression method and shipped artifacts is open until Expander results exist.
