// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/menus.hpp"
using namespace globe;
int main() {
    using Overlay = KeyBinding::Overlay;
    const auto choice = [](short menu, short item, TerrainStyle look = TerrainStyle::Map) {
        return menu_choice(menu, item, look);
    };
    CHECK(choice(kAppleMenu, kAboutItem).about);
    CHECK(choice(kFileMenu, kQuitItem).command == Command::Quit);
    CHECK(choice(kViewMenu, kZoomInItem).command == Command::ZoomIn);
    CHECK(choice(kViewMenu, kZoomOutItem).command == Command::ZoomOut);
    CHECK(choice(kViewMenu, kResetItem).command == Command::Reset);
    CHECK(choice(kViewMenu, kAlignNorthItem).command == Command::AlignNorth);
    CHECK(choice(kViewMenu, kGraticuleItem).command == Command::ToggleGraticule);
    CHECK(choice(kViewMenu, kBordersItem).command == Command::ToggleBorders);
    CHECK(choice(kViewMenu, kTrackballItem).command == Command::ToggleDragMode);
    CHECK(choice(kViewMenu, kNightItem).overlay == Overlay::Night);
    CHECK(choice(kViewMenu, kMeasureItem).overlay == Overlay::Measure);
    // The look already shown is no command; the other look is the same switch as the palette.
    CHECK(!choice(kViewMenu, kMapItem, TerrainStyle::Map).command);
    CHECK(choice(kViewMenu, kReliefItem, TerrainStyle::Map).command == Command::CycleStyle);
    CHECK(!choice(kViewMenu, kReliefItem, TerrainStyle::Relief).command);
    CHECK(choice(kViewMenu, kMapItem, TerrainStyle::Relief).command == Command::CycleStyle);
    // Dividers, Edit and unknown items mean nothing; so does the wrong menu.
    for (const short item :
         {short(kViewDivider1), short(kViewDivider2), short(kViewDivider3), short(0), short(99)}) {
        const auto c = choice(kViewMenu, item);
        CHECK(!c.command && c.overlay == Overlay::Nothing && !c.about);
    }
    for (short item = 0; item < 8; ++item) {
        const auto c = choice(kEditMenu, item);
        CHECK(!c.command && c.overlay == Overlay::Nothing && !c.about);
    }
    CHECK(choice(kFileMenu, kCloseItem).close && !choice(kFileMenu, kCloseItem).command);
    CHECK(!choice(kFileMenu, 2).command && !choice(kFileMenu, 2).close && !choice(300, 1).command);

    // Every View command item agrees with the shortcut key for the same thing.
    CHECK(binding_for_char('g').command == choice(kViewMenu, kGraticuleItem).command);
    CHECK(binding_for_char('b').command == choice(kViewMenu, kBordersItem).command);
    CHECK(binding_for_char('t').command == choice(kViewMenu, kTrackballItem).command);
    CHECK(binding_for_char('n').command == choice(kViewMenu, kAlignNorthItem).command);
    CHECK(binding_for_char('r').command == choice(kViewMenu, kResetItem).command);
    CHECK(binding_for_char('q').command == choice(kFileMenu, kQuitItem).command);
    CHECK(binding_for_char('d').overlay == Overlay::Night);
    CHECK(binding_for_char('m').overlay == Overlay::Measure);
    CHECK(binding_for_char('p').overlay == Overlay::Nothing); // the looks list is always showing

    // State follows the model, through update().
    auto model = initial_model(640, 480);
    auto state = [&](short item, bool night = false, bool measuring = false) {
        return view_item_state(model, night, measuring, item);
    };
    CHECK(state(kMapItem).checked && !state(kReliefItem).checked);
    CHECK(!state(kGraticuleItem).checked == !model.layers.graticule);
    CHECK(!state(kAlignNorthItem).enabled); // home view is already north-up
    CHECK(state(kNightItem, true).checked && state(kMeasureItem, false, true).checked);
    CHECK(!state(kNightItem).checked && !state(kMeasureItem).checked);
    update(model, CommandIssued{1000, Command::CycleStyle});
    CHECK(!state(kMapItem).checked && state(kReliefItem).checked);
    const bool grat = model.layers.graticule;
    update(model, CommandIssued{1001, Command::ToggleGraticule});
    CHECK(state(kGraticuleItem).checked == !grat);
    update(model, CommandIssued{1002, Command::ToggleDragMode});
    CHECK(state(kTrackballItem).checked);
    // Zoom limits disable the matching item.
    for (int i = 0; i < 60; ++i)
        update(model, CommandIssued{2000u + i, Command::ZoomIn});
    CHECK(!state(kZoomInItem).enabled && state(kZoomOutItem).enabled);
    for (int i = 0; i < 120; ++i)
        update(model, CommandIssued{3000u + i, Command::ZoomOut});
    CHECK(state(kZoomInItem).enabled && !state(kZoomOutItem).enabled);
    return 0;
}
