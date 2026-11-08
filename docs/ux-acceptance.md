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
