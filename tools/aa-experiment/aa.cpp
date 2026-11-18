// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "aa.hpp"
#include <algorithm>
#include <cmath>
#include <new>

namespace aa_experiment {
std::optional<Workspace> Workspace::create(int w, int h, std::size_t budget, Allocate allocate) {
    if (w < 1 || h < 1 || w > 4096 || h > 4096)
        return std::nullopt;
    const auto n = static_cast<std::size_t>(w) * h;
    if (n > budget)
        return std::nullopt;
    std::unique_ptr<std::uint8_t[]> p(allocate ? allocate(n) : new (std::nothrow) std::uint8_t[n]);
    if (!p)
        return std::nullopt;
    return Workspace(w, h, std::move(p));
}
globe::Rgb565 blend(globe::Rgb565 d, globe::Rgb565 s, unsigned a) {
    if (a == 0)
        return d;
    if (a >= 255)
        return s;
    unsigned out = 0;
    for (int shift : {11, 5, 0}) {
        const unsigned bits = shift == 5 ? 6 : 5, m = (1u << bits) - 1;
        const auto expand = [bits](unsigned v) {
            return bits == 5 ? ((v << 3) | (v >> 2)) : ((v << 2) | (v >> 4));
        };
        const unsigned z =
            (a * expand((s >> shift) & m) + (255 - a) * expand((d >> shift) & m) + 127) / 255;
        out |= ((z * m + 127) / 255) << shift;
    }
    return static_cast<globe::Rgb565>(out);
}
namespace {
bool same(globe::ScreenPoint a, globe::ScreenPoint b) {
    return a.x == b.x && a.y == b.y;
}
struct Bounds {
    int x0, y0, x1, y1;
};
Bounds bounds(double lo_x, double lo_y, double hi_x, double hi_y, int w, int first, int end) {
    return {std::clamp(static_cast<int>(std::floor(lo_x - 1)), 0, w),
            std::clamp(static_cast<int>(std::floor(lo_y - 1)), first, end),
            std::clamp(static_cast<int>(std::ceil(hi_x + 1)) + 1, 0, w),
            std::clamp(static_cast<int>(std::ceil(hi_y + 1)) + 1, first, end)};
}
} // namespace
Result render_feature(globe::Framebuffer& fb, std::span<const Chain> chains, globe::Rgb565 color,
                      Workspace& ws) {
    if (fb.width() != ws.width_ || fb.height() != ws.height_)
        return Result::WorkspaceMismatch;
    std::size_t total = 0;
    double lx = 1e6, ly = 1e6, hx = -1e6, hy = -1e6;
    for (const auto& chain : chains) {
        if (chain.points.size() > 1000000 || total > 1000000 - chain.points.size())
            return Result::InvalidInput;
        total += chain.points.size();
        for (auto p : chain.points) {
            if (!std::isfinite(p.x) || !std::isfinite(p.y) || std::fabs(p.x) > 1e6 ||
                std::fabs(p.y) > 1e6)
                return Result::InvalidInput;
            lx = std::min(lx, p.x);
            ly = std::min(ly, p.y);
            hx = std::max(hx, p.x);
            hy = std::max(hy, p.y);
        }
    }
    if (!total)
        return Result::Ok;
    const auto all = bounds(lx, ly, hx, hy, fb.width(), fb.first_row(), fb.end_row());
    for (int y = all.y0; y < all.y1; ++y)
        std::fill(ws.mask_.get() + y * ws.width_ + all.x0, ws.mask_.get() + y * ws.width_ + all.x1,
                  0);
    const auto paint = [&](Bounds b, auto distance) {
        for (int y = b.y0; y < b.y1; ++y)
            for (int x = b.x0; x < b.x1; ++x) {
                const double a = std::clamp(0.5 - distance(x + 0.5, y + 0.5), 0.0, 1.0);
                const auto q = static_cast<std::uint8_t>(std::floor(255 * a + 0.5));
                auto& pixel = ws.mask_[y * ws.width_ + x];
                pixel = std::max(pixel, q);
            }
    };
    const auto disc = [&](globe::ScreenPoint p) {
        paint(bounds(p.x, p.y, p.x, p.y, fb.width(), fb.first_row(), fb.end_row()),
              [p](double x, double y) { return std::hypot(x - p.x, y - p.y) - 0.5; });
    };
    for (const auto& chain : chains) {
        bool have_segment = false;
        globe::ScreenPoint first{};
        const auto segment = [&](globe::ScreenPoint a, globe::ScreenPoint b) {
            if (b.x < a.x || (b.x == a.x && b.y < a.y))
                std::swap(a, b);
            const double dx = b.x - a.x, dy = b.y - a.y, length = std::hypot(dx, dy);
            const double ux = dx / length, uy = dy / length;
            const double cx = (a.x + b.x) * 0.5, cy = (a.y + b.y) * 0.5;
            paint(bounds(std::min(a.x, b.x), std::min(a.y, b.y), std::max(a.x, b.x),
                         std::max(a.y, b.y), fb.width(), fb.first_row(), fb.end_row()),
                  [=](double x, double y) {
                      const double u = std::fabs((x - cx) * ux + (y - cy) * uy) - length * 0.5;
                      const double v = std::fabs(-(x - cx) * uy + (y - cy) * ux) - 0.5;
                      return std::hypot(std::max(u, 0.0), std::max(v, 0.0)) +
                             std::min(std::max(u, v), 0.0);
                  });
        };
        for (std::size_t i = 1; i < chain.points.size(); ++i) {
            const auto a = chain.points[i - 1], b = chain.points[i];
            if (same(a, b))
                continue;
            if (have_segment)
                disc(a);
            else
                first = a;
            segment(a, b);
            have_segment = true;
        }
        if (chain.closed && have_segment) {
            const auto last = chain.points.back();
            if (!same(last, first)) {
                disc(last);
                segment(last, first);
            }
            disc(first);
        }
    }
    for (int y = all.y0; y < all.y1; ++y)
        for (int x = all.x0; x < all.x1; ++x) {
            const auto q = ws.mask_[y * ws.width_ + x];
            if (q)
                fb.set_pixel(x, y, blend(fb.pixel(x, y), color, q));
        }
    return Result::Ok;
}
} // namespace aa_experiment
