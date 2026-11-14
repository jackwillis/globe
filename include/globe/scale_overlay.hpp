// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// The scale at the center of the map, drawn over the finished frame by both front ends: a bar of
// a round distance (1, 2 or 5 times a power of ten) with its label above it, bottom center, just
// over the refinement progress bar. White with a one-pixel black edge and no background, so it
// reads over land, sea and night. Drawn on every shown frame, dragging included: it depends only on
// the projected radius, which a drag does not change.
#pragma once
#include "globe/framebuffer.hpp"
#include "globe/prefs.hpp"
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
// The label text for `camera` ("5,000 km", "500 m"; "5,000 mi", "500 ft" in imperial units) into
// `out`; its length, or 0 when no scale fits a frame `width` wide. `capacity` includes the
// terminator.
int scale_label_text(const Camera& camera, int width, char* out, int capacity,
                     Units units = Units::Metric);
// The most columns the bar may span, for a frame `width` wide.
int scale_bar_max_width(int width);
// The ticks inside the bar. The bar is `units` whole decade units long (1, 2 or 5 times 10^n km:
// 1 km, 200 m, 5 km, or the same numbers of miles or feet), cut into steps of `steps_per_unit`,
// with long ticks at the ends. A bar of 1 is cut into tenths (100 m on 1 km) with its halfway tick,
// the 5, medium; a bar of 2 or 5 has a medium tick at each whole unit inside it (one at 100 m on a
// 200 m bar), with small ticks at the half-steps between. All other ticks are small.
struct ScaleTicks {
    int units = 1, steps_per_unit = 1;
    int steps() const { return units * steps_per_unit; }
    // The tick at step `i` (1 to steps() - 1) is a medium one.
    bool medium(int i) const {
        return units == 1 ? i == steps_per_unit / 2 : i % steps_per_unit == 0;
    }
};
ScaleTicks scale_ticks(double distance_km);
// The bottom rows of a `width` x `height` frame the scale may change (0 when the frame is too
// small to show it). The progress bar's own rows are included.
int scale_overlay_rows(int width, int height);
// Draws the scale for `camera`'s radius into `fb`; nothing when it does not fit or has no scale.
// A `label` replaces the built-in digits. `units` is the preference: the bar is a round distance in
// kilometres and metres, or in miles and feet.
void draw_center_scale(Framebuffer& fb, const Camera& camera, const LabelBitmap* label = nullptr,
                       Units units = Units::Metric);
} // namespace globe
