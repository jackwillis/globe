# Classic Mac menu bar: design for #46

Status: **design only**, not implemented. The cursor and palette changes were kept separate and
reviewable; the menu bar is a follow-up ticket (scope below). Header facts were checked against the
Retro68 Multiversal `Multiverse.h`: `InsertMenu`, `AppendMenu`, `AppendResMenu`, `GetNewMBar`,
`SetMenuBar`, `DrawMenuBar`, `MenuSelect`, `MenuKey`, `HiliteMenu`, `EnableItem`, `DisableItem`,
`CheckItem`, `SetItemCmd`, `SystemEdit` and `OpenDeskAcc` are all declared. `main()` already calls
`InitMenus()` and nothing installs a menu bar.

## Hierarchy and command ids

Menu ids: Apple 128, File 129, Edit 130, View 131. Item numbers are 1-based; `-` is a divider.

| Menu | Item | Key | Sends |
|---|---|---|---|
| Apple | About Globe... | | front end: info window with `DataCredit` text (#11) |
| Apple | (desk accessories) | | `AppendResMenu('DRVR')`, `OpenDeskAcc` |
| File | Quit | Cmd-Q | `Command::Quit` |
| Edit | Undo, Cut, Copy, Paste, Clear | Z X C V | `SystemEdit(item-1)` only; disabled while no desk accessory is front |
| View | Zoom In | Cmd-+ | `Command::ZoomIn` |
| View | Zoom Out | Cmd-- | `Command::ZoomOut` |
| View | Reset View | Cmd-R | `Command::Reset` |
| View | Align North | Cmd-N | `Command::AlignNorth` (disabled when already north-up) |
| View | Graticule | Cmd-G | `Command::ToggleGraticule`, check mark |
| View | Borders | Cmd-B | `Command::ToggleBorders`, check mark |
| View | Trackball Drag | Cmd-T | `Command::ToggleDragMode`, check mark |
| View | Map | | `looks_pick(style, 0)`, check on the current look |
| View | Relief | | `looks_pick(style, 1)`, check on the current look |
| View | Looks Palette | Cmd-P | open the palette window |
| View | Night Side, Measure | Cmd-D, Cmd-M | overlay toggles handled by the front end, as today |

Single-letter shortcuts stay as they are (Cmd is additive: `MenuKey` runs first only when Cmd is
down, otherwise the key goes to the existing shared table).

## Event flow

1. `mouseDown` with `FindWindow` = `inMenuBar`: `MenuSelect(event.where)`, split the long result
   into menu id and item, dispatch, then `HiliteMenu(0)`.
2. `keyDown` with Cmd: `MenuKey(ch)`; same dispatch. Without Cmd: today's `key()`.
3. Dispatch maps (menu, item) to a `Command` through one table and calls
   `apply(CommandIssued{time, command})`, so menus add no UI-specific behavior. Overlay items call
   the same front-end functions as their keys.
4. Menu state comes from `toolbar_state(model)` (`checked`, `enabled`), refreshed with
   `CheckItem`/`EnableItem` when the menu is about to be shown (after `MenuSelect` is too late, so
   refresh on `mouseDown` in the menu bar and on every Cmd-key, before the lookup).
5. Progressive refinement: `MenuSelect` runs its own loop and blocks the application's event loop
   while a menu is held. The renderer is cooperative and runs from that loop, so refinement pauses
   while the menu is down (the current frame stays on screen). That is the standard Macintosh
   behavior; it needs no special handling, but the first turn after the menu closes must not count
   the hold in `max_turn_ms_` (reset `turn_start` after `MenuSelect`).
6. After a menu is used, the area it covered comes back through an update event, which already
   repaints from the cached GWorld without re-rendering.

## Resources

Prefer resources over `AppendMenu` calls: `MENU` 128-131 and an `MBAR` 128 in `os9/globe.r`, built
with the existing Rez step; `GetNewMBar`/`SetMenuBar`/`AppendResMenu`/`DrawMenuBar` at start. They
cost a few hundred bytes of resource data and no heap beyond the menu handles. If the Rez
`MENU` template is awkward in the Retro68 build, `AppendMenu("\p...")` with the same ids is the
fallback and is equivalent.

## Open points that need the owner or hardware

- About Globe: whether it opens the existing info window or a small dialog with the `DataCredit`
  file text (#11). Credits must come from existing attribution data, not new notice text.
- Whether the app should survive a Quit that arrives while refinement is running (it does today).
- Menu behavior under bare OS 9 vs Tiger Classic (the latter has a different Apple menu).
- Cursor: the arrow must show over the menu bar (the cursor rule already returns the arrow for any
  pointer off the globe).

## Scope and risk for the follow-up

Small: one resource file addition, one dispatch table (testable on the host as a pure function
from (menu, item) to a command, plus a state table from `toolbar_state`), and about 80 lines in the
event loop. Risk is the menu-hold turn-length accounting and the Edit menu with desk accessories.
Recommended as its own ticket, after the owner has run the palette on hardware.
