// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/overlays.hpp"
#include "globe/daynight.hpp"
#include "globe/highlight.hpp"
#include "globe/measure.hpp"
#include "globe/trackball.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace globe {
void draw_progress_bar(Framebuffer& fb, double progress) {
    const int rows = std::min(kProgressBarRows, fb.height());
    const int filled = static_cast<int>(std::clamp(progress, 0.0, 1.0) * fb.width());
    if (filled <= 0)
        return;
    for (int r = 0; r < rows; ++r) {
        const int y = fb.height() - rows + r;
        fb.fill_span(y, 0, filled - 1, r == 0 ? kProgressEdge : kProgressFill);
    }
}
void Overlays::toggle_measure() {
    measuring_ = !measuring_;
    if (!measuring_) {
        measure_a_.reset();
        measure_b_.reset();
    }
}
namespace {
// A tiny formatter: the floppy build avoids linking printf with floating point (about 150 KB).
struct Text {
    char* out;
    std::size_t size, at = 0;
    Text(char* buffer, std::size_t n) : out(buffer), size(n) {
        if (size > 0)
            out[0] = '\0';
    }
    void add(const char* s) {
        while (*s != '\0' && at + 1 < size)
            out[at++] = *s++;
        if (size > 0)
            out[at] = '\0';
    }
    void add(unsigned long v) {
        char digits[24];
        int n = 0;
        do {
            digits[n++] = static_cast<char>('0' + v % 10);
            v /= 10;
        } while (v != 0 && n < 23);
        while (n > 0 && at + 1 < size)
            out[at++] = digits[--n];
        if (size > 0)
            out[at] = '\0';
    }
    // value with `decimals` digits after the point, rounded, never negative.
    void add_fixed(double value, int decimals) {
        unsigned long scale = 1;
        for (int i = 0; i < decimals; ++i)
            scale *= 10;
        const auto scaled = static_cast<unsigned long>(
            std::floor(std::fabs(value) * static_cast<double>(scale) + 0.5));
        add(scaled / scale);
        if (decimals > 0) {
            add(".");
            const unsigned long fraction = scaled % scale;
            for (unsigned long d = scale / 10; d > 1 && fraction < d; d /= 10)
                add("0");
            add(fraction);
        }
    }
};
void place_text(char* out, std::size_t n, const char* label, LatLon q) {
    Text t(out, n);
    t.add(label);
    t.add(" ");
    t.add_fixed(q.lat_deg, 2);
    t.add(q.lat_deg >= 0 ? " N, " : " S, ");
    t.add_fixed(q.lon_deg, 2);
    t.add(q.lon_deg >= 0 ? " E" : " W");
}
} // namespace
void Overlays::measure_click(LatLon p) {
    if (!measure_a_.has_value() || measure_b_.has_value()) {
        measure_a_ = p;
        measure_b_.reset();
        return;
    }
    measure_b_ = p;
    const LatLon a = *measure_a_;
    const double km = great_circle_km(a, p);
    place_text(text_[0], sizeof text_[0], tr(Str::From), a);
    place_text(text_[1], sizeof text_[1], tr(Str::To), p);
    Text distance(text_[2], sizeof text_[2]);
    distance.add(tr(Str::GreatCircle));
    distance.add(" ");
    if (prefs_.units == Units::Imperial) {
        distance.add_fixed(km * 0.621371, 0);
        distance.add(" mi");
    } else {
        distance.add_fixed(km, 0);
        distance.add(" km");
    }
    Text bearing(text_[3], sizeof text_[3]);
    bearing.add(tr(Str::InitialBearing));
    bearing.add(" ");
    bearing.add_fixed(initial_bearing_deg(a, p), 0);
    bearing.add(" ");
    bearing.add(tr(Str::Degrees));
    card_ = InfoCard{};
    card_.title = tr(Str::Distance);
    for (unsigned i = 0; i < 4; ++i)
        card_.lines[i] = text_[i];
    card_.line_count = 4;
    card_ready_ = true;
}
bool Overlays::click(const Camera& camera, ScreenPoint at) {
    const double x = (at.x - camera.center_x) / camera.radius_px;
    const double y = (at.y - camera.center_y) / camera.radius_px;
    if (!(x * x + y * y <= 1))
        return false;
    const auto v = unproject_to_sphere(camera, at);
    const auto world = camera.right * v.x + camera.up * v.y + camera.forward * v.z;
    if (measuring_) {
        measure_click(from_unit_vector(world));
        return true;
    }
    if (country_scene_ == nullptr || countries_ == nullptr)
        return false;
    const auto city = place_scene_ != nullptr ? place_scene_->pick(camera, at) : std::nullopt;
    const auto id = country_scene_->pick(from_unit_vector(world));
    const auto before = focused_;
    focused_ = city ? std::nullopt : id; // a place card, or the sea, clears the focus
    if (city && places_ != nullptr) {
        card_ = place_card(*places_, *city);
        card_ready_ = true;
    } else if (id) {
        card_ = country_card(*countries_, *id);
        card_ready_ = true;
    } else if (before) {
        card_ = nothing_selected_card(); // the click cleared a selection
        card_ready_ = true;
    }
    return focused_ != before;
}
bool Overlays::clickable_at(const Camera& camera, ScreenPoint at) const {
    const double x = (at.x - camera.center_x) / camera.radius_px;
    const double y = (at.y - camera.center_y) / camera.radius_px;
    if (!(x * x + y * y <= 1))
        return false;
    if (measuring_)
        return true;
    return place_scene_ != nullptr && place_scene_->pick(camera, at).has_value();
}
std::optional<InfoCard> Overlays::take_card() {
    if (!card_ready_)
        return std::nullopt;
    card_ready_ = false;
    return card_;
}
Framebuffer* Overlays::compose(const Framebuffer& frame, const Camera& camera,
                               std::int64_t unix_seconds, bool quick) {
    const auto focus = focused_;
    const auto a = measure_a_;
    const auto b = measure_b_;
    if (!night_ && !focus.has_value() && !a.has_value())
        return nullptr;
    if (copy_.has_value() && (copy_->width() != frame.width() || copy_->height() != frame.height()))
        copy_.reset();
    if (!copy_.has_value())
        copy_ = Framebuffer::create(frame.width(), frame.height());
    if (!copy_.has_value())
        return nullptr;
    Framebuffer& out = *copy_;
    std::memcpy(out.data(), frame.data(), frame.byte_size());
    if (night_) {
        // The sun moves a quarter of a degree a minute: recompute at most every 30 s.
        if (sun_time_ == 0 || unix_seconds - sun_time_ >= 30 || unix_seconds < sun_time_) {
            sun_time_ = unix_seconds;
            sun_ = to_unit_vector(subsolar_point(unix_seconds));
        }
        if (quick)
            draw_terminator(out, camera, sun_, kTerminatorColor);
        else
            shade_night(out, camera, sun_);
    }
    if (a.has_value() && b.has_value())
        draw_measure(out, camera, *a, *b, kMeasureColor);
    else if (a.has_value())
        draw_measure_point(out, camera, *a, kMeasureColor);
    if (focus.has_value() && countries_ != nullptr) {
        if (!quick)
            fill_country_dither(out, camera, *countries_, *focus, kCountryHighlight);
        draw_country_outline(out, camera, *countries_, *focus, kCountryHighlight);
    }
    return &out;
}
} // namespace globe
