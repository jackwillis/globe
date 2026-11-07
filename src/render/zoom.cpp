// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/zoom.hpp"
#include <cmath>
#include <cstring>
namespace globe {
std::size_t scale_frame(const Framebuffer& src, Framebuffer& dst, double cx, double cy,
                        double factor, Rgb565 outside, Rgb565 hole, double disc_radius,
                        std::vector<int>& columns) {
    if (src.width() != dst.width() || src.height() != dst.height() || !std::isfinite(factor) ||
        factor <= 0 || !std::isfinite(cx) || !std::isfinite(cy))
        return kInvalidZoom;
    const int w = dst.width(), h = dst.height();
    columns.resize(static_cast<std::size_t>(w));
    for (int x = 0; x < w; ++x) {
        const int sx = static_cast<int>(std::floor(cx + (x + 0.5 - cx) / factor));
        columns[static_cast<std::size_t>(x)] = sx >= 0 && sx < w ? sx : -1;
    }
    std::size_t holes = 0;
    const double limit = disc_radius * disc_radius;
    for (int y = 0; y < h; ++y) {
        const int sy = static_cast<int>(std::floor(cy + (y + 0.5 - cy) / factor));
        Rgb565* out =
            dst.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(dst.stride());
        const bool row_ok = sy >= 0 && sy < h;
        const Rgb565* in = row_ok ? src.data() + static_cast<std::size_t>(sy) *
                                                     static_cast<std::size_t>(src.stride())
                                  : nullptr;
        const double dy = y + 0.5 - cy;
        for (int x = 0; x < w; ++x) {
            const int sx = columns[static_cast<std::size_t>(x)];
            if (row_ok && sx >= 0) {
                out[x] = in[sx];
                continue;
            }
            const double dx = x + 0.5 - cx;
            if (dx * dx + dy * dy <= limit) {
                out[x] = hole;
                ++holes;
            } else {
                out[x] = outside;
            }
        }
    }
    return holes;
}
} // namespace globe
