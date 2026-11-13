# Shared presentation architecture — exploratory proposal

**Status:** Nonbinding research/design, 2026-11-12, condensed 2026-11-13. No adopted interface, release gate or authorization to refactor the running frontends. The decision it feeds is tracked in [#114](https://github.com/jackwillis/globe/issues/114); geographic identity and the navigation journey are defined in [PR #110](https://github.com/jackwillis/globe/pull/110) and are not redefined here.

**Question:** Globe has one portable application and two presentation shells (native Classic Mac and X11). Would a small shared description of *semantic* presentation decisions reduce drift between frontends, without recreating system widgets? The honest alternative (H0 in #114) is that today's small explicit per-frontend translations are cheaper.

**Baselines:** [architecture](../architecture.md), accepted [ADR 0002](../adr/0002-rendering-shared-code-and-usability.md), [text layout](text-layout.md), [typography](typography.md), [menu bar](menu-bar.md), and issues [#3](https://github.com/jackwillis/globe/issues/3), [#16](https://github.com/jackwillis/globe/issues/16), [#23](https://github.com/jackwillis/globe/issues/23), [#36](https://github.com/jackwillis/globe/issues/36), [#61](https://github.com/jackwillis/globe/issues/61), [#83](https://github.com/jackwillis/globe/issues/83), [#114](https://github.com/jackwillis/globe/issues/114).

## What already exists (inventory before proposing anything)

| Existing structure | Already shared | Stays per frontend |
| --- | --- | --- |
| `Command` (`globe/app.hpp`) | The enumerated set of user actions (move, zoom, reset, toggles, align north, drag mode, quit, cycle style) | — |
| `CommandIssued` in `Msg` | Delivery of a command, with its time, through `update()` | Producing the command from native input |
| `binding_for_char`, `KeyBinding` | Character → command or frontend-handled overlay; documented as "the part that must not drift" | Decoding native key events |
| `menu_choice` (`globe/menus.hpp`) | (menu, item) → `Command`, overlay, About or Close; which items are enabled or checked; item numbers match the Toolbox `MENU` resources | The menu bar widgets; Toolbox/X11 specifics |
| `InfoCard` (`globe/countries.hpp`), `card_text`, word wrap (`globe/wrap.hpp`) | Card content (title, up to 12 lines of borrowed `string_view`), clipboard text, wrapping at a platform-measured width | Drawing, the sidebar/info window, text measurement and editing |
| `Overlays` and `ShownFrame` | Which overlays (night, measure, focused country) and progress bar are composed, and when, on a copy of the rendered frame | Pixel conversion and submission |
| `RefineRunner`, `ZoomPreview` | Job start/cancel, preview, step, place markers, moving-frame rendering | Event loop, wake timers, window and pixel APIs |

Consequence: a shared **action vocabulary** and a shared **menu/shortcut mapping** already exist. Do not introduce a competing `ActionId`, enabled/checked table or second card representation; new actions extend `Command` and `menu_choice`. What the repository does *not* yet have is a card model for selected entities, and that is defined once, as `EntitySummary`, in the [PR #110 contract](https://github.com/jackwillis/globe/pull/110). Any presentation snapshot would be derived from it, not defined in parallel.

## Candidate composition

Keep the Elm-style core authoritative: native event → `Command`/`Msg` → `update` → derived presentation → adapter updates widgets. Borrow the *Presentation Model* idea (derive user-visible state from the Model) and *ports and adapters* for native widgets. No second imperative controller with its own geographic truth, and expensive work stays an effect.

A semantic action vocabulary is also a possible input for keyboard mapping, tests, accessibility and the optional Listener/command-observation idea ([#63](https://github.com/jackwillis/globe/issues/63)); none of these may mutate the Model outside `update()`.

**Ownership:** distinguish authoritative application state (camera, selection, history), optional UI session state (pane or scroll position, open tool) and ephemeral native-widget state (focus, tracking, handles). Never copy camera or selection into a second mutable model. Begin, if at all, with derived immutable snapshots; add persisted UI state only when needed.

## Test for the idea

[#114](https://github.com/jackwillis/globe/issues/114) owns the experiment and its decision gate. In outline: inventory actual duplicated decisions across OS 9 and X11 (the table above is the starting list); run the single geographic journey defined in the PR #110 contract through both frontends and require *semantically* identical state transitions, navigation and card content (not pixel-identical UI); compare code, allocation, partition impact and responsiveness against the current design. The X11 sidebar (#83) is still in progress, so drift measured before it lands may overstate or understate the need.

**Exit:** retain existing adapters, extract narrow shared functions, or propose a separate owner-reviewed ADR. A failed or neutral result is a valid outcome, and per [ADR 0002](../adr/0002-rendering-shared-code-and-usability.md) (D6) a generic coordinator stays rejected until a demonstrated need.

## Typography boundary

Accepted policy: installed fonts, no bundled font or glyph data ([typography](typography.md)); placement is portable and glyph drawing native ([text layout](text-layout.md), recommendation A). Compositing system-generated glyph masks into the shared framebuffer (alternative B in `text-layout.md`) is an **alternative to evaluate under [#36](https://github.com/jackwillis/globe/issues/36), not an adopted design**. Its prerequisites are the native-drawing baseline measurement that `text-layout.md` already names, and review of the memory and licensing points in `typography.md`. Labels stay out of reusable terrain and zoom-preview source images in either case.

## Risks and non-goals

- OS 9 has bounded partitions and Toolbox restrictions: no mandatory GUI framework, virtual-DOM diffing or generic layout/style language.
- Different installed fonts and Unicode coverage mean identical semantics need not give identical pixels; missing glyphs and name fallback must be represented honestly.
- Focus, clipboard, accessibility and text selection are subtle; keep OS-appropriate controls.
- No production change, milestone or release-gate change follows from this note.

Rendering backends, execution strategies and display adaptation are covered separately in [rendering backends and execution](../research/rendering-backends-and-execution.md).
