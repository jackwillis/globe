# ADR 0006 — Where script-runner state lives

**Status:** Proposed, 2026-11-20. Question raised during native acceptance of [#221](https://github.com/jackwillis/poodle-earth/issues/221) / PR #222. Nothing here changes PR #222, which was reviewed and tested as an adapter-level runner. Not accepted until the owner decides.

## Context

The first Classic script runner (`pe-script/1`) keeps its progress outside the Model: script position, the pending wait and the transcript sink live in `src/platform/common/script.*`. Each event-loop turn it reads the Model plus native presentation flags (`ScriptPresentation`) and dispatches existing messages such as `CommandIssued{now_ms, Command::ToggleBorders}`. `src/core` is unchanged. Time enters as `now_ms` from the monotonic adapter.

AGENTS.md requires the Elm-style Model / Msg / update / Effects core.

Scripted input already exists in core in one form: `include/globe/trace.hpp` and `src/core/trace.cpp` turn text lines into `Msg` values, and the X11 `--script` replays them. That is **open-loop**: it sends messages and observes nothing. The PR's runner is **closed-loop**: `get`, `assert` and `wait settled` read the Model and the presentation state, and it writes a JSONL transcript.

Review found a P0 in the first version: with presentation already idle, the wait slept until its timeout and then failed. Host tests had not covered it. The fix is tested by `idle_wait` in `tests/test_script.cpp`, which runs the real runner, the wake helper and the presentation queue inside a **hand-written copy of the outer event loop**. The real wiring is in `src/platform/os9/main.cpp` (`WaitNextEvent` with `runner.wake_at(...)`, and the `ScriptPresentation` flags). No host test exercises that file.

## Options

**A. Adapter-level runner (as in PR #222).** Core stays small. Sequencing and wait logic are host-tested through the adapter. Native flags are read directly. The event-loop wiring in `main.cpp` is covered only by native runs, and the `idle_wait` loop can drift from it.

**B. Script state in the Model.** `update` owns script position and the pending wait. Frontends send facts as messages (script loaded, `PresentationSettled{generation, revision}`, wake) and handle effects (write record, schedule wake). Wait logic and sequencing become deterministic host tests, and the presentation facts that the adapter reads today become messages the tests can feed in. X11 and OS 9 could share it, and closed-loop scripting would sit beside the trace parser. The Model grows, and "settled" is still a frontend fact that the Model learns only by message.

## Propagation (option B), traced one hop from the current tree

- `Msg` is a `std::variant`. There is no exhaustive `std::visit` in `src`, `include` or `tests`; consumers use `holds_alternative` or `get_if` (OS 9 `main.cpp`, X11 `main.cpp`, `trace.cpp`). Adding alternatives compiles everywhere.
- `Effects` is a plain aggregate. Direct `update()` callers are `x11/main.cpp` and `bench/bench.cpp`; new defaulted fields keep them working. The OS 9 path goes through `Application` and is not yet traced.
- `Model` is copied, and the refinement job holds a snapshot of layers. Script state in `Model` would be copied with it. Whether that is harmless is not yet checked.
- Hop two (`Application`, the refinement runner) is not traced.

## Questions to answer before deciding

1. Should closed-loop scripting live in core next to the open-loop trace parser, or stay an adapter feature?
2. Does moving presentation facts into messages let host tests cover the event-loop wiring that `idle_wait` currently copies by hand?
3. Cost of moving: re-review, and re-acceptance of the native build. Plus the unchecked `Model` snapshot question above.

## Evidence so far

The first native runs on a Power Mac G4 (Mac OS 9.2.2, 2026-11-20) completed the idle wait in about a second with no input, twice. That is the first end-to-end evidence for the wiring that host tests cannot reach under option A. It is an owner observation of one candidate build, not a measured bound.

## Decision

None yet. Recommendation for discussion: keep A for PR #222; revisit B before a second scripted-input feature (for example drop or double-click, #223) or `pe-script/2` (#224).
