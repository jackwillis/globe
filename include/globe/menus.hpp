// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The menu bar's logic, shared so it can be tested without a Toolbox: which command or overlay a
// (menu, item) pair means, and whether the item is enabled or checked. Choosing an item sends the
// same Command (through update()) or runs the same front-end overlay as its key; the menus add no
// behavior of their own. Item numbers are 1-based and match the MENU resources in os9/globe.r.
#pragma once
#include "globe/app.hpp"
#include "globe/looks.hpp"
#include <optional>
namespace globe {
constexpr short kAppleMenu = 128, kFileMenu = 129, kEditMenu = 130, kViewMenu = 131;
constexpr short kAboutItem = 1; // Apple; desk accessories follow the divider
constexpr short kQuitItem = 1;  // File
enum ViewItem : short {
    kZoomInItem = 1,
    kZoomOutItem,
    kResetItem,
    kAlignNorthItem,
    kViewDivider1,
    kGraticuleItem,
    kBordersItem,
    kTrackballItem,
    kViewDivider2,
    kMapItem,
    kReliefItem,
    kPaletteItem,
    kViewDivider3,
    kNightItem,
    kMeasureItem,
    kViewItemCount = kMeasureItem,
};
struct MenuChoice {
    std::optional<Command> command; // sent through update()
    KeyBinding::Overlay overlay = KeyBinding::Overlay::Nothing;
    bool about = false; // the front end shows its About window
};
// What choosing the item means while `current` is the look shown. Choosing the look already shown
// is nothing (no redraw, no new refinement), as in the palette. Edit and the desk accessories are
// the front end's, so they give nothing here.
inline MenuChoice menu_choice(short menu, short item, TerrainStyle current) {
    MenuChoice c;
    using Overlay = KeyBinding::Overlay;
    if (menu == kAppleMenu && item == kAboutItem)
        c.about = true;
    else if (menu == kFileMenu && item == kQuitItem)
        c.command = Command::Quit;
    else if (menu == kViewMenu) {
        switch (item) {
        case kZoomInItem:
            c.command = Command::ZoomIn;
            break;
        case kZoomOutItem:
            c.command = Command::ZoomOut;
            break;
        case kResetItem:
            c.command = Command::Reset;
            break;
        case kAlignNorthItem:
            c.command = Command::AlignNorth;
            break;
        case kGraticuleItem:
            c.command = Command::ToggleGraticule;
            break;
        case kBordersItem:
            c.command = Command::ToggleBorders;
            break;
        case kTrackballItem:
            c.command = Command::ToggleDragMode;
            break;
        case kMapItem:
            c.command = looks_pick(current, 0);
            break;
        case kReliefItem:
            c.command = looks_pick(current, 1);
            break;
        case kPaletteItem:
            c.overlay = Overlay::Palette;
            break;
        case kNightItem:
            c.overlay = Overlay::Night;
            break;
        case kMeasureItem:
            c.overlay = Overlay::Measure;
            break;
        default:
            break;
        }
    }
    return c;
}
struct MenuItemState {
    bool enabled = true, checked = false;
};
// The state of a View item, from the model and the two front-end overlays. Dividers and items
// without a state are enabled and unchecked.
inline MenuItemState view_item_state(const Model& model, bool night, bool measuring, short item) {
    const auto state = toolbar_state(model);
    const auto of = [&](Command command) { return state[static_cast<std::size_t>(command)]; };
    MenuItemState s;
    switch (item) {
    case kZoomInItem:
        s.enabled = of(Command::ZoomIn).enabled;
        break;
    case kZoomOutItem:
        s.enabled = of(Command::ZoomOut).enabled;
        break;
    case kAlignNorthItem:
        s.enabled = of(Command::AlignNorth).enabled;
        break;
    case kGraticuleItem:
        s.checked = of(Command::ToggleGraticule).checked;
        break;
    case kBordersItem:
        s.checked = of(Command::ToggleBorders).checked;
        break;
    case kTrackballItem:
        s.checked = of(Command::ToggleDragMode).checked;
        break;
    case kMapItem:
        s.checked = model.terrain_style == TerrainStyle::Map;
        break;
    case kReliefItem:
        s.checked = model.terrain_style == TerrainStyle::Relief;
        break;
    case kNightItem:
        s.checked = night;
        break;
    case kMeasureItem:
        s.checked = measuring;
        break;
    default:
        break;
    }
    return s;
}
} // namespace globe
