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
