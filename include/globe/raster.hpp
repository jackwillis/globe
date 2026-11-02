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

// Fills a triangle, interpolating a scalar linearly across it (Gouraud shading of a palette
// index). Each vertex carries a value; a covered pixel is painted `palette[floor(v)]` where v is
// the plane through the three vertex values, evaluated at the pixel center. Values are clamped
// to [0, 256) first, so `palette` must hold 256 entries.
//
// Pixel coverage uses the same center-sampling rule as the other primitives (a pixel is covered
// when its center is inside; a center exactly on an edge belongs to the triangle below/right of
// it). Edge positions are computed from the two endpoints in a canonical order, so two triangles
// sharing an edge leave no gap and no overlap, in any vertex order and winding. Per-pixel work is
// one add and one table lookup. Non-finite values or coordinates, coordinates beyond +-1e6, and
// triangles with (near) zero area draw nothing: clip before calling.
void fill_triangle_shaded(Framebuffer& fb, double x0, double y0, double v0, double x1, double y1,
                          double v1, double x2, double y2, double v2, const Rgb565* palette);

} // namespace globe
