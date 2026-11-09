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

## Expander 6.0 compatibility and the stuffit-rs workaround (2026-11-08)

stuffit-rs 0.4.0 output is read correctly by `unar` but **StuffIt Expander 6.0 rejected it as a
broken archive**. Comparing with a real StuffIt 5 archive (a Fetch/Oregon Trail `.sit` from the OS 9
volume, read only, not committed) showed three CRC-16s that stuffit-rs writes wrongly or leaves
zero; all are the ARC CRC (polynomial 0xA001, no pre/post conditioning):

1. Archive header: CRC at offset 98 over bytes 0..114 with the field zeroed (stuffit-rs writes the
   constant `0x009b`).
2. Each entry header: CRC at +32 over the whole `header_size` bytes with the field zeroed
   (stuffit-rs checksums only the first 32 bytes).
3. Each file's block after the name (flags2, **CRC**, type, creator, Finder flags, padding, and the
   14-byte resource-fork info when present): the 2 bytes after flags2 are the CRC over that whole
   block, 36 bytes (50 with a resource fork), field zeroed (stuffit-rs writes zero).

`tools/sit-pack` recomputes all three after serializing (`fix_header_crcs`). Each CRC was verified
against the real archive before it was applied; fixing 1 and 2 alone was not enough (Expander then
reported "couldn't open header" for every entry), 3 made it work.

Result, build `18b08b5`, StuffIt Expander 6.0 in Classic on Tiger 10.4.11 (shared G4, launched over
ssh `open -a`, owner at the console): both **method 0 and method 13** archives expand; GlobeFloppy's
data fork (546,172 bytes, cksum 2756265576) and resource fork (882 bytes, cksum 1514633535) are
identical to the MacBinary's, ReadMe/License/DataCredit match. Not yet checked: the expanded file's
type/creator (Tiger has no GetFileInfo; launching the expanded copy would test it), Expander on bare
OS 9, and older Expanders (5.5). A good upstream contribution: stuffit-rs should write these three
CRCs itself.

## Not done

- Bare OS 9 expansion and a launch of the expanded copy; decision on method (13 saves 35%; 0 is
  the safer fallback) and on which artifacts ship.
- Disk-image fallback is still `tools/package-floppy.sh` (floppy-sized); a larger layout is #51.
- Upstream issue/PR to stuffit-rs not filed (outward-facing; needs the owner's go-ahead).

## Owner check after expansion (2026-11-08)

The owner launched the Expander-expanded Globe in Classic on Tiger and reported "it works great".
That is a one-line report: it shows the expanded copy launches (so type and creator survived), but
it does not say which menu items, Cmd keys or About were tried, so the #46 checklist in
`docs/ux-acceptance.md` is still not recorded as passed. Bare OS 9 is untested.
