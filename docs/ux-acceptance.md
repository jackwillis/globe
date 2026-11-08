# Classic Mac usability: hardware acceptance checklist

Build status is not acceptance. Each item needs the owner on **bare OS 9** (judge Tiger Classic only
for function, never speed). Record the build, the OS and the result next to each line.

## Cursors (#44)

Smoke test so far: the build with the pointing-hand cursor launched under Tiger Classic on maul
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

Built for PowerPC and host-tested only; it has not been run on any Macintosh (maul went offline
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

Design only ([menu-bar.md](design/menu-bar.md)); nothing to test yet.
