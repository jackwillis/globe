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
// The bottom rows of a `width` x `height` frame the scale may change (0 when the frame is too
// small to show it). The progress bar's own rows are included.
int scale_overlay_rows(int width, int height);
// Draws the scale for `camera`'s radius into `fb`; nothing when it does not fit or has no scale.
void draw_center_scale(Framebuffer& fb, const Camera& camera);
} // namespace globe
