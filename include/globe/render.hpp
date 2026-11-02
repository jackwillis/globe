// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Draws the globe: ocean disc, graticule, coastlines, borders (design doc section 6 layer order,
// minus the layers that do not exist yet). Pure function of (camera, atlas, palette, layers):
// the same inputs give the same pixels within one build.
#pragma once

#include <cstdint>

#include "globe/atlas.hpp"
#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"

namespace globe {

struct Palette {
    Rgb565 background;
    Rgb565 ocean;
    Rgb565 graticule;
    Rgb565 coastline;
    Rgb565 border;
};

struct Layers {
    bool graticule = true;
    bool coastlines = true;
    bool borders = true;
};

// What a frame cost, in countable terms (not time; timing belongs to the platform layer).
struct RenderStats {
    std::uint32_t segments = 0;         // atlas and graticule segments considered
    std::uint32_t segments_visible = 0; // of those, with any part on the front hemisphere
    std::uint32_t lines_drawn = 0;      // straight line pieces rasterized after subdivision
};

constexpr Palette kDefaultPalette = {
    rgb565(8, 12, 24),     // background
    rgb565(24, 64, 120),   // ocean
    rgb565(40, 90, 150),   // graticule
    rgb565(230, 220, 170), // coastline
    rgb565(200, 90, 80),   // border
};

// Clears the framebuffer to the background and draws one frame.
RenderStats render_globe(Framebuffer& fb, const Camera& camera, const Atlas& atlas,
                         const Palette& palette, const Layers& layers);

} // namespace globe
