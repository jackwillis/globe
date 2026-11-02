// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// 2D primitives on the RGB565 Framebuffer. Coordinates are continuous screen space (the pixel
// containing a point is floor(x), floor(y)), matching globe/projection.hpp.
#pragma once

#include "globe/framebuffer.hpp"

namespace globe {

// Integer midpoint line between the pixels containing the endpoints, both included.
//
// Clipping is exact: the pixels drawn are precisely those of the unclipped line that fall inside
// the buffer, and the work is bounded by the buffer size, not the line length. Drawing a line
// from a to b gives the same pixels as from b to a. Non-finite coordinates draw nothing;
// coordinates beyond +-1e6 are first clipped to that box.
void draw_line(Framebuffer& fb, double x0, double y0, double x1, double y1, Rgb565 color);

// Fills the pixels whose centers lie within `radius` of (cx, cy). Safe for any finite radius;
// only rows inside the buffer are touched. Non-finite or non-positive radius draws nothing.
void fill_disc(Framebuffer& fb, double cx, double cy, double radius, Rgb565 color);

} // namespace globe
