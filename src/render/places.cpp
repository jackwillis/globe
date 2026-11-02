// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/places.hpp"
#include <cmath>
namespace globe {
void PlaceScene::draw(Framebuffer& fb, const Camera& camera) const {
    for (const auto world : points_) {
        const auto v = to_camera_space(camera, world);
        if (v.z < 0)
            continue;
        const auto p = to_screen(camera, v);
        if (p.x < -3 || p.x > fb.width() + 3 || p.y < -3 || p.y > fb.height() + 3)
            continue;
        const auto x = static_cast<int>(std::floor(p.x + 0.5)),
                   y = static_cast<int>(std::floor(p.y + 0.5));
        for (int row = y - 2; row <= y + 2; ++row)
            fb.fill_span(row, x - 2, x + 2, 0);
        for (int row = y - 1; row <= y + 1; ++row)
            fb.fill_span(row, x - 1, x + 1, 0xffff);
    }
}
} // namespace globe
