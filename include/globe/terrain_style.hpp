// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include <cstdint>
namespace globe {
// Map is the Google-Maps-like default; Relief is a plain hypsometric tint.
enum class TerrainStyle : std::uint8_t { Map, Relief };
constexpr TerrainStyle next_terrain_style(TerrainStyle style) {
    return style == TerrainStyle::Map ? TerrainStyle::Relief : TerrainStyle::Map;
}
} // namespace globe
