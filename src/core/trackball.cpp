// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/trackball.hpp"

#include <cmath>

namespace globe {

namespace {

bool finite(ScreenPoint p) {
    return std::isfinite(p.x) && std::isfinite(p.y);
}

// Rodrigues rotation of v about unit axis k, given cos and sin of the angle.
Vec3 rotate(Vec3 v, Vec3 k, double c, double s) {
    return v * c + cross(k, v) * s + k * (dot(k, v) * (1.0 - c));
}

} // namespace

Vec3 unproject_to_sphere(const Camera& camera, ScreenPoint p) {
    if (!finite(p) || !(camera.radius_px > 0.0)) {
        return {0.0, 0.0, 1.0};
    }
    const double x = (p.x - camera.center_x) / camera.radius_px;
    const double y = (camera.center_y - p.y) / camera.radius_px;
    const double d2 = x * x + y * y;
    if (d2 >= 1.0) {
        const double d = std::sqrt(d2);
        return {x / d, y / d, 0.0};
    }
    return {x, y, std::sqrt(1.0 - d2)};
}

Camera drag_rotate(const Camera& camera, ScreenPoint from, ScreenPoint to) {
    if (!finite(from) || !finite(to)) {
        return camera;
    }
    const Vec3 p0 = unproject_to_sphere(camera, from);
    const Vec3 p1 = unproject_to_sphere(camera, to);
    const Vec3 axis_raw = cross(p0, p1);
    const double s = length(axis_raw);
    const double c = dot(p0, p1);
    Vec3 axis{0.0, 0.0, 1.0};
    if (s < 1e-15) {
        if (c > 0.0) {
            return camera; // same point: nothing to do
        }
        // Opposite limb points: the shortest rotation is ambiguous, so turn the globe half a
        // revolution in the screen plane. That still carries p0 onto p1.
    } else {
        axis = axis_raw * (1.0 / s);
    }
    // Camera-space rotation Q with Q * p0 = p1. The new camera coordinates of every world point
    // are Q times the old ones, so the new basis rows are the rows of Q applied to the old rows.
    const Vec3 col0 = rotate({1.0, 0.0, 0.0}, axis, c, s);
    const Vec3 col1 = rotate({0.0, 1.0, 0.0}, axis, c, s);
    const Vec3 col2 = rotate({0.0, 0.0, 1.0}, axis, c, s);
    Camera moved = camera;
    moved.right = camera.right * col0.x + camera.up * col1.x + camera.forward * col2.x;
    moved.up = camera.right * col0.y + camera.up * col1.y + camera.forward * col2.y;
    moved.forward = camera.right * col0.z + camera.up * col1.z + camera.forward * col2.z;
    return orthonormalized(moved);
}

Camera orthonormalized(const Camera& camera) {
    const double flen = length(camera.forward);
    if (!(flen > 0.0) || !std::isfinite(flen)) {
        return camera;
    }
    const Vec3 f = camera.forward * (1.0 / flen);
    const Vec3 r_raw = camera.right - f * dot(camera.right, f);
    const double rlen = length(r_raw);
    if (!(rlen > 1e-12) || !std::isfinite(rlen)) {
        return camera;
    }
    Camera out = camera;
    out.forward = f;
    out.right = r_raw * (1.0 / rlen);
    out.up = cross(out.forward, out.right);
    return out;
}

Camera zoomed(const Camera& camera, double factor, double min_radius_px, double max_radius_px) {
    if (!(factor > 0.0) || !std::isfinite(factor) || !std::isfinite(min_radius_px) ||
        !std::isfinite(max_radius_px) || !(min_radius_px > 0.0) || min_radius_px > max_radius_px) {
        return camera;
    }
    double r = camera.radius_px * factor;
    if (r < min_radius_px) {
        r = min_radius_px;
    } else if (r > max_radius_px) {
        r = max_radius_px;
    }
    Camera out = camera;
    out.radius_px = r;
    return out;
}

LatLon center_of(const Camera& camera) {
    return from_unit_vector(camera.forward);
}

} // namespace globe
