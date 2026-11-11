# Classic Mac usability: hardware acceptance checklist

Build status is not acceptance. Each item needs the owner on **bare OS 9** (judge Tiger Classic only
for function, never speed). Record the build, the OS and the result next to each line.

## Cursors (#44)

Smoke test so far: the build with the pointing-hand cursor launched under Tiger Classic on the G4
(2026-11-08), rendered the globe and did not crash. The pointer was not moved, so no cursor was
observed.

1. Pointer over the globe, nothing selected, no button: open hand.
2. Pointer off the globe (inside the window): arrow.
3. Hold the button and drag: closed hand, also when the pointer leaves the globe.
4. Release: back to the right cursor for where the pointer is, without moving it.
5. Pointer over a city dot: pointing hand. Move off it: open hand.
6. Click a country (wait 400 ms): its card appears. Now the whole globe shows the pointing hand.
7. Click the sea: the selection clears and the open hand returns.
8. Press M (measure): the globe shows the pointing hand; press M again: open hand.
9. Click another application or the Finder: the arrow, no stale hand. Click back: the correct
   cursor at once.
10. Click the info card window and its scroll bar: the arrow (native).
11. A hand never stays when the pointer is over the menu bar, the title bar or the grow box.

If `SetThemeCursor` fails, the program falls back to `InitCursor`, the arrow.

## Looks palette (#45)

Built for PowerPC and host-tested only; it has not been run on any Macintosh (the G4 went offline
before it could be launched).

1. Press P: a small "Looks" window appears right of the globe with Map and Relief, a check on Map.
2. Click Relief: the globe re-renders as Relief and the check moves. Click Relief again: nothing
   redraws and no refinement starts.
3. Press S in the main window: the look switches and the palette check follows.
4. Click the palette while the main window is active: the first click only activates it; the next
   one chooses.
5. Close box hides it; P brings it back. Dragging it by the title bar works.
6. While the globe is refining, choose a look: input stays responsive; the old refinement is
   replaced.
7. Click back and forth between the palette, the info card and the globe: no stale cursor.
8. Quit with the palette open: clean exit.

## Menu bar (#46)

Built for PowerPC (Retro68, no warnings) and the menu logic host-tested (`test_menus`); it has not
been run on any Macintosh. Design: [menu-bar.md](design/menu-bar.md). Item numbers and ids are in
`include/globe/menus.hpp` and `os9/globe.r`. The program grew about 4.8 KB (542,336 to 547,200
bytes `.bin`); the resources are about 1 KB; no change to the SIZE partition.

1. The bar shows Apple, File, Edit, View. The Apple menu lists About Globe, a divider, then the
   desk accessories.
2. View: Zoom In/Out (Cmd-+, Cmd--) dim at the zoom limits. Align North dims when already north-up.
3. Graticule, Borders, Trackball Drag, Night Side and Measure show a check that matches the
   shortcut keys (G, B, T, D, M) and the toolbar-equivalent state.
4. Map/Relief: the check is on the current look; the other one switches and the palette check
   follows; choosing the current look does nothing (no redraw).
5. Cmd-Q and File > Quit exit cleanly, including mid-refinement.
6. Edit is dimmed unless a desk accessory is in front; with one in front Cut/Copy/Paste reach it.
7. Hold a menu open during refinement: the frame stays up, refinement resumes on release, and the
   title bar timing does not report the hold as a long event-loop turn.
8. Plain keys (no Cmd) behave as before; Cmd-P opens the palette.
9. About Globe opens an information window with the license line; the data credits are #11.
10. The cursor is the arrow over the menu bar; no stale hand afterward.
11. Compare Tiger Classic and bare OS 9 (the Apple menu differs).

### Tiger Classic smoke test, 2026-11-08 (not bare OS 9)

Build `18b08b5` (`GlobeFloppy` 546,172 byte application, Retro68 PowerPC), pushed as
`<temporary-test-binary>` and copied to `<temporary-test-binary>/` on the G4 (Mac OS X 10.4.11,
Classic). It launched, rendered Map with the title-bar timing (125 bands, 2748 ms, copy 890 ms,
turn 23 ms, step 9 ms; Classic under Tiger is much slower than bare OS 9, so these are not
comparable), and the menu bar showed File, Edit and View (plus the Help menu Classic adds). No input
can be sent remotely, so steps 1 to 11 above are all still open: menu tracking, check marks, Cmd keys
and the About window were **not** exercised. The instance was left running.

**Observed after the smoke test (owner at the console, 2026-11-08):** the Tiger pointer stopped
responding for a while, then recovered, but the Globe window could not be closed. Classic was
ended by killing `TruBlueEnvironment` with the owner's approval. Cause unknown: the owner did not
report whether menus, Cmd-Q or the close box had worked first, and the Globe build under test was
`18b08b5`. Re-test the menus and Quit on a fresh Classic launch, and on bare OS 9, before treating
this as a Classic artifact; a hang in `MenuSelect`/`MenuKey` handling is still possible.
