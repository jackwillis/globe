// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/projection.hpp"

#include <cmath>

namespace globe {

namespace {

constexpr double kNearAntipodalSin = 1e-9;

bool all_finite(const CameraParams& p) {
    return std::isfinite(p.center.lat_deg) && std::isfinite(p.center.lon_deg) &&
           std::isfinite(p.heading_deg) && std::isfinite(p.radius_px);
}

} // namespace

std::optional<Camera> make_camera(const CameraParams& params) {
    if (!all_finite(params) || !(params.radius_px > 0.0) || params.width < 1 ||
        params.height < 1 || params.width > kMaxViewportDimension ||
        params.height > kMaxViewportDimension) {
        return std::nullopt;
    }
    double lat = params.center.lat_deg;
    if (lat > 90.0) {
        lat = 90.0;
    } else if (lat < -90.0) {
        lat = -90.0;
    }
    const Vec3 forward = to_unit_vector({lat, params.center.lon_deg});
    const double phi = lat * kDegToRad;
    // Local east and north from the partial derivatives of the unit vector; both stay unit length
    // and well defined at the poles, where a "project north onto the tangent plane" rule breaks.
    const double lambda = std::atan2(forward.x, forward.z);
    const Vec3 east{std::cos(lambda), 0.0, -std::sin(lambda)};
    const Vec3 north{-std::sin(phi) * std::sin(lambda), std::cos(phi),
                     -std::sin(phi) * std::cos(lambda)};
    const double h = params.heading_deg * kDegToRad;
    const double ch = std::cos(h);
    const double sh = std::sin(h);
    Camera cam{};
    cam.up = north * ch + east * sh;
    cam.right = east * ch - north * sh;
    cam.forward = forward;
    cam.radius_px = params.radius_px;
    cam.width = params.width;
    cam.height = params.height;
    cam.center_x = params.width / 2.0;
    cam.center_y = params.height / 2.0;
    return cam;
}

Vec3 to_camera_space(const Camera& camera, Vec3 world) {
    return {dot(camera.right, world), dot(camera.up, world), dot(camera.forward, world)};
}

ScreenPoint to_screen(const Camera& camera, Vec3 camera_point) {
    return {camera.center_x + camera.radius_px * camera_point.x,
            camera.center_y - camera.radius_px * camera_point.y};
}

std::optional<Arc> clip_arc_to_front(Vec3 a, Vec3 b) {
    if (length(cross(a, b)) < kNearAntipodalSin && dot(a, b) < 0.0) {
        return std::nullopt;
    }
    const bool a_front = is_front(a);
    const bool b_front = is_front(b);
    if (a_front && b_front) {
        return Arc{a, b};
    }
    if (!a_front && !b_front) {
        return std::nullopt;
    }
    // The two weights are both non-negative, so q lies on the shorter arc between a and b, and
    // its z component is |bz| * az + |az| * bz = 0 because az and bz have opposite signs.
    const Vec3 q = normalized(a * std::fabs(b.z) + b * std::fabs(a.z));
    return a_front ? Arc{a, q} : Arc{q, b};
}

std::size_t subdivide_arc(Vec3 a, Vec3 b, double max_step_rad, Vec3* out, std::size_t capacity) {
    if (!(max_step_rad > 0.0) || !std::isfinite(max_step_rad) || out == nullptr || capacity < 2) {
        return 0;
    }
    const double sin_omega = length(cross(a, b));
    const double omega = std::atan2(sin_omega, dot(a, b));
    if (sin_omega < kNearAntipodalSin && dot(a, b) < 0.0) {
        return 0;
    }
    // The tolerance keeps an arc that is an exact multiple of the step from gaining a spare step
    // through rounding in the division.
    const double steps_needed = std::ceil(omega / max_step_rad - 1e-9);
    if (!(steps_needed < static_cast<double>(capacity))) {
        return 0;
    }
    const std::size_t steps = steps_needed < 1.0 ? 1 : static_cast<std::size_t>(steps_needed);
    out[0] = a;
    for (std::size_t i = 1; i < steps; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(steps);
        if (sin_omega < 1e-12) {
            out[i] = normalized(a * (1.0 - t) + b * t);
        } else {
            out[i] = normalized(a * (std::sin((1.0 - t) * omega) / sin_omega) +
                                b * (std::sin(t * omega) / sin_omega));
        }
    }
    out[steps] = b;
    return steps + 1;
}

} // namespace globe
