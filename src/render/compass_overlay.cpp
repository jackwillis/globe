// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/compass_overlay.hpp"
#include "globe/overlays.hpp"
#include <algorithm>
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

constexpr double kCore = 0.6;
constexpr double kEdge = 1.75;
// Every segment lies within radius kReach of the origin. kReach + kEdge is less
// than 12, so no integer pixel offset can exceed 11 on either axis.
constexpr int kStrokeReach = kReach + static_cast<int>(kEdge);
static_assert(kCenterX >= kStrokeReach && kCenterLift - kStrokeReach > kProgressBarRows);
static_assert(kRows == kCenterLift + kStrokeReach);

double square(double value) {
    return value * value;
}

double stroke_distance_squared(double along, double across) {
    const double shaft = square(along - std::clamp(along, -5.0, 10.0)) + square(across);
    const double crossbar = square(along) + square(across - std::clamp(across, -5.0, 5.0));
    // Reflect across the shaft to test the nearer of the two symmetric barbs.
    const double side = std::abs(across);
    const double from_tip = 10.0 - along;
    const double step = std::clamp((from_tip + side) * 0.5, 0.0, 4.0);
    const double barb = square(from_tip - step) + square(side - step);
    return std::min({shaft, crossbar, barb});
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
    for (int y = -kStrokeReach; y <= kStrokeReach; ++y) {
        for (int x = -kStrokeReach; x <= kStrokeReach; ++x) {
            if (x * x + y * y > square(kReach + kEdge))
                continue;
            const double along = x * north.x + y * north.y;
            const double across = y * north.x - x * north.y;
            const double distance = stroke_distance_squared(along, across);
            if (distance <= square(kEdge))
                fb.set_pixel(kCenterX + x, center_y + y,
                             distance <= square(kCore) ? kProgressFill : kProgressEdge);
        }
    }
}
} // namespace globe
