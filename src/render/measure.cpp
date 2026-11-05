// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/measure.hpp"
#include "globe/raster.hpp"
#include <algorithm>
#include <cmath>
namespace globe {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg = kPi / 180.0;
constexpr int kSegments = 96;
Vec3 project(const Camera& camera, Vec3 world) {
    const auto v = to_camera_space(camera, world);
    const auto p = to_screen(camera, v);
    return {p.x, p.y, v.z};
}
} // namespace
double great_circle_km(LatLon a, LatLon b) {
    const double p1 = a.lat_deg * kDeg, p2 = b.lat_deg * kDeg;
    const double dp = p2 - p1, dl = (b.lon_deg - a.lon_deg) * kDeg;
    const double s1 = std::sin(dp / 2), s2 = std::sin(dl / 2);
    const double h = s1 * s1 + std::cos(p1) * std::cos(p2) * s2 * s2;
    return 2.0 * kEarthMeanRadiusKm * std::asin(std::min(1.0, std::sqrt(h)));
}
double initial_bearing_deg(LatLon a, LatLon b) {
    const double p1 = a.lat_deg * kDeg, p2 = b.lat_deg * kDeg, dl = (b.lon_deg - a.lon_deg) * kDeg;
    const double y = std::sin(dl) * std::cos(p2);
    const double x = std::cos(p1) * std::sin(p2) - std::sin(p1) * std::cos(p2) * std::cos(dl);
    if (x == 0.0 && y == 0.0)
        return 0.0;
    const double degrees = std::atan2(y, x) / kDeg;
    return degrees < 0 ? degrees + 360.0 : degrees;
}
void draw_measure_point(Framebuffer& fb, const Camera& camera, LatLon a, Rgb565 color) {
    const auto p = project(camera, to_unit_vector(a));
    if (p.z < 0)
        return;
    fill_disc(fb, p.x, p.y, 4.0, rgb565(255, 255, 255));
    fill_disc(fb, p.x, p.y, 2.5, color);
}
std::size_t draw_measure(Framebuffer& fb, const Camera& camera, LatLon a, LatLon b, Rgb565 color) {
    const Vec3 ua = to_unit_vector(a), ub = to_unit_vector(b);
    const double omega = std::acos(std::max(-1.0, std::min(1.0, dot(ua, ub))));
    std::size_t drawn = 0;
    // Coincident or antipodal points have no unique arc.
    if (omega > 1e-9 && std::sin(omega) > 1e-9) {
        const double sine = std::sin(omega);
        Vec3 previous = project(camera, ua);
        for (int i = 1; i <= kSegments; ++i) {
            const double t = static_cast<double>(i) / kSegments;
            const Vec3 world =
                ua * (std::sin((1.0 - t) * omega) / sine) + ub * (std::sin(t * omega) / sine);
            const Vec3 current = project(camera, world);
            if (previous.z >= 0 && current.z >= 0) {
                for (const double d : {0.0, 1.0}) {
                    draw_line(fb, previous.x + d, previous.y, current.x + d, current.y, color);
                    draw_line(fb, previous.x, previous.y + d, current.x, current.y + d, color);
                }
                ++drawn;
            }
            previous = current;
        }
    }
    draw_measure_point(fb, camera, a, color);
    draw_measure_point(fb, camera, b, color);
    return drawn;
}
} // namespace globe
