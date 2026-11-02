// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/raster.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace globe {

namespace {

constexpr double kBox = 1.0e6;

// Liang-Barsky against [-kBox, kBox]^2; false if the segment misses the box entirely.
bool clip_to_box(double& x0, double& y0, double& x1, double& y1) {
    double t0 = 0.0;
    double t1 = 1.0;
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double p[4] = {-dx, dx, -dy, dy};
    const double q[4] = {x0 + kBox, kBox - x0, y0 + kBox, kBox - y0};
    for (int i = 0; i < 4; ++i) {
        if (p[i] == 0.0) {
            if (q[i] < 0.0) {
                return false;
            }
        } else {
            const double t = q[i] / p[i];
            if (p[i] < 0.0) {
                t0 = std::max(t0, t);
            } else {
                t1 = std::min(t1, t);
            }
        }
    }
    if (t0 > t1) {
        return false;
    }
    const double nx0 = x0 + t0 * dx;
    const double ny0 = y0 + t0 * dy;
    x1 = x0 + t1 * dx;
    y1 = y0 + t1 * dy;
    x0 = nx0;
    y0 = ny0;
    return true;
}

// Midpoint line along the major axis a (b is the minor axis). Requires a0 <= a1 and a1 - a0 >=
// |b1 - b0|. The minor coordinate at step k is b0 + sgn * floor((2 * |db| * k + da) / (2 * da)),
// the closed form of Bresenham's error term, so any sub-range of k can be drawn exactly.
template <typename Plot>
void walk(std::int64_t a0, std::int64_t b0, std::int64_t a1, std::int64_t b1, std::int64_t a_min,
          std::int64_t a_max, Plot plot) {
    const std::int64_t da = a1 - a0;
    const std::int64_t db = b1 - b0;
    const std::int64_t adb = db < 0 ? -db : db;
    const std::int64_t sgn = db < 0 ? -1 : 1;
    const std::int64_t kmin = std::max<std::int64_t>(0, a_min - a0);
    const std::int64_t kmax = std::min<std::int64_t>(da, a_max - a0);
    if (kmin > kmax) {
        return;
    }
    if (da == 0) {
        plot(a0, b0);
        return;
    }
    const std::int64_t num = 2 * adb * kmin + da;
    std::int64_t b = b0 + sgn * (num / (2 * da));
    std::int64_t r = num % (2 * da);
    for (std::int64_t k = kmin; k <= kmax; ++k) {
        plot(a0 + k, b);
        r += 2 * adb;
        if (r >= 2 * da) {
            r -= 2 * da;
            b += sgn;
        }
    }
}

} // namespace

void draw_line(Framebuffer& fb, double x0, double y0, double x1, double y1, Rgb565 color) {
    if (!(std::isfinite(x0) && std::isfinite(y0) && std::isfinite(x1) && std::isfinite(y1))) {
        return;
    }
    if (!clip_to_box(x0, y0, x1, y1)) {
        return;
    }
    std::int64_t ax = static_cast<std::int64_t>(std::floor(x0));
    std::int64_t ay = static_cast<std::int64_t>(std::floor(y0));
    std::int64_t bx = static_cast<std::int64_t>(std::floor(x1));
    std::int64_t by = static_cast<std::int64_t>(std::floor(y1));
    const auto absval = [](std::int64_t v) { return v < 0 ? -v : v; };
    if (absval(bx - ax) >= absval(by - ay)) {
        if (ax > bx) { // canonical direction: a line and its reverse draw the same pixels
            std::swap(ax, bx);
            std::swap(ay, by);
        }
        // set_pixel ignores the minor coordinate when it is outside the buffer; its range is
        // within +-1e6 (the clip box), so the int conversion is safe.
        walk(ax, ay, bx, by, 0, fb.width() - 1, [&](std::int64_t x, std::int64_t y) {
            fb.set_pixel(static_cast<int>(x), static_cast<int>(y), color);
        });
    } else {
        if (ay > by) {
            std::swap(ax, bx);
            std::swap(ay, by);
        }
        walk(ay, ax, by, bx, 0, fb.height() - 1, [&](std::int64_t y, std::int64_t x) {
            fb.set_pixel(static_cast<int>(x), static_cast<int>(y), color);
        });
    }
}

void fill_disc(Framebuffer& fb, double cx, double cy, double radius, Rgb565 color) {
    if (!(std::isfinite(cx) && std::isfinite(cy) && std::isfinite(radius)) || !(radius > 0.0)) {
        return;
    }
    // Clamp to the buffer before converting to int: the disc may be far larger than, or far
    // outside, the buffer.
    const double h = static_cast<double>(fb.height());
    const int y_first = static_cast<int>(std::min(std::max(0.0, std::ceil(cy - radius - 0.5)), h));
    const int y_last =
        static_cast<int>(std::max(std::min(h - 1.0, std::floor(cy + radius - 0.5)), -1.0));
    for (int y = y_first; y <= y_last; ++y) {
        const double dy = y + 0.5 - cy;
        const double d2 = radius * radius - dy * dy;
        if (d2 < 0.0) {
            continue;
        }
        const double half = std::sqrt(d2);
        // Clamp before converting: the span may be astronomically wide when zoomed in.
        const double lo = std::max(-1.0, std::ceil(cx - half - 0.5));
        const double hi = std::min(static_cast<double>(fb.width()), std::floor(cx + half - 0.5));
        if (lo <= hi) {
            fb.fill_span(y, static_cast<int>(lo), static_cast<int>(hi), color);
        }
    }
}

} // namespace globe
