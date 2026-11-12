# Shared presentation architecture — exploratory proposal

**Status:** Research/design, 2026-11-12. No adopted interface, new release gate, or authorization to refactor the running frontends.

**Problem:** Globe has one portable geographic application and two different presentation shells (native Classic Mac and X11). The desired future is *semantic feature parity* across native-feeling frontends, without maintaining independent application behaviors or recreating every system widget.

**Current baselines:** [Architecture](../architecture.md), accepted [ADR 0002](../adr/0002-rendering-shared-code-and-usability.md), [text layout](text-layout.md), [typography](typography.md), [menu bar](menu-bar.md), [#3 portable application epic](https://github.com/jackwillis/globe/issues/3), [#16 shared render lifecycle](https://github.com/jackwillis/globe/issues/16), [#23 geographic parity](https://github.com/jackwillis/globe/issues/23), [#36 labels](https://github.com/jackwillis/globe/issues/36), [#61 font roles](https://github.com/jackwillis/globe/issues/61), [#83 X11 sidebar](https://github.com/jackwillis/globe/issues/83). Geographic identity and navigation contracts are separately proposed in [PR #110](https://github.com/jackwillis/globe/pull/110).

## Candidate composition of patterns

Retain the **Elm Architecture** as the authoritative state-transition mechanism: `Model / Msg / update / Effects`, with deterministic replay and no platform UI mutation of geographic state. Borrow the **Presentation Model** (or MVVM's model-to-view-description idea) to derive user-visible state; use **Ports and Adapters** for native widgets and platform services. Do *not* add a second imperative MVC controller with its own geographic truth.

The proposed flow is: native user event -> semantic action/`Msg` -> `update` -> presentation description -> adapter updates widgets. Expensive work remains an effect and results return as validated messages. Existing core shortcuts, toolbar states, `globe/menus.hpp` mapping and `InfoCard` are starting points, not throwaway implementations.

| Shared responsibility | Native responsibility |
| --- | --- |
| Geographic selection, navigation, history, feature IDs | Toolkit event translation and OS-specific gestures |
| Action ID, allowed/enabled/checked state, meanings | Menu placement, keyboard conventions, controls/dialogs |
| Card content, information hierarchy, optional typed blocks | Widget creation, text editing and selection, window management |
| Text roles, label priorities, placement and synthetic-metrics tests | Font discovery, metrics, glyph rendering and encoding adapter |
| Viewport and optional pane-layout policy where it reduces duplication | DPI, native window constraints, focus, scrolling mechanics |
| Application-facing render/progress state | Native display submission and lifecycle |

**Ownership:** distinguish authoritative geographic/application state from optional UI session state (pane/scroll position, open tool, console visibility) and ephemeral native-widget state (focus, tracking, handles). Do not copy camera, selection, or resource lifetimes into two mutable models. Any `UiState` is a proposal; begin with derived immutable snapshots and add persisted state only when needed.

## Smallest useful contract

A possible *semantic* representation, **not** a prescribed C++ API:

```text
ActionId, action label/shortcut metadata -> enabled/checked state
Selection / navigation result -> CardView {title, typed content blocks, actions}
Application Model + bounded UI session state -> PresentationSnapshot
Native adapter: dispatch(action), present(snapshot), manage native controls
```

Start with existing cards rather than creating a generic DOM or toolkit. If richer hypermedia requires it, introduce a *bounded* vocabulary of heading, paragraph, fact, link, divider and fixed-image blocks. Links must carry stable semantic targets, not platform pointers. Reflow and hit testing can be shared where proven useful; native font measurement and native text editing remain platform-specific.

A semantic action vocabulary also provides a future input for keyboard mapping, scripting/Listener observation, tests and accessibility. It should never let scripts or widgets mutate the Model outside `update()`. This is related to the exploration in [PR #110](https://github.com/jackwillis/globe/pull/110) and [#63](https://github.com/jackwillis/globe/issues/63); neither interpreter nor public scripting API is selected.

## Vertical slice: United States -> Wisconsin -> Madison -> Back

**Before touching existing architecture**, test a narrow path with available geography (if a required administrative level is absent, use a controlled fixture and flag that distinction):

1. A selection event carries an entity ID into the shared application update; resolution results are owned and generation checked.
2. A small derived presentation snapshot reports card content, navigation availability, selected style and enabled/checked menu actions.
3. Existing OS 9 and X11 frontends consume this snapshot and produce *semantically identical* results. Different widget arrangements and font pixels are expected.
4. Synthetic-metrics tests prove ordering, wrapping, placement, invalidation and long/missing-glyph behavior where text layout is involved.
5. Verify no new frame-loop allocation, no second authoritative selection store, no regression in measured G4 responsiveness/partition peaks, and no change to existing replay/golden-image contracts.

The experiment must also compare code/behavior duplication and adapter complexity *before and after*. A new abstraction is justified only if it reduces actual duplicated decisions, improves parity tests, or enables a real third frontend without significant runtime cost. A failed result is grounds for keeping small explicit per-platform implementations.

## Typography boundary and proposed alternate experiment

The accepted current typography policy remains **installed fonts only, no bundled font or glyph packs**, with role-based Helvetica/Palatino mappings on Classic Mac. [Text layout](text-layout.md) currently recommends *native drawing* for cartographic labels. It already identifies *system-generated glyph bitmaps plus shared framebuffer composition* as alternative B.

The proposed research is to compare A (draw native text on the final presentation surface) against B (measure/rasterize via system font APIs, cache bounded glyph masks, compose into the final shown frame). Keep labels **out of reusable terrain and zoom-preview source images**. A logical text layer can consist of positioned runs, not a full-screen bitmap. Record cache memory, build/runtime cost, text quality, source-font redistribution/legal constraints, cancellation, cross-OS encoding and G4 latency. Do not adopt B until it wins a real test. `libschrift`/FreeType are additional research alternatives, **not dependencies or authority to bundle fonts**.

## Risks and explicit non-goals

- OS 9 has bounded memory partitions and Toolbox restrictions; no mandatory GUI framework or virtual-DOM diffing.
- Different installed fonts and Unicode coverage mean identical semantic content need not give pixel-identical output. Missing glyphs and source name fallback must be represented honestly.
- Native focus, clipboard, accessibility and text selection are more subtle than drawing; keep OS-appropriate controls where possible.
- Do not expose generic layout/style DSLs, CSS, virtual widgets or rich text shaping without a demonstrated need.
- [ADR 0002](../adr/0002-rendering-shared-code-and-usability.md) rejects a generic frontend coordinator for the then-small duplication; this document does **not** reverse it. A third frontend and the proposed parity experiment may justify a new decision later.

**Exit decision:** after the vertical slice, choose *retain existing adapters*, *extract narrow shared presentation functions*, or *propose an ADR for a broader contract*. No production change or milestone is implied by this note.
