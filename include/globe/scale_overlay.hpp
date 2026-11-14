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
#include <cstdint>
namespace globe {
// A label the front end drew with a system font: one bit a pixel, rows packed left to right with
// the high bit first, `stride` bytes a row, a set bit being ink. The scale draws it, with its black
// edge, instead of the built-in digits; the front end keeps the bits alive while it is in use.
struct LabelBitmap {
    static constexpr int kMaxHeight = 16;
    int width = 0, height = 0, stride = 0;
    const std::uint8_t* bits = nullptr;
};
// The label text for `camera` ("5,000 km", "500 m") into `out`; its length, or 0 when no scale fits
// a frame `width` wide. `capacity` includes the terminator.
int scale_label_text(const Camera& camera, int width, char* out, int capacity);
// The most columns the bar may span, for a frame `width` wide.
int scale_bar_max_width(int width);
// How many equal parts the bar is divided into by small ticks, so each part is a round number
// (a 1 km bar by 100 m, 2 km by 500 m, 5 km by 1 km): 10 for a 1, 4 for a 2, 5 for a 5.
int scale_divisions(double distance_km);
// The bottom rows of a `width` x `height` frame the scale may change (0 when the frame is too
// small to show it). The progress bar's own rows are included.
int scale_overlay_rows(int width, int height);
// Draws the scale for `camera`'s radius into `fb`; nothing when it does not fit or has no scale.
// A `label` replaces the built-in digits.
void draw_center_scale(Framebuffer& fb, const Camera& camera, const LabelBitmap* label = nullptr);
} // namespace globe
