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
    { ITEM("Close", "W"); DIVIDER; ITEM("Quit", "Q"); DIVIDER; ITEM("Run Script\0xC9", noKey); ITEM("Cancel Script", noKey) }
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

/* Window enlargement refused for lack of memory (#116): the text is Str::ResizeNeedsMemory,
 * passed through ParamText so that l10n/en.txt stays the only copy. */
resource 'ALRT' (129, purgeable) {
    {80, 60, 195, 400}, 129,
    { OK, visible, sound1, OK, visible, sound1, OK, visible, sound1, OK, visible, sound1 },
    alertPositionMainScreen
};
resource 'DITL' (129, purgeable) {
    {
        {83, 262, 103, 330}, Button { enabled, "OK" };
        {10, 70, 75, 330}, StaticText { disabled, "^0" };
    }
};

/* All confirmation is before the cooperative runner starts; no nested execution loop. */
resource 'ALRT' (130, purgeable) {
    {80, 60, 240, 460}, 130,
    { OK, visible, sound1, OK, visible, sound1, OK, visible, sound1, OK, visible, sound1 },
    alertPositionMainScreen
};
resource 'DITL' (130, purgeable) {
    {
        {120, 300, 140, 380}, Button { enabled, "Run" };
        {120, 14, 140, 94}, Button { enabled, "Cancel" };
        {120, 120, 140, 280}, Button { enabled, "Run and Save Output" };
        {10, 16, 105, 380}, StaticText { disabled,
            "Run script: ^0\rThe script may change the view. Save output chooses a new file before execution. "
            "During execution, use File > Cancel Script or Escape to stop." };
    }
};
resource 'ALRT' (132, purgeable) {
    {80, 60, 240, 460}, 132,
    { OK, visible, sound1, OK, visible, sound1, OK, visible, sound1, OK, visible, sound1 },
    alertPositionMainScreen
};
resource 'DITL' (132, purgeable) {
    {
        {120, 300, 140, 380}, Button { enabled, "OK" };
        {10, 16, 110, 380}, StaticText { disabled, "^0" };
    }
};
