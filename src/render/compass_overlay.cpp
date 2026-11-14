// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/compass_overlay.hpp"
#include "overlay_pen.hpp"
#include <cmath>

namespace globe {
namespace {
constexpr int kCenterX = 16;
constexpr int kCenterLift = kProgressBarRows + 13;
constexpr int kReach = 10;
constexpr int kRows = kCenterLift + kReach + 1;
constexpr int kMinWidth = 180;

bool fits(int width, int height) {
    return width >= kMinWidth && height >= kRows;
}

int nearest(double value) {
    const int truncated = static_cast<int>(value);
    const double fraction = value - truncated;
    return truncated + (fraction >= 0.5) - (fraction <= -0.5);
}
} // namespace

std::optional<ScreenPoint> compass_north(const Camera& camera) {
    // World +Y is the north pole. Its projected direction also represents the local north
    // tangent at the view center, even when the pole itself is beyond the visible hemisphere.
    const double x = camera.right.y, y = -camera.up.y;
    const double magnitude_squared = x * x + y * y;
    if (!(magnitude_squared > 1e-12))
        return std::nullopt;
    const double magnitude = std::sqrt(magnitude_squared);
    return ScreenPoint{x / magnitude, y / magnitude};
}

int compass_overlay_rows(int width, int height) {
    return fits(width, height) ? kRows : 0;
}

void draw_compass_rose(Framebuffer& fb, ScreenPoint north) {
    if (!fits(fb.width(), fb.height()))
        return;
    const int center_y = fb.height() - kCenterLift;
    overlay_detail::Pen pen{fb};
    stroke(pen, [&](auto put) {
        const auto point = [&](double along, double across) {
            put(kCenterX + nearest(along * north.x - across * north.y),
                center_y + nearest(along * north.y + across * north.x));
        };
        for (int distance = -5; distance <= kReach; ++distance)
            point(distance, 0);
        for (int distance = 1; distance <= 4; ++distance) {
            point(kReach - distance, distance);
            point(kReach - distance, -distance);
        }
        for (int distance = -5; distance <= 5; ++distance)
            point(0, distance);
    });
}
} // namespace globe
