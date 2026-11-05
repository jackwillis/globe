// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/overlays.hpp"
#include "globe/daynight.hpp"
#include "globe/highlight.hpp"
#include "globe/measure.hpp"
#include "globe/trackball.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
namespace globe {
void Overlays::toggle_measure() {
    measuring_ = !measuring_;
    if (!measuring_) {
        measure_a_.reset();
        measure_b_.reset();
    }
}
void Overlays::measure_click(LatLon p) {
    if (!measure_a_.has_value() || measure_b_.has_value()) {
        measure_a_ = p;
        measure_b_.reset();
        return;
    }
    measure_b_ = p;
    const LatLon a = *measure_a_;
    const double km = great_circle_km(a, p);
    const auto place = [](char* out, std::size_t n, const char* label, LatLon q) {
        std::snprintf(out, n, "%s %.2f %c, %.2f %c", label, std::fabs(q.lat_deg),
                      q.lat_deg >= 0 ? 'N' : 'S', std::fabs(q.lon_deg), q.lon_deg >= 0 ? 'E' : 'W');
    };
    place(text_[0], sizeof text_[0], "From", a);
    place(text_[1], sizeof text_[1], "To", p);
    std::snprintf(text_[2], sizeof text_[2], "Great circle: %.0f km (%.0f mi)", km, km * 0.621371);
    std::snprintf(text_[3], sizeof text_[3], "Initial bearing: %.0f degrees",
                  initial_bearing_deg(a, p));
    card_ = InfoCard{};
    card_.title = "Distance";
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
    }
    return focused_ != before;
}
std::optional<InfoCard> Overlays::take_card() {
    if (!card_ready_)
        return std::nullopt;
    card_ready_ = false;
    return card_;
}
Framebuffer* Overlays::compose(const Framebuffer& frame, const Camera& camera,
                               std::int64_t unix_seconds) {
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
        shade_night(out, camera, sun_);
    }
    if (a.has_value() && b.has_value())
        draw_measure(out, camera, *a, *b, kMeasureColor);
    else if (a.has_value())
        draw_measure_point(out, camera, *a, kMeasureColor);
    if (focus.has_value() && countries_ != nullptr) {
        fill_country_dither(out, camera, *countries_, *focus, kCountryHighlight);
        draw_country_outline(out, camera, *countries_, *focus, kCountryHighlight);
    }
    return &out;
}
} // namespace globe
