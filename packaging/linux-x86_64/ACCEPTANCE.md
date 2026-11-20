# Linux x86-64 archive: build and acceptance

Draft for v0.1.3 (release plan #212, Refs #122). Builds the X11 Reader with embedded data; no data
files, no packs. Needs the install rules from #209 and the `poodle-earth` name from #211.

## Build (maintainer)

```sh
tools/package-unix-archive.sh build-unix-archive     # Linux x86-64; CMake, Ninja, Python 3, libx11-dev, Xvfb optional
```

Builds from a `git archive` of committed HEAD in a scratch directory, then writes to the output
directory: `poodle-earth-VERSION-REV-linux-x86_64.tar.gz`, the matching `-source.tar.gz` and
`SHA256SUMS`. The script unpacks the archive, installs to a scratch prefix, compares the binary with
the build, launches it under Xvfb, uninstalls and checks nothing is left. Same revision and
`SOURCE_DATE_EPOCH` gives the same archive (checked twice on one host).

**Baseline:** the archive's glibc requirement is the newest `GLIBC_x.y` symbol in the binary, written
to `BUILD-INFO.txt`. Build in the oldest distribution you intend to support; a Fedora 44 build needs
glibc 2.38, a Debian 12 build needs 2.34. The binary needs only libX11, libstdc++, libgcc_s, libm and
libc, and has no RPATH.

## Evidence so far (revision 2dfa108; scripts from this branch)

| Build host | Result |
|---|---|
| Fedora Linux 44 x86_64, GCC 16.2.1, CMake 4.3.0 | archive built twice, identical checksums; round trip and Xvfb smoke ok; glibc >= 2.38 |
| Debian 12 x86_64 (podman container on the Fedora host), GCC 12.2.0, CMake 3.25.1 | built; round trip and Xvfb smoke ok; glibc >= 2.34 (revision differs: scratch git repo of the same tree) |
| Debian-12-built archive unpacked and run on the Fedora 44 host | `sha256sum -c` ok; install, Xvfb `--script tests/data/trace1.txt` gives 113 frames, exit 0; uninstall leaves no files |

## Acceptance checklist (owner; not yet done)

- [ ] Choose the glibc baseline and build the release archive on that distribution.
- [ ] On at least one real (non-container) desktop with a real X or XWayland display: unpack, run
      `./install.sh --prefix "$HOME/.local"`, launch, drag the globe, uninstall.
- [ ] Repeat on a second distribution family (for example Ubuntu LTS or Arch) and record the OS.
- [ ] Confirm the install notes, `SOURCE.txt` and notices read correctly to a new user.
- [ ] Decide archive file names, publish the archive, source archive and `SHA256SUMS` together.
- [ ] AppImage: optional, not started; needs an old-enough build baseline and a real multi-distribution test.
