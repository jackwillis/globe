# Globe

A tiny, offline-first, software-rendered interactive 3D Earth for PowerPC G4
(NetBSD/X11 and classic Mac OS 9). Status: **M0** (portable projection core);
there is no window or atlas yet.

- Design: [docs/v2-design.md](docs/v2-design.md)
- Toolchains and measured limits: [docs/toolchain.md](docs/toolchain.md)
- Relationship to the v1 prototype: [docs/prototype-notes.md](docs/prototype-notes.md)

```
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

`-DGLOBE_SANITIZE=ON` adds ASan and UBSan. `tools/probe-retro68.sh` compiles the core
for PowerPC and 68k inside the Retro68 container (podman). Licence: GPL-3.0-or-later.
