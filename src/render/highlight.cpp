// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/highlight.hpp"
#include "globe/raster.hpp"
#include <algorithm>
#include <cmath>
#include <vector>
namespace globe {
namespace {
constexpr double kPiValue = 3.14159265358979323846;
struct Pt {
    double x, y;
};
// Clips one ring to the visible hemisphere and appends the screen polygon to `out`. Where the ring
// leaves the front side the polygon follows the limb (the shorter way) to where it comes back.
void clip_ring(const Camera& camera, const std::vector<Vec3>& v,
               std::vector<std::vector<Pt>>& out) {
    const std::size_t n = v.size();
    std::size_t start = n;
    bool any_hidden = false;
    for (std::size_t i = 0; i < n; ++i)
        any_hidden |= v[i].z < 0;
    const auto screen = [&](Vec3 c) {
        return Pt{camera.center_x + camera.radius_px * c.x,
                  camera.center_y - camera.radius_px * c.y};
    };
    const auto limb = [](Vec3 a, Vec3 b) {
        const double t = a.z / (a.z - b.z);
        Vec3 c = a + (b - a) * t;
        const double len = std::hypot(c.x, c.y);
        return len > 0 ? Vec3{c.x / len, c.y / len, 0.0} : Vec3{1, 0, 0};
    };
    if (!any_hidden) {
        std::vector<Pt> poly;
        poly.reserve(n);
        for (const auto& p : v)
            poly.push_back(screen(p));
        out.push_back(std::move(poly));
        return;
    }
    for (std::size_t i = 0; i < n && start == n; ++i)
        if (v[i].z >= 0 && v[(i + n - 1) % n].z < 0)
            start = i;
    if (start == n)
        return; // entirely hidden
    std::vector<Pt> poly;
    const Vec3 entry = limb(v[(start + n - 1) % n], v[start]);
    poly.push_back(screen(entry));
    double exit_angle = 0;
    bool outside = false;
    const auto arc = [&](double from, double to) {
        double d = to - from;
        while (d > kPiValue)
            d -= 2 * kPiValue;
        while (d < -kPiValue)
            d += 2 * kPiValue;
        const int steps =
            std::max(1, static_cast<int>(std::ceil(std::fabs(d) / (3.0 * kPiValue / 180.0))));
        for (int k = 1; k < steps; ++k) {
            const double a = from + d * k / steps;
            poly.push_back(screen({std::cos(a), std::sin(a), 0.0}));
        }
    };
    for (std::size_t k = 0; k < n; ++k) {
        const Vec3 a = v[(start + k) % n], b = v[(start + k + 1) % n];
        if (!outside) {
            if (a.z >= 0 && k == 0)
                poly.push_back(screen(a));
            if (b.z >= 0)
                poly.push_back(screen(b));
            else {
                const Vec3 x = limb(a, b);
                exit_angle = std::atan2(x.y, x.x);
                poly.push_back(screen(x));
                outside = true;
            }
        } else if (b.z >= 0) {
            const Vec3 x = limb(a, b);
            const double angle = std::atan2(x.y, x.x);
            arc(exit_angle, angle);
            poly.push_back(screen(x));
            poly.push_back(screen(b));
            outside = false;
        }
    }
    out.push_back(std::move(poly));
}
} // namespace
std::size_t draw_country_outline(Framebuffer& fb, const Camera& camera, const Countries& countries,
                                 std::uint32_t id, Rgb565 color) {
    if (id >= countries.count())
        return 0;
    const auto record = countries.country(id);
    std::size_t drawn = 0;
    for (std::uint32_t r = 0; r < record.count; ++r) {
        const auto ring = countries.ring(record.first + r);
        if (ring.count < 2)
            continue;
        const auto project = [&](std::uint32_t i) {
            const auto v = to_camera_space(camera, to_unit_vector(countries.point(ring.first + i)));
            const auto p = to_screen(camera, v);
            return Vec3{p.x, p.y, v.z};
        };
        auto previous = project(ring.count - 1); // the ring is closed by its last-to-first edge
        for (std::uint32_t i = 0; i < ring.count; ++i) {
            const auto current = project(i);
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
    return drawn;
}
} // namespace globe
namespace globe {
std::size_t fill_country_dither(Framebuffer& fb, const Camera& camera, const Countries& countries,
                                std::uint32_t id, Rgb565 color) {
    if (id >= countries.count())
        return 0;
    const auto record = countries.country(id);
    std::vector<std::vector<Pt>> polygons;
    for (std::uint32_t r = 0; r < record.count; ++r) {
        const auto ring = countries.ring(record.first + r);
        if (ring.count < 3)
            continue;
        // Long edges are great-circle arcs, which curve on the screen: add a point about every
        // two degrees so the straight screen segments follow them.
        std::vector<Vec3> v;
        v.reserve(ring.count);
        Vec3 previous =
            to_camera_space(camera, to_unit_vector(countries.point(ring.first + ring.count - 1)));
        for (std::uint32_t i = 0; i < ring.count; ++i) {
            const Vec3 current =
                to_camera_space(camera, to_unit_vector(countries.point(ring.first + i)));
            const double omega = std::acos(std::clamp(dot(previous, current), -1.0, 1.0));
            const int steps = static_cast<int>(omega / (2.0 * kPiValue / 180.0));
            if (omega > 1e-9 && std::sin(omega) > 1e-9)
                for (int k = 1; k <= steps; ++k) {
                    const double t = static_cast<double>(k) / (steps + 1);
                    v.push_back(previous * (std::sin((1 - t) * omega) / std::sin(omega)) +
                                current * (std::sin(t * omega) / std::sin(omega)));
                }
            v.push_back(current);
            previous = current;
        }
        clip_ring(camera, v, polygons);
    }
    // A fill that would cover most of the disc means the limb clip picked the wrong side.
    double area = 0, top = 1e300, bottom = -1e300;
    for (const auto& poly : polygons) {
        double a = 0;
        for (std::size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
            a += poly[j].x * poly[i].y - poly[i].x * poly[j].y;
            top = std::min(top, poly[i].y);
            bottom = std::max(bottom, poly[i].y);
        }
        area += std::fabs(a) / 2;
    }
    if (polygons.empty() || area > 0.55 * kPiValue * camera.radius_px * camera.radius_px)
        return 0;
    std::size_t painted = 0;
    const int y0 = std::max(0, static_cast<int>(std::ceil(top - 0.5)));
    const int y1 = std::min(fb.height() - 1, static_cast<int>(std::floor(bottom - 0.5)));
    std::vector<double> xs;
    for (int y = y0; y <= y1; ++y) {
        const double cy = y + 0.5;
        xs.clear();
        for (const auto& poly : polygons)
            for (std::size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
                const Pt a = poly[j], b = poly[i];
                if ((a.y <= cy && b.y > cy) || (b.y <= cy && a.y > cy))
                    xs.push_back(a.x + (cy - a.y) * (b.x - a.x) / (b.y - a.y));
            }
        std::sort(xs.begin(), xs.end());
        for (std::size_t k = 0; k + 1 < xs.size(); k += 2) {
            const int x0 = std::max(0, static_cast<int>(std::ceil(xs[k] - 0.5)));
            const int x1 = std::min(fb.width() - 1, static_cast<int>(std::floor(xs[k + 1] - 0.5)));
            for (int x = x0; x <= x1; ++x)
                if (((x + 2 * y) & 3) == 0) {
                    fb.set_pixel(x, y, color);
                    ++painted;
                }
        }
    }
    return painted;
}
} // namespace globe
