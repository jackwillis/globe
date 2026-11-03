// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/highlight.hpp"
#include "globe/raster.hpp"
namespace globe {
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
