// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/terrain.hpp"
#include <algorithm>
#include <cmath>
#include <new>
namespace globe {
namespace {
struct Stop {
    int height, r, g, b;
};
constexpr Stop stops[] = {{-11000, 4, 12, 48},  {-2000, 12, 54, 112},  {-200, 30, 105, 168},
                          {-1, 80, 160, 205},   {0, 78, 134, 67},      {1000, 143, 158, 83},
                          {3000, 151, 110, 69}, {5000, 190, 165, 130}, {9000, 255, 255, 255}};
void fill_polygon(Framebuffer& fb, const Vec3* p, int count, Rgb565 color) {
    if (count < 3)
        return;
    double low = p[0].y, high = low;
    for (int i = 1; i < count; ++i) {
        low = std::min(low, p[i].y);
        high = std::max(high, p[i].y);
    }
    const int first = std::max(
        fb.first_row(),
        static_cast<int>(std::clamp(std::ceil(low - 0.5), 0.0, static_cast<double>(fb.height()))));
    const int end = std::min(
        fb.end_row(),
        static_cast<int>(std::clamp(std::ceil(high - 0.5), 0.0, static_cast<double>(fb.height()))));
    for (int y = first; y < end; ++y) {
        const double cy = y + 0.5;
        double left = 0, right = 0;
        bool found = false;
        for (int i = 0, j = count - 1; i < count; j = i++) {
            const auto a = p[j], b = p[i];
            if ((a.y <= cy && b.y > cy) || (b.y <= cy && a.y > cy)) {
                const double x = a.x + (cy - a.y) * (b.x - a.x) / (b.y - a.y);
                if (!found) {
                    left = right = x;
                    found = true;
                } else {
                    left = std::min(left, x);
                    right = std::max(right, x);
                }
            }
        }
        if (found) {
            const int x0 = static_cast<int>(
                std::clamp(std::ceil(left - 0.5), 0.0, static_cast<double>(fb.width())));
            const int x1 = static_cast<int>(std::clamp(std::ceil(right - 0.5), 0.0,
                                                       static_cast<double>(fb.width()))) -
                           1;
            if (x0 <= x1)
                fb.fill_span(y, x0, x1, color);
        }
    }
}
} // namespace
Rgb565 hypsometric_color(std::int16_t meters) {
    const int h = meters;
    if (h <= stops[0].height)
        return rgb565(stops[0].r, stops[0].g, stops[0].b);
    for (std::size_t i = 1; i < sizeof(stops) / sizeof(stops[0]); ++i)
        if (h <= stops[i].height) {
            const auto a = stops[i - 1], b = stops[i];
            const int span = b.height - a.height, t = h - a.height;
            return rgb565(static_cast<std::uint8_t>(a.r + (b.r - a.r) * t / span),
                          static_cast<std::uint8_t>(a.g + (b.g - a.g) * t / span),
                          static_cast<std::uint8_t>(a.b + (b.b - a.b) * t / span));
        }
    return rgb565(255, 255, 255);
}
std::optional<TerrainScene> TerrainScene::create(const Elevation& grid) {
    const auto corners = static_cast<std::size_t>(grid.width() + 1) * (grid.height() + 1);
    const auto cells = static_cast<std::size_t>(grid.width()) * grid.height();
    if (corners * 2 * sizeof(Vec3) + cells * sizeof(CellBands) + 512 > kTerrainCacheBudget)
        return std::nullopt;
    std::unique_ptr<Vec3[]> world(new (std::nothrow) Vec3[corners]),
        projected(new (std::nothrow) Vec3[corners]);
    std::unique_ptr<CellBands[]> bands(new (std::nothrow) CellBands[cells]);
    if (!world || !projected || !bands)
        return std::nullopt;
    for (std::uint32_t y = 0; y <= grid.height(); ++y)
        for (std::uint32_t x = 0; x <= grid.width(); ++x) {
            const double lat =
                grid.south_deg() + (grid.north_deg() - grid.south_deg()) * y / grid.height();
            const double lon =
                grid.west_deg() + (grid.east_deg() - grid.west_deg()) * x / grid.width();
            world[static_cast<std::size_t>(y) * (grid.width() + 1) + x] =
                to_unit_vector({lat, lon});
        }
    TerrainScene scene(grid, std::move(world), std::move(projected), std::move(bands));
    for (unsigned i = 0; i < 256; ++i)
        scene.colors_[i] = hypsometric_color(grid.value(static_cast<std::uint8_t>(i)));
    return scene;
}
void TerrainScene::project_corner(std::size_t index, const Camera& camera) {
    const auto v = to_camera_space(camera, world_[index]);
    const auto screen = to_screen(camera, v);
    projected_[index] = {screen.x, screen.y, v.z};
}
void TerrainScene::bound_cell(std::size_t index, int height, int band_rows) {
    const auto x = index % grid_.width(), y = index / grid_.width(),
               first = y * (grid_.width() + 1) + x;
    const auto a = projected_[first], b = projected_[first + 1],
               c = projected_[first + grid_.width() + 2], d = projected_[first + grid_.width() + 1];
    if (a.z < 0 && b.z < 0 && c.z < 0 && d.z < 0) {
        bands_[index] = {};
        return;
    }
    const double low = std::min({a.y, b.y, c.y, d.y}), high = std::max({a.y, b.y, c.y, d.y});
    const int begin =
        static_cast<int>(std::clamp(low, 0.0, static_cast<double>(height))) / band_rows;
    const int end = std::min(
        (height + band_rows - 1) / band_rows,
        static_cast<int>(std::clamp(high, 0.0, static_cast<double>(height))) / band_rows + 1);
    bands_[index] = {static_cast<std::int16_t>(begin), static_cast<std::int16_t>(end)};
}
void TerrainScene::draw_cell(Framebuffer& fb, std::size_t index, int band, int band_rows) const {
    (void)band_rows;
    if (band < bands_[index].first || band >= bands_[index].end)
        return;
    const auto x = index % grid_.width(), y = index / grid_.width(),
               first = y * (grid_.width() + 1) + x;
    const Vec3 input[] = {projected_[first], projected_[first + 1],
                          projected_[first + grid_.width() + 2],
                          projected_[first + grid_.width() + 1]};
    Vec3 clipped[8];
    int count = 0;
    Vec3 previous = input[3];
    for (const auto current : input) {
        if ((previous.z >= 0) != (current.z >= 0)) {
            const double t = previous.z / (previous.z - current.z);
            clipped[count++] = previous + (current - previous) * t;
        }
        if (current.z >= 0)
            clipped[count++] = current;
        previous = current;
    }
    fill_polygon(fb, clipped, count, colors_[grid_.cells()[index]]);
}
} // namespace globe
