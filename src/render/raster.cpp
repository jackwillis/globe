// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/raster.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>

namespace globe {

namespace {

constexpr double kBox = 1.0e6;
// Largest shade value that still floors to palette index 255.
constexpr double kMaxShade = 255.9999;

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

// Call only after finite input has been confined to the safety box. Truncation is floor for
// nonnegative values; negative nonintegers need one more step toward minus infinity.
std::int64_t floor_coordinate(double v) {
    const auto truncated = static_cast<std::int64_t>(v);
    return truncated - (v < static_cast<double>(truncated) ? 1 : 0);
}

} // namespace

void draw_line(Framebuffer& fb, double x0, double y0, double x1, double y1, Rgb565 color) {
    if (!(std::isfinite(x0) && std::isfinite(y0) && std::isfinite(x1) && std::isfinite(y1))) {
        return;
    }
    const bool inside = x0 >= -kBox && x0 <= kBox && y0 >= -kBox && y0 <= kBox && x1 >= -kBox &&
                        x1 <= kBox && y1 >= -kBox && y1 <= kBox;
    if (!inside && !clip_to_box(x0, y0, x1, y1)) {
        return;
    }
    std::int64_t ax = floor_coordinate(x0);
    std::int64_t ay = floor_coordinate(y0);
    std::int64_t bx = floor_coordinate(x1);
    std::int64_t by = floor_coordinate(y1);
    if (std::max(ay, by) < fb.first_row() || std::min(ay, by) >= fb.end_row()) {
        return;
    }
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
        walk(ay, ax, by, bx, fb.first_row(), fb.end_row() - 1, [&](std::int64_t y, std::int64_t x) {
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
    const int y_first = std::max(
        fb.first_row(), static_cast<int>(std::min(std::max(0.0, std::ceil(cy - radius - 0.5)), h)));
    const int y_last = std::min(
        fb.end_row() - 1,
        static_cast<int>(std::max(std::min(h - 1.0, std::floor(cy + radius - 0.5)), -1.0)));
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

namespace {

struct ShadeVertex {
    double x, y, v;
};

// x of the edge a->b at height cy. Callers pass the endpoints with a.y < b.y, ordered the same
// way for every triangle, so a shared edge gets bit-identical positions from both sides.
double edge_x(const ShadeVertex& a, const ShadeVertex& b, double cy) {
    return a.x + (cy - a.y) * (b.x - a.x) / (b.y - a.y);
}

bool before(const ShadeVertex& a, const ShadeVertex& b) {
    return a.y < b.y || (a.y == b.y && a.x < b.x);
}

// static_cast<int>(std::clamp(std::ceil(v), 0.0, limit)) for finite v and an integral limit that
// fits an int, without the libm call (slow on the G4) or the extra compare of clamp. Same value
// for every such input, including -0.0 and exact integers. NaN gives 0 here; the old expression
// converted a NaN to int, which is undefined, and callers have already rejected non-finite input.
inline int clamp_ceil(double v, double limit) {
    if (!(v > 0.0)) {
        return 0;
    }
    if (v >= limit) {
        return static_cast<int>(limit);
    }
    const int i = static_cast<int>(v); // 0 <= v < limit: truncation is floor
    return i + (v > static_cast<double>(i) ? 1 : 0);
}

} // namespace

void fill_triangle_shaded(Framebuffer& fb, double x0, double y0, double v0, double x1, double y1,
                          double v1, double x2, double y2, double v2, const Rgb565* palette) {
    // Reject non-finite input and coordinates outside the supported box. The values only need to be
    // finite (they are clamped below); fabs(v) <= DBL_MAX is false for NaN and infinity.
    if (!(std::fabs(x0) <= kBox && std::fabs(y0) <= kBox && std::fabs(x1) <= kBox &&
          std::fabs(y1) <= kBox && std::fabs(x2) <= kBox && std::fabs(y2) <= kBox &&
          std::fabs(v0) <= DBL_MAX && std::fabs(v1) <= DBL_MAX && std::fabs(v2) <= DBL_MAX)) {
        return;
    }
    ShadeVertex a{x0, y0, std::clamp(v0, 0.0, kMaxShade)};
    ShadeVertex b{x1, y1, std::clamp(v1, 0.0, kMaxShade)};
    ShadeVertex c{x2, y2, std::clamp(v2, 0.0, kMaxShade)};
    if (before(b, a)) {
        std::swap(a, b);
    }
    if (before(c, b)) {
        std::swap(b, c);
        if (before(b, a)) {
            std::swap(a, b);
        }
    }
    // a, b, c now run top to bottom (ties left to right).
    const double det = (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
    if (!(std::fabs(det) > 1.0e-9)) {
        return;
    }
    const double gx = ((b.v - a.v) * (c.y - a.y) - (c.v - a.v) * (b.y - a.y)) / det;
    const double gy = ((c.v - a.v) * (b.x - a.x) - (b.v - a.v) * (c.x - a.x)) / det;

    const double h = static_cast<double>(fb.height());
    const double w = static_cast<double>(fb.width());
    const int y_first = std::max(fb.first_row(), clamp_ceil(a.y - 0.5, h));
    const int y_end = std::min(fb.end_row(), clamp_ceil(c.y - 0.5, h));
    for (int y = y_first; y < y_end; ++y) {
        const double cy = y + 0.5;
        const double long_x = edge_x(a, c, cy);
        const double short_x = cy < b.y ? edge_x(a, b, cy) : edge_x(b, c, cy);
        const double left = std::min(long_x, short_x);
        const double right = std::max(long_x, short_x);
        const int xs = clamp_ceil(left - 0.5, w);
        const int xe = clamp_ceil(right - 0.5, w) - 1;
        if (xs > xe) {
            continue;
        }
        // Plane value at the first and last covered pixel centers, clamped so rounding can never
        // index outside the palette. The truncated fixed-point step keeps every pixel between them.
        const double row = a.v + gy * (cy - a.y) - gx * a.x;
        const double vs = std::clamp(row + gx * (xs + 0.5), 0.0, kMaxShade);
        const double ve = std::clamp(row + gx * (xe + 0.5), 0.0, kMaxShade);
        std::int32_t value = static_cast<std::int32_t>(vs * 65536.0);
        const std::int32_t step =
            xe > xs ? static_cast<std::int32_t>((ve - vs) / (xe - xs) * 65536.0) : 0;
        if (fb.recording_coverage())
            fb.mark_span(y, xs, xe);
        Rgb565* out =
            fb.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(fb.stride());
        for (int x = xs; x <= xe; ++x) {
            out[x] = palette[value >> 16];
            value += step;
        }
    }
}

} // namespace globe
