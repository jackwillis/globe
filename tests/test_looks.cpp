// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/looks.hpp"
using namespace globe;
int main() {
    // The rows and the looks agree both ways.
    CHECK(looks_style(0) == TerrainStyle::Map && looks_style(1) == TerrainStyle::Relief);
    CHECK(looks_row(TerrainStyle::Map) == 0 && looks_row(TerrainStyle::Relief) == 1);
    // Choosing the look already shown, or no row, is no command: nothing redraws or refines.
    CHECK(!looks_pick(TerrainStyle::Map, 0));
    CHECK(!looks_pick(TerrainStyle::Relief, 1));
    for (const int bad : {-1, kLooksRows, 99})
        CHECK(!looks_pick(TerrainStyle::Map, bad));
    CHECK(looks_pick(TerrainStyle::Map, 1) == Command::CycleStyle);
    CHECK(looks_pick(TerrainStyle::Relief, 0) == Command::CycleStyle);

    // The command flows through update(): one switch, one redraw, no second source of truth.
    auto model = initial_model(640, 480);
    CHECK(model.terrain_style == TerrainStyle::Map);
    const auto pick = looks_pick(model.terrain_style, 1);
    CHECK(pick);
    if (pick) {
        const auto fx = update(model, CommandIssued{1000, *pick});
        CHECK(model.terrain_style == TerrainStyle::Relief && fx.redraw);
    }
    CHECK(looks_row(model.terrain_style) == 1);
    // Picking Relief again changes nothing.
    CHECK(!looks_pick(model.terrain_style, 1));
    // Back to Map through the same path; the palette check follows the model, not its own state.
    if (const auto back = looks_pick(model.terrain_style, 0))
        update(model, CommandIssued{2000, *back});
    CHECK(model.terrain_style == TerrainStyle::Map);
    // S cycles the looks; there is no palette to open (the list is always in the sidebar).
    CHECK(binding_for_char('s').command == Command::CycleStyle);
    return TEST_MAIN_RESULT();
}
