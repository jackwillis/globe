// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/geometry.hpp"

#include <cmath>

namespace globe {

double length(Vec3 v) { return std::sqrt(dot(v, v)); }

Vec3 normalized(Vec3 v) {
    const double len = length(v);
    if (!(len > 0.0) || !std::isfinite(len)) {
        return v;
    }
    return v * (1.0 / len);
}

namespace {

// Wraps to [-180, 180). The result is exact for inputs already in range.
double wrap_longitude(double lon_deg) {
    double w = std::fmod(lon_deg + 180.0, 360.0);
    if (w < 0.0) {
        w += 360.0;
    }
    return w - 180.0;
}

} // namespace

Vec3 to_unit_vector(LatLon p) {
    if (!std::isfinite(p.lat_deg) || !std::isfinite(p.lon_deg)) {
        return {0.0, 0.0, 1.0};
    }
    double lat = p.lat_deg;
    if (lat > 90.0) {
        lat = 90.0;
    } else if (lat < -90.0) {
        lat = -90.0;
    }
    const double phi = lat * kDegToRad;
    const double lambda = wrap_longitude(p.lon_deg) * kDegToRad;
    const double c = std::cos(phi);
    return {c * std::sin(lambda), std::sin(phi), c * std::cos(lambda)};
}

LatLon from_unit_vector(Vec3 v) {
    const Vec3 u = normalized(v);
    const double horizontal = std::sqrt(u.x * u.x + u.z * u.z);
    const double lat = std::atan2(u.y, horizontal) * kRadToDeg;
    // Treat near-polar vectors as longitude 0: atan2 of rounding noise would be arbitrary.
    const double lon = horizontal < 1e-12 ? 0.0 : std::atan2(u.x, u.z) * kRadToDeg;
    return {lat, lon};
}

double angle_between(Vec3 a, Vec3 b) { return std::atan2(length(cross(a, b)), dot(a, b)); }

} // namespace globe
