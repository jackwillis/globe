// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#pragma once

#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"
#include <optional>

namespace globe {
// Unit screen-space direction toward geographic north at the view center.
// No bearing within the floating-point tolerance of either geographic pole.
std::optional<ScreenPoint> compass_north(const Camera& camera);
int compass_overlay_rows(int width, int height);
void draw_compass_rose(Framebuffer& fb, ScreenPoint north);
} // namespace globe
