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
    // From a few metres to thousands of kilometres (with its comma) the label stays centered and
    // black and white, and the end ticks are three rows tall (one row above and below the bar).
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
            CHECK(fb->pixel(x0, bar_y - 1) == kProgressFill &&
                  fb->pixel(x0, bar_y + 1) == kProgressFill);
            CHECK(fb->pixel(x0, bar_y - 2) == kProgressEdge &&
                  fb->pixel(x0, bar_y + 2) == kProgressEdge);
        }
    }
    // A label the front end drew replaces the built-in digits, in the same place and with the same
    // black edge; its text comes from the core.
    {
        const int width = 420, height = 315;
        const auto camera = camera_for(width, height, 140);
        char text[16] = {};
        const int length = scale_label_text(camera, width, text, sizeof text);
        CHECK(length > 0 && text[length] == 0 && text[length - 1] == 'm');
        CHECK(scale_label_text(camera_for(60, 40, 18), 60, text, sizeof text) == 0);
        // A solid 30x12 block of ink stands in for the glyphs.
        std::vector<std::uint8_t> bits(48, 0xff); // 12 rows of 4 bytes
        LabelBitmap label{30, 12, 4, bits.data()};
        auto plain = Framebuffer::create(width, height);
        auto native = Framebuffer::create(width, height);
        plain->clear(kBackground);
        native->clear(kBackground);
        draw_center_scale(*plain, camera);
        draw_center_scale(*native, camera, &label);
        const auto d = measure(*native);
        CHECK(d.changed > 0 && d.only_edge_and_fill && d.outside == 0);
        const int bar_y = height - kProgressBarRows - 6;
        const int left = (width - 30) / 2, top = bar_y - 6 - 12;
        CHECK(native->pixel(left, top) == kProgressFill &&
              native->pixel(left + 29, top + 11) == kProgressFill);
        CHECK(native->pixel(left - 1, top) == kProgressEdge &&
              native->pixel(left, top - 1) == kProgressEdge);
        CHECK(native->pixel(left + 30, top + 11) == kProgressEdge);
        // The bar underneath is the same either way.
        bool same = true;
        for (int x = 0; x < width; ++x)
            same &= plain->pixel(x, bar_y) == native->pixel(x, bar_y);
        CHECK(same);
        // Bitmaps that cannot be drawn are ignored in favour of the built-in digits.
        LabelBitmap tall{30, 40, 4, bits.data()}, empty{0, 0, 0, nullptr};
        for (const LabelBitmap* bad : {&tall, &empty}) {
            auto again = Framebuffer::create(width, height);
            again->clear(kBackground);
            draw_center_scale(*again, camera, bad);
            bool equal = true;
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                    equal &= again->pixel(x, y) == plain->pixel(x, y);
            CHECK(equal);
        }
        // ShownFrame passes it along and the rendered frame is put back.
        auto fb = Framebuffer::create(width, height);
        fb->clear(kBackground);
        Overlays overlays;
        ShownFrame shown;
        shown.set_scale_label(&label);
        const auto& frame = shown.begin(overlays, *fb, camera, 0, true, -1.0);
        CHECK(frame.pixel(left, top) == kProgressFill);
        shown.end();
        CHECK(measure(*fb).changed == 0);
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
