// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/app.hpp"
#include "globe/overlays.hpp"
#include "globe/scale_overlay.hpp"
#include "globe/shown_frame.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>
using namespace globe;
namespace {
constexpr Rgb565 kBackground = 0x1234;
Camera camera_for(int width, int height, double radius) {
    Camera camera = initial_model(width, height).camera;
    camera.radius_px = radius;
    return camera;
}
struct Drawn {
    int changed = 0, top = 1 << 30, bottom = -1, left = 1 << 30, right = -1, outside = 0;
    bool only_edge_and_fill = true;
};
Drawn measure(const Framebuffer& fb) {
    Drawn d;
    const int rows = scale_overlay_rows(fb.width(), fb.height());
    for (int y = 0; y < fb.height(); ++y)
        for (int x = 0; x < fb.width(); ++x) {
            const auto v = fb.pixel(x, y);
            if (v == kBackground)
                continue;
            ++d.changed;
            d.only_edge_and_fill &= v == kProgressEdge || v == kProgressFill;
            d.top = std::min(d.top, y);
            d.bottom = std::max(d.bottom, y);
            d.left = std::min(d.left, x);
            d.right = std::max(d.right, x);
            d.outside += y < fb.height() - rows;
        }
    return d;
}
} // namespace
int main() {
    for (const int width : {160, 420, 640}) {
        const int height = width * 3 / 4;
        auto fb = Framebuffer::create(width, height);
        CHECK(fb);
        if (!fb)
            return 1;
        fb->clear(kBackground);
        const auto camera = camera_for(width, height, height * 0.45);
        draw_center_scale(*fb, camera);
        const auto d = measure(*fb);
        // Drawn, white on a black edge only, inside the rows it reserves, above the progress bar,
        // and centered.
        CHECK(d.changed > 100 && d.only_edge_and_fill && d.outside == 0);
        CHECK(d.bottom < height - kProgressBarRows);
        CHECK(std::abs((d.left + d.right) / 2 - width / 2) <= 1);
        CHECK(d.right - d.left + 1 <= scale_bar_max_width(width) + 2);
        // No background: the gap between label and bar, and the rows under the bar between its end
        // ticks (past the medium ticks' edge), are the map showing through.
        const int bar_y = height - kProgressBarRows - 6;
        for (int x = d.left; x <= d.right; ++x)
            CHECK(fb->pixel(x, bar_y - 5) == kBackground);
        const auto bar = center_scale_bar(camera.radius_px, scale_bar_max_width(width));
        CHECK(bar.has_value());
        const int bar_left = (width - bar->width_px) / 2;
        for (int x = bar_left + 3; x < bar_left + bar->width_px - 3; ++x)
            CHECK(fb->pixel(x, bar_y + 4) == kBackground && fb->pixel(x, bar_y + 5) == kBackground);
        // The label is above the bar; a zoom changes what is drawn.
        auto other = Framebuffer::create(width, height);
        other->clear(kBackground);
        draw_center_scale(*other, camera_for(width, height, height * 0.9));
        bool differs = false;
        for (int y = 0; y < height && !differs; ++y)
            for (int x = 0; x < width && !differs; ++x)
                differs = fb->pixel(x, y) != other->pixel(x, y);
        CHECK(differs);
    }
    // From a few metres to thousands of kilometres (with its comma) the label stays centered and
    // black and white, and the end ticks are seven rows tall (three above and below the bar).
    {
        const int width = 420, height = 315;
        auto fb = Framebuffer::create(width, height);
        for (const double radius : {2.0, 10.0, 60.0, 140.0, 300.0, 4000.0}) {
            fb->clear(kBackground);
            draw_center_scale(*fb, camera_for(width, height, radius));
            const auto d = measure(*fb);
            if (d.changed == 0)
                continue; // a radius with no bar that fits
            CHECK(d.only_edge_and_fill && d.outside == 0);
            CHECK(std::abs((d.left + d.right) / 2 - width / 2) <= 2);
            const int bar_y = height - kProgressBarRows - 6;
            int x0 = 0;
            while (x0 < width && fb->pixel(x0, bar_y) != kProgressFill)
                ++x0;
            CHECK(x0 < width);
            CHECK(fb->pixel(x0, bar_y - 3) == kProgressFill &&
                  fb->pixel(x0, bar_y + 3) == kProgressFill);
            CHECK(fb->pixel(x0, bar_y - 4) == kProgressEdge &&
                  fb->pixel(x0, bar_y + 4) == kProgressEdge);
        }
    }
    // Ticks: the bar is whole decade units (a 1, 2 or 5), with a medium tick at each whole unit
    // inside it and small ticks between.
    {
        const auto t1 = scale_ticks(1.0), t2 = scale_ticks(200.0), t5 = scale_ticks(0.005);
        CHECK(t1.units == 1 && t1.steps_per_unit == 10 && t1.steps() == 10);
        // On a 1 (100, 1,000) the halfway tick is the medium one and no other.
        for (int i = 1; i < 10; ++i)
            CHECK(t1.medium(i) == (i == 5));
        CHECK(!t2.medium(1) && t2.medium(2) && !t2.medium(3));
        CHECK(t2.units == 2 && t2.steps_per_unit == 2 && t2.steps() == 4);
        CHECK(t5.units == 5 && t5.steps_per_unit == 2 && t5.steps() == 10);
        CHECK(scale_ticks(100.0).units == 1 && scale_ticks(5000.0).units == 5);
        CHECK(scale_ticks(0.0).steps() == 1 && scale_ticks(-3.0).steps() == 1);
    }
    {
        const int width = 420, height = 315, bar_y = height - kProgressBarRows - 6;
        auto fb = Framebuffer::create(width, height);
        int seen_small = 0, seen_medium = 0;
        // True when the column has white exactly `reach` rows either side of the line, in black.
        const auto column = [&](int x, int reach) {
            return fb->pixel(x, bar_y - reach) == kProgressFill &&
                   fb->pixel(x, bar_y + reach) == kProgressFill &&
                   fb->pixel(x, bar_y - reach - 1) == kProgressEdge &&
                   fb->pixel(x, bar_y + reach + 1) == kProgressEdge;
        };
        for (int step = 0; step < 90; ++step) {
            const double radius = 20.0 * std::pow(1.07, step);
            const auto camera = camera_for(width, height, radius);
            const auto bar = center_scale_bar(radius, scale_bar_max_width(width));
            if (!bar || bar->width_px < 2)
                continue;
            fb->clear(kBackground);
            draw_center_scale(*fb, camera);
            const auto ticks = scale_ticks(bar->distance_km);
            const int steps = ticks.steps(), span = bar->width_px - 1;
            const int left = (width - bar->width_px) / 2;
            CHECK(column(left, 3) && column(left + span, 3)); // long at the ends
            const bool small = span / steps >= 4;
            const bool medium = span / (ticks.units > 1 ? ticks.units : 2) >= 4;
            int expected = 0;
            for (int i = 1; i < steps; ++i) {
                // Medium: the 5 on a 1 (500 of 1,000), each whole unit on a 2 or 5.
                const bool whole = ticks.units == 1 ? i == 5 : i % 2 == 0;
                CHECK(whole == ticks.medium(i));
                const int x = left + (i * span + steps / 2) / steps;
                if (whole ? medium : small) {
                    CHECK(column(x, whole ? 2 : 1));
                    ++expected;
                    (whole ? seen_medium : seen_small) += 1;
                }
            }
            // Nothing else rises above the line between the ends: the other columns are bare.
            int marked = 0;
            for (int x = left + 1; x < left + span; ++x)
                marked += fb->pixel(x, bar_y - 1) == kProgressFill;
            CHECK(marked == expected);
        }
        CHECK(seen_small > 5 && seen_medium > 2);
    }
    // Too small to fit: nothing is drawn and no rows are claimed.
    {
        auto tiny = Framebuffer::create(60, 40);
        tiny->clear(kBackground);
        draw_center_scale(*tiny, camera_for(60, 40, 18));
        CHECK(measure(*tiny).changed == 0 && scale_overlay_rows(60, 40) == 0);
        CHECK(scale_overlay_rows(640, 20) == 0);
    }
    // Every shown frame has it, a moving (quick) one and one with the progress bar included, and
    // the rendered frame is put back as it was.
    {
        const int width = 420, height = 315;
        auto fb = Framebuffer::create(width, height);
        Overlays overlays;
        const auto camera = camera_for(width, height, 140);
        for (const bool settled : {true, false}) {
            for (const double progress : {-1.0, 0.5}) {
                fb->clear(kBackground);
                ShownFrame shown;
                const auto& frame = shown.begin(overlays, *fb, camera, 0, settled, progress);
                const auto d = measure(frame);
                CHECK(d.changed > 100 && d.only_edge_and_fill);
                CHECK(d.bottom == height - 1 || progress < 0.0); // the bar reaches the bottom row
                CHECK(shown.bar() == (progress >= 0.0));
                shown.end();
                CHECK(measure(*fb).changed == 0);
            }
        }
        // With an overlay copy in play it is drawn on the copy; the frame itself stays clean.
        overlays.toggle_night();
        fb->clear(kBackground);
        ShownFrame shown;
        const auto& frame = shown.begin(overlays, *fb, camera, 1793793600, true, -1.0);
        CHECK(&frame != fb.operator->());
        shown.end();
        CHECK(measure(*fb).changed == 0);
    }
    return TEST_MAIN_RESULT();
}
