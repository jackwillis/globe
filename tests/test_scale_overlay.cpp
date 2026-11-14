// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "check.hpp"
#include "globe/app.hpp"
#include "globe/measure.hpp"
#include "globe/overlays.hpp"
#include "globe/scale_overlay.hpp"
#include "globe/shown_frame.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
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
    // A label the front end drew replaces the built-in digits, in the same place and with the same
    // black edge; its text comes from the core.
    {
        const int width = 420, height = 315;
        const auto camera = camera_for(width, height, 140);
        char text[16] = {};
        const int length = scale_label_text(camera, width, text, sizeof text);
        CHECK(length > 0 && text[length] == 0 && text[length - 1] == 'm');
        CHECK(scale_label_text(camera_for(60, 40, 18), 60, text, sizeof text) == 0);
        // The text names the unit: km or m in metric, mi or ft in imperial.
        {
            const auto text_for = [&](double max_km, Units units, const char* want) {
                const int w = 120;
                const double r = kEarthMeanRadiusKm * scale_bar_max_width(w) / max_km;
                char out[16] = {};
                const int n = scale_label_text(camera_for(w, 90, r), w, out, sizeof out, units);
                CHECK(n > 0 && std::strcmp(out, want) == 0);
            };
            text_for(9000.0, Units::Metric, "5,000 km");
            text_for(9000.0, Units::Imperial, "5,000 mi");
            text_for(0.3, Units::Metric, "200 m");
            text_for(0.3, Units::Imperial, "500 ft");
            text_for(1.0, Units::Imperial, "2,000 ft");
            text_for(3.0, Units::Imperial, "1 mi");
        }
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
        for (const Units units : {Units::Metric, Units::Imperial})
            for (int step = 0; step < 90; ++step) {
                const double radius = 20.0 * std::pow(1.07, step);
                const auto camera = camera_for(width, height, radius);
                const auto bar = center_scale_bar(radius, scale_bar_max_width(width), units);
                if (!bar || bar->width_px < 2)
                    continue;
                fb->clear(kBackground);
                draw_center_scale(*fb, camera, nullptr, units);
                const auto ticks = scale_ticks(bar->value);
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
    // Units: metric bars are km (m under one km), imperial bars miles (feet under one mile); the
    // length is a 1, 2 or 5 in that unit and fits the width.
    {
        constexpr int kWidth = 100;
        const auto radius_for = [&](double max_km) { return kEarthMeanRadiusKm * kWidth / max_km; };
        const auto bar = [&](double max_km, Units units) {
            return center_scale_bar(radius_for(max_km), kWidth, units);
        };
        const auto check = [&](double max_km, Units units, double value, LengthUnit unit) {
            const auto b = bar(max_km, units);
            CHECK(b.has_value());
            if (!b)
                return;
            CHECK(b->unit == unit && std::abs(b->value - value) < 1e-9);
            CHECK(b->width_px >= 1 && b->width_px <= kWidth);
            const double km_per = unit == LengthUnit::Mile    ? 1.609344
                                  : unit == LengthUnit::Foot  ? 0.0003048
                                  : unit == LengthUnit::Meter ? 0.001
                                                              : 1.0;
            CHECK(std::abs(b->distance_km - value * km_per) < 1e-9 * (1.0 + b->distance_km));
            CHECK(b->distance_km <= max_km * (1.0 + 1e-9));
        };
        check(9000.0, Units::Metric, 5000.0, LengthUnit::Kilometer);
        check(9000.0, Units::Imperial, 5000.0, LengthUnit::Mile);
        check(10000.0, Units::Metric, 10000.0, LengthUnit::Kilometer);
        check(3.0, Units::Imperial, 1.0, LengthUnit::Mile);
        check(1.0, Units::Imperial, 2000.0, LengthUnit::Foot); // under a mile: feet
        check(1.0, Units::Metric, 1.0, LengthUnit::Kilometer);
        check(0.3, Units::Metric, 200.0, LengthUnit::Meter);
        check(0.3, Units::Imperial, 500.0, LengthUnit::Foot);
        // The default is metric, as before.
        CHECK(center_scale_bar(radius_for(10000.0), kWidth)->unit == LengthUnit::Kilometer);
        // At one radius the two bars differ (10,000 km fills all 120 columns, 5,000 mi is 96), and
        // each is what is drawn, whether drawn directly or by ShownFrame.
        const int width = 420, height = 315, bar_y = height - kProgressBarRows - 6;
        const double radius = kEarthMeanRadiusKm * scale_bar_max_width(width) / 10000.0;
        const auto camera = camera_for(width, height, radius);
        const auto metric = center_scale_bar(radius, scale_bar_max_width(width), Units::Metric);
        const auto imperial = center_scale_bar(radius, scale_bar_max_width(width), Units::Imperial);
        CHECK(metric && imperial && metric->width_px != imperial->width_px);
        const auto span_drawn = [&](const Framebuffer& fb) {
            int left = -1, right = -1;
            for (int x = 0; x < width; ++x)
                if (fb.pixel(x, bar_y - 3) == kProgressFill) {
                    left = left < 0 ? x : left;
                    right = x;
                }
            return right - left + 1;
        };
        auto fb = Framebuffer::create(width, height);
        for (const Units units : {Units::Metric, Units::Imperial}) {
            fb->clear(kBackground);
            draw_center_scale(*fb, camera, nullptr, units);
            CHECK(span_drawn(*fb) == (units == Units::Metric ? metric : imperial)->width_px);
            fb->clear(kBackground);
            Overlays overlays;
            ShownFrame shown;
            shown.set_scale_units(units);
            const auto& frame = shown.begin(overlays, *fb, camera, 0, true, -1.0);
            CHECK(span_drawn(frame) == (units == Units::Metric ? metric : imperial)->width_px);
            shown.end();
        }
        // Unset, ShownFrame draws metric.
        fb->clear(kBackground);
        Overlays overlays;
        ShownFrame plain;
        CHECK(span_drawn(plain.begin(overlays, *fb, camera, 0, true, -1.0)) == metric->width_px);
        plain.end();
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
