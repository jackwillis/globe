/* SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2026 Jack Willis
 * Application partition includes the terrain cache, framebuffer, GWorld and C++ runtime.
 */
#include "Types.r"
resource 'SIZE' (-1) {
    reserved, acceptSuspendResumeEvents, reserved, canBackground,
    needsActivateOnFGSwitch, backgroundAndForeground, dontGetFrontClicks,
    ignoreChildDiedEvents, is32BitCompatible, notHighLevelEventAware,
    onlyLocalHLEvents, notStationeryAware, dontUseTextEditServices,
    reserved, reserved, reserved,
    24 * 1024 * 1024, 12 * 1024 * 1024
};

/* Menu bar: ids and item numbers match include/globe/menus.hpp. */
#define ITEM(name, key) name, noIcon, key, noMark, plain
#define DIVIDER "-", noIcon, noKey, noMark, plain
resource 'MENU' (128, preload) {
    128, textMenuProc, allEnabled, enabled, apple,
    { ITEM("About Poodle Earth\0xC9", noKey); ITEM("Controls\0xC9", "?"); DIVIDER }
};
resource 'MENU' (129, preload) {
    129, textMenuProc, allEnabled, enabled, "File",
    { ITEM("Close", "W"); DIVIDER; ITEM("Quit", "Q") }
};
resource 'MENU' (130, preload) {
    130, textMenuProc, allEnabled, enabled, "Edit",
    { ITEM("Undo", "Z"); DIVIDER; ITEM("Cut", "X"); ITEM("Copy", "C"); ITEM("Paste", "V");
      ITEM("Clear", noKey); DIVIDER; ITEM("Preferences\0xC9", ",") }
};
resource 'MENU' (131, preload) {
    131, textMenuProc, allEnabled, enabled, "View",
    { ITEM("Zoom In", "+"); ITEM("Zoom Out", "-"); ITEM("Reset View", "R");
      ITEM("Align North", "N"); DIVIDER;
      ITEM("Graticule", "G"); ITEM("Borders", "B"); ITEM("Trackball Drag", "T"); DIVIDER;
      ITEM("Map", noKey); ITEM("Relief", noKey); DIVIDER;
      ITEM("Night Side", "D"); ITEM("Measure", "M") }
};
resource 'MBAR' (128, preload) { { 128, 129, 130, 131 } };

/* Startup failure: shown by main() when the application cannot be built (usually no memory). */
resource 'ALRT' (128, purgeable) {
    {80, 60, 215, 400}, 128,
    { OK, visible, sound1, OK, visible, sound1, OK, visible, sound1, OK, visible, sound1 },
    alertPositionMainScreen
};
resource 'DITL' (128, purgeable) {
    {
        {103, 262, 123, 330}, Button { enabled, "OK" };
        {10, 70, 95, 330}, StaticText { disabled,
            "Poodle Earth could not start. This is usually because too little memory is available. "
            "Quit other applications, or select Poodle Earth in the Finder, choose Get Info, and raise "
            "its Preferred size." };
    }
};
