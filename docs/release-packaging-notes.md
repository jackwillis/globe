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

## Not done

- **StuffIt (.sit):** no Rust toolchain is installed on the host. Building `stuffit-rs` means
  installing rustup/cargo and fetching an unreviewed third-party crate tree, so I did not. The crate
  describes StuffIt 5.0-compatible output with method 13 and both forks; that is its documentation,
  not a verified result. Needed: owner approval to install a toolchain (or a container image to use),
  a review of the crate's licenses and dependencies, then a host round trip with `unar`.
- **Expander on a Mac:** nothing has been expanded on Tiger Classic or bare OS 9; Tiger has no
  StuffIt Expander version recorded here. Do not claim compatibility from a Linux-made archive.
- No script was added; the packaging script waits on the `.sit` decision so the three artifacts
  come from one script.
