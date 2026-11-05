// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/daynight.hpp"
#include <algorithm>
#include <cmath>
namespace globe {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg = kPi / 180.0;
double wrap360(double degrees) {
    const double r = std::fmod(degrees, 360.0);
    return r < 0 ? r + 360.0 : r;
}
// sqrt(1 - u) for u in [0, 1] by linear interpolation, so the per-pixel loop has no sqrt call
// (a libm sqrt is slow on the G4). The error is largest at the limb and well under a pixel there.
constexpr int kTable = 4096;
struct SqrtTable {
    double v[kTable + 2];
    SqrtTable() {
        for (int i = 0; i <= kTable + 1; ++i)
            v[i] = std::sqrt(std::max(0.0, 1.0 - static_cast<double>(i) / kTable));
    }
    double at(double u) const {
        const double x = u * kTable;
        const int i = static_cast<int>(x);
        return v[i] + (v[i + 1] - v[i]) * (x - i);
    }
};
// Night multiplier from the cosine of the sun's angle: 1 in daylight, floor at night.
constexpr double kDay = 0.06, kNight = -0.12, kFloor = 0.30;
double daylight(double cosine) {
    if (cosine >= kDay)
        return 1.0;
    if (cosine <= kNight)
        return kFloor;
    const double t = (cosine - kNight) / (kDay - kNight);
    const double smooth = t * t * (3.0 - 2.0 * t);
    return kFloor + (1.0 - kFloor) * smooth;
}
} // namespace
LatLon subsolar_point(std::int64_t unix_seconds) {
    // Days since J2000.0 (2000-01-01 12:00 UTC = UNIX 946728000).
    const double d = (static_cast<double>(unix_seconds) - 946728000.0) / 86400.0;
    const double mean_longitude = 280.460 + 0.9856474 * d;
    const double anomaly = (357.528 + 0.9856003 * d) * kDeg;
    const double ecliptic =
        (mean_longitude + 1.915 * std::sin(anomaly) + 0.020 * std::sin(2 * anomaly)) * kDeg;
    const double obliquity = (23.439 - 0.0000004 * d) * kDeg;
    const double declination = std::asin(std::sin(obliquity) * std::sin(ecliptic));
    const double ascension =
        std::atan2(std::cos(obliquity) * std::sin(ecliptic), std::cos(ecliptic)) / kDeg;
    const double sidereal = 280.46061837 + 360.98564736629 * d; // Greenwich, degrees
    double lon = wrap360(ascension - sidereal);
    if (lon > 180.0)
        lon -= 360.0;
    return {declination / kDeg, lon};
}
void shade_night(Framebuffer& fb, const Camera& camera, Vec3 sun_world) {
    static const SqrtTable table;
    const Vec3 sun = to_camera_space(camera, sun_world);
    const double radius = camera.radius_px;
    if (!(radius > 0) || !std::isfinite(sun.x + sun.y + sun.z))
        return;
    const int first = std::max(0, static_cast<int>(std::floor(camera.center_y - radius)));
    const int last =
        std::min(fb.height() - 1, static_cast<int>(std::ceil(camera.center_y + radius)));
    for (int y = first; y <= last; ++y) {
        const double ny = (camera.center_y - (y + 0.5)) / radius;
        const double rest = 1.0 - ny * ny;
        if (rest <= 0)
            continue;
        const double half = std::sqrt(rest) * radius;
        const int x0 = std::max(0, static_cast<int>(std::ceil(camera.center_x - half - 0.5)));
        const int x1 =
            std::min(fb.width() - 1, static_cast<int>(std::floor(camera.center_x + half - 0.5)));
        Rgb565* row =
            fb.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(fb.stride());
        const double base = ny * sun.y;
        for (int x = x0; x <= x1; ++x) {
            const double nx = (x + 0.5 - camera.center_x) / radius;
            const double u = nx * nx + ny * ny;
            if (u >= 1.0)
                continue;
            const double m = daylight(nx * sun.x + base + table.at(u) * sun.z);
            if (m >= 1.0)
                continue;
            const unsigned p = row[x];
            const double r = static_cast<double>((p >> 11) & 31) * m;
            const double g = static_cast<double>((p >> 5) & 63) * m;
            const double b = static_cast<double>(p & 31) * (m + (1.0 - m) * 0.35);
            row[x] =
                static_cast<Rgb565>((static_cast<unsigned>(r) << 11) |
                                    (static_cast<unsigned>(g) << 5) | static_cast<unsigned>(b));
        }
    }
}
} // namespace globe
