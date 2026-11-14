// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The scale at the center of the map, drawn over the finished frame by both front ends: a bar of
// a round distance (1, 2 or 5 times a power of ten) with its label above it, bottom center, just
// over the refinement progress bar. White with a one-pixel black edge and no background, so it
// reads over land, sea and night. Drawn on every shown frame, dragging included: it depends only on
// the projected radius, which a drag does not change.
#pragma once
#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"
namespace globe {
// The most columns the bar may span, for a frame `width` wide.
int scale_bar_max_width(int width);
// The ticks inside the bar. The bar is `units` whole decade units long (1, 2 or 5 times 10^n km:
// 1 km, 200 m, 5 km), cut into steps of `steps_per_unit`: a medium tick at each whole unit inside
// the bar (one at 100 m on a 200 m bar), small ticks at the steps between, long ticks at the ends.
// A 1 is cut into tenths (100 m on 1 km), a 2 or 5 into halves of a unit.
struct ScaleTicks {
    int units = 1, steps_per_unit = 1;
    int steps() const { return units * steps_per_unit; }
};
ScaleTicks scale_ticks(double distance_km);
// The bottom rows of a `width` x `height` frame the scale may change (0 when the frame is too
// small to show it). The progress bar's own rows are included.
int scale_overlay_rows(int width, int height);
// Draws the scale for `camera`'s radius into `fb`; nothing when it does not fit or has no scale.
void draw_center_scale(Framebuffer& fb, const Camera& camera);
} // namespace globe
