// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The looks palette's logic, shared by the front ends: the rows and what choosing one does. The
// look itself stays in Model::terrain_style; choosing a row sends a command through update().
#pragma once
#include "globe/app.hpp"
#include <optional>
namespace globe {
constexpr int kLooksRows = 2;
// Row 0 is Map, row 1 is Relief; the order of TerrainStyle.
constexpr TerrainStyle looks_style(int row) {
    return row == 1 ? TerrainStyle::Relief : TerrainStyle::Map;
}
constexpr int looks_row(TerrainStyle style) {
    return style == TerrainStyle::Relief ? 1 : 0;
}
// The command for choosing `row` while `current` is shown: nothing for the look already shown or a
// row that does not exist (so no redraw, no new refinement), else the switch to the other look.
inline std::optional<Command> looks_pick(TerrainStyle current, int row) {
    if (row < 0 || row >= kLooksRows || looks_style(row) == current)
        return std::nullopt;
    return Command::CycleStyle;
}
static_assert(kLooksRows == 2 && next_terrain_style(TerrainStyle::Map) == TerrainStyle::Relief,
              "choosing the other look is CycleStyle only while there are two looks");
} // namespace globe
