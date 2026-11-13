# Codex notes

These notes add to [AGENTS.md](../../AGENTS.md).

## Testing and quality gates

Start with the smallest relevant tests, then verify the portable build and cross-target boundary:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
```

Check current scripts/flags rather than assuming the archived environment:
- `tools/probe-retro68.sh` checks the portable source subset for PowerPC and 68k compatibility; **68k compile does not mean 68k app/runtime support**.
- `tools/build-edition.sh` and `tools/build-os9.sh` build/package Classic targets; only run with a suitable configured Retro68 environment and relevant task. Check binary forks, size/report and format/policy checks after source additions.
- Code formatting/static analysis: use repository-supported clang-format/clang-tidy workflows, noting host lint does not cover Toolbox-only code. Watch sanitizer timing sensitivity in `x11_toolbar` (fixed 700-ms wait can fail under load); reproduce before labeling a regression.
- Rendering refactors: maintain deterministic trace and final-frame tests where output is unchanged. When output intentionally changes (e.g., limb clipping), update pins only with a documented decision, independent/oracle tests and a recorded G4 cost. Test job cancellation, stale/duplicate completions, multiple continuation budgets, viewport resize and foreground/background transitions. **No promises of wall-time bounds from counting work units alone.**

Before authorized network work, read the [fetch design](../fetch-design.md) for TLS trust and entropy policy.
