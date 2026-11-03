// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/terrain.hpp"
#include "globe/raster.hpp"
#include "globe/render.hpp"
#include "globe/trackball.hpp"
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
                          {3000, 151, 110, 69}, {4500, 200, 185, 160}, {6000, 255, 255, 255}};
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
Rgb565 terrain_color(std::int16_t meters, double latitude, TerrainStyle style) {
    if (meters >= 0 && latitude < -60)
        return rgb565(255, 255, 255);
    if (style == TerrainStyle::Map)
        return landclass_color(meters, LandClass::Unknown);
    return hypsometric_color(meters);
}
// The Map land palette, one row per class (Unknown, Desert, Forest, GrassShrub, Tundra). Change the
// saturation of the whole look here; relief shading lifts each color a little per 500 m of height.
Rgb565 landclass_color(std::int16_t meters, LandClass kind) {
    if (meters < 0)
        return kMapOcean;
    struct Color {
        int r, g, b;
    };
    static constexpr Color map[kLandClassCount] = {
        {206, 214, 170}, {236, 208, 140}, {102, 166, 92}, {176, 200, 110}, {196, 204, 178}};
    const auto i = static_cast<unsigned>(kind);
    const auto c = map[i < kLandClassCount ? i : 0];
    const int lift = std::clamp(static_cast<int>(meters), 0, 6000) / 500 * 2;
    return rgb565(static_cast<std::uint8_t>(std::min(255, c.r + lift)),
                  static_cast<std::uint8_t>(std::min(255, c.g + lift)),
                  static_cast<std::uint8_t>(std::min(255, c.b + lift)));
}
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
std::size_t TerrainScene::required_bytes(const Elevation& grid) {
    return static_cast<std::size_t>(grid.width() + 1) * (grid.height() + 1) *
               (2 * sizeof(Vec3) + 1) +
           static_cast<std::size_t>(grid.width()) * grid.height() * (sizeof(CellBands) + 2) + 3072;
}
std::optional<TerrainScene> TerrainScene::create(const Elevation& grid,
                                                 const LandClasses* classes) {
    const auto corners = static_cast<std::size_t>(grid.width() + 1) * (grid.height() + 1);
    const auto cells = static_cast<std::size_t>(grid.width()) * grid.height();
    if (required_bytes(grid) > kTerrainCacheBudget)
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
    scene.values_.reset(new (std::nothrow) std::uint8_t[corners]);
    scene.passes_.reset(new (std::nothrow) std::uint8_t[cells]);
    scene.class_cells_.reset(new (std::nothrow) std::uint8_t[cells]);
    if (!scene.values_ || !scene.passes_ || !scene.class_cells_)
        return std::nullopt;
    for (unsigned i = 0; i < 256; ++i) {
        const auto height = grid.value(static_cast<std::uint8_t>(i));
        if (height >= 0 && scene.first_land_ == 256)
            scene.first_land_ = i;
        for (unsigned kind = 0; kind < kLandClassCount; ++kind)
            scene.class_colors_[kind][i] = landclass_color(height, static_cast<LandClass>(kind));
        scene.colors_[i] = terrain_color(height, 0, TerrainStyle::Relief);
    }
    for (std::uint32_t y = 0; y < grid.height(); ++y)
        for (std::uint32_t x = 0; x < grid.width(); ++x) {
            const double lat = grid.south_deg() +
                               (grid.north_deg() - grid.south_deg()) * (y + 0.5) / grid.height();
            const double lon =
                grid.west_deg() + (grid.east_deg() - grid.west_deg()) * (x + 0.5) / grid.width();
            scene.class_cells_[static_cast<std::size_t>(y) * grid.width() + x] =
                static_cast<std::uint8_t>(classes ? classes->at(lat, lon) : LandClass::Unknown);
        }
    const bool wrap = grid.west_deg() == -180 && grid.east_deg() == 180;
    for (std::uint32_t y = 0; y <= grid.height(); ++y)
        for (std::uint32_t x = 0; x <= grid.width(); ++x) {
            unsigned sum = 0, n = 0;
            for (int dy = -1; dy <= 0; ++dy)
                for (int dx = -1; dx <= 0; ++dx) {
                    int row = static_cast<int>(y) + dy, col = static_cast<int>(x) + dx;
                    if (row < 0 || row >= static_cast<int>(grid.height()))
                        continue;
                    if (wrap)
                        col =
                            (col + static_cast<int>(grid.width())) % static_cast<int>(grid.width());
                    if (col < 0 || col >= static_cast<int>(grid.width()))
                        continue;
                    sum += grid.cells()[static_cast<std::size_t>(row) * grid.width() +
                                        static_cast<unsigned>(col)];
                    ++n;
                }
            scene.values_[static_cast<std::size_t>(y) * (grid.width() + 1) + x] =
                static_cast<std::uint8_t>(sum / n);
        }
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
unsigned PassPlan::pass_at(double x, double y) const {
    const double dx = x - focus.x, dy = y - focus.y;
    const double v = dx * dx + dy * dy;
    unsigned pass = 0;
    while (pass < kRefinePasses - 1 && v >= limit[pass])
        ++pass;
    return pass;
}
PassPlan make_pass_plan(const Camera& camera, ScreenPoint focus) {
    PassPlan plan;
    plan.focus = std::isfinite(focus.x) && std::isfinite(focus.y)
                     ? focus
                     : ScreenPoint{camera.center_x, camera.center_y};
    // Reach the far side of the disc.
    const double rx = plan.focus.x - camera.center_x, ry = plan.focus.y - camera.center_y;
    const double farthest = std::max(std::sqrt(rx * rx + ry * ry) + camera.radius_px, 1e-9);
    for (unsigned k = 0; k < kRefinePasses - 1; ++k) {
        const double edge = farthest * (k + 1) / kRefinePasses;
        plan.limit[k] = edge * edge;
    }
    return plan;
}
void TerrainScene::assign_pass(std::size_t index, const PassPlan& plan) {
    if (bands_[index].first == bands_[index].end) {
        passes_[index] = 255;
        return;
    }
    const auto x = index % grid_.width(), y = index / grid_.width();
    const auto first = y * (grid_.width() + 1) + x;
    const auto a = first, b = first + 1, c = first + grid_.width() + 2,
               d = first + grid_.width() + 1;
    const unsigned pass = plan.pass_at(
        (projected_[a].x + projected_[b].x + projected_[c].x + projected_[d].x) * 0.25,
        (projected_[a].y + projected_[b].y + projected_[c].y + projected_[d].y) * 0.25);
    passes_[index] = static_cast<std::uint8_t>(pass);
}
void TerrainScene::draw_cell(Framebuffer& fb, std::size_t index, int band, int band_rows,
                             TerrainStyle style, bool smooth) const {
    (void)band_rows;
    if (band < bands_[index].first || band >= bands_[index].end)
        return;
    const auto x = index % grid_.width(), y = index / grid_.width(),
               first = y * (grid_.width() + 1) + x;
    const Vec3 input[] = {projected_[first], projected_[first + 1],
                          projected_[first + grid_.width() + 2],
                          projected_[first + grid_.width() + 1]};
    const auto index_value = grid_.cells()[index];
    const auto meters = grid_.value(index_value);
    const double latitude = grid_.south_deg() + (grid_.north_deg() - grid_.south_deg()) *
                                                    (static_cast<double>(y) + 0.5) / grid_.height();
    if (smooth && !(meters >= 0 && latitude < -60) && input[0].z >= 0 && input[1].z >= 0 &&
        input[2].z >= 0 && input[3].z >= 0) {
        const unsigned low = meters >= 0 ? first_land_ : 0;
        const unsigned high = meters >= 0 ? 255 : first_land_ - 1;
        const auto value = [&](std::size_t i) {
            return static_cast<double>(std::clamp(static_cast<unsigned>(values_[i]), low, high));
        };
        const auto a = value(first), b = value(first + 1), c = value(first + grid_.width() + 2),
                   d = value(first + grid_.width() + 1);
        const auto* palette =
            style == TerrainStyle::Map ? class_colors_[class_cells_[index]].data() : colors_.data();
        fill_triangle_shaded(fb, input[0].x, input[0].y, a, input[1].x, input[1].y, b, input[2].x,
                             input[2].y, c, palette);
        fill_triangle_shaded(fb, input[0].x, input[0].y, a, input[2].x, input[2].y, c, input[3].x,
                             input[3].y, d, palette);
        return;
    }
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
    const auto color =
        meters >= 0 && latitude < -60
            ? rgb565(255, 255, 255)
            : this->color(index_value, style, static_cast<LandClass>(class_cells_[index]));
    fill_polygon(fb, clipped, count, color);
}
void TerrainScene::draw_base(Framebuffer& fb, const Camera& camera) const {
    const auto sx = std::max<std::uint32_t>(1, grid_.width() / 36),
               sy = std::max<std::uint32_t>(1, grid_.height() / 18);
    for (std::uint32_t y = 0; y < grid_.height(); y += sy)
        for (std::uint32_t x = 0; x < grid_.width(); x += sx) {
            const auto endx = std::min(x + sx, grid_.width()),
                       endy = std::min(y + sy, grid_.height());
            const auto value =
                grid_.cells()[static_cast<std::size_t>((y + endy) / 2) * grid_.width() +
                              (x + endx) / 2];
            if (grid_.value(value) < 0)
                continue;
            const auto project = [&](std::uint32_t col, std::uint32_t row) {
                const auto v = to_camera_space(
                    camera, world_[static_cast<std::size_t>(row) * (grid_.width() + 1) + col]);
                const auto p = to_screen(camera, v);
                return Vec3{p.x, p.y, v.z};
            };
            const Vec3 input[] = {project(x, y), project(endx, y), project(endx, endy),
                                  project(x, endy)};
            Vec3 clipped[8];
            int count = 0;
            auto previous = input[3];
            for (auto current : input) {
                if ((previous.z >= 0) != (current.z >= 0))
                    clipped[count++] =
                        previous + (current - previous) * (previous.z / (previous.z - current.z));
                if (current.z >= 0)
                    clipped[count++] = current;
                previous = current;
            }
            const double latitude = grid_.south_deg() + (grid_.north_deg() - grid_.south_deg()) *
                                                            (y + endy) * 0.5 / grid_.height();
            fill_polygon(fb, clipped, count, latitude < -60 ? rgb565(255, 255, 255) : kMovingLand);
        }
}
} // namespace globe
