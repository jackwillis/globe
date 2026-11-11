// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once
#include "globe/geometry.hpp"
#include <cstddef>
#include <cstdint>
namespace globe::detail {
// Construction only: dimensions are nonzero and storage holds (width+1)*(height+1) vectors.
// Use the unpublished first row for longitude factors, then write it last. No extra allocation.
// Calling to_unit_vector retains its longitude wrapping and latitude clamping. Multiplication by
// cos(0)==1 leaves each factor unchanged, so the final products match the original per-corner call.
inline void initialize_terrain_world(Vec3* world, std::uint32_t width, std::uint32_t height,
                                     double west, double east, double south, double north) {
    for (std::uint32_t x = 0; x <= width; ++x) {
        const double lon = west + (east - west) * x / width;
        world[x] = to_unit_vector({0, lon});
    }
    for (std::uint32_t y = height;; --y) {
        const double lat = south + (north - south) * y / height;
        const auto row = to_unit_vector({lat, 0});
        for (std::uint32_t x = 0; x <= width; ++x) {
            const auto column = world[x];
            world[static_cast<std::size_t>(y) * (width + 1) + x] = {row.z * column.x, row.y,
                                                                    row.z * column.z};
        }
        if (y == 0)
            break;
    }
}
} // namespace globe::detail
