// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include <cstdint>
namespace globe {
enum class TerrainStyle : std::uint8_t { Hypsometric, CelShaded, Map, Natural };
constexpr TerrainStyle next_terrain_style(TerrainStyle style) {
    if (style == TerrainStyle::Map)
        return TerrainStyle::Natural;
    if (style == TerrainStyle::Natural)
        return TerrainStyle::Hypsometric;
#ifdef GLOBE_FLOPPY
    return TerrainStyle::Map; // Three looks on the floppy; Cel-shaded remains in the full edition.
#else
    return style == TerrainStyle::Hypsometric ? TerrainStyle::CelShaded : TerrainStyle::Map;
#endif
}
} // namespace globe
