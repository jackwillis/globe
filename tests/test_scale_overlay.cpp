// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/app.hpp"
#include "globe/overlays.hpp"
#include "globe/scale_overlay.hpp"
#include "globe/shown_frame.hpp"
#include <algorithm>
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
        // No background: the box around it is mostly the map showing through.
        const int box = (d.right - d.left + 1) * (d.bottom - d.top + 1);
        CHECK(d.changed * 2 < box);
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
    return 0;
}
