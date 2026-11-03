// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "globe/terrain.hpp"
#include "globe/raster.hpp"
#include "globe/render.hpp"
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
Rgb565 terrain_color(std::int16_t meters, double latitude, TerrainStyle style) {
    if (meters >= 0 && latitude < -60)
        return rgb565(255, 255, 255);
    if (style == TerrainStyle::Hypsometric)
        return hypsometric_color(meters);
    if (meters < -2000)
        return rgb565(8, 36, 96);
    if (meters < 0)
        return rgb565(32, 108, 170);
    if (meters < 200)
        return rgb565(80, 144, 72);
    if (meters < 1000)
        return rgb565(152, 176, 88);
    if (meters < 3000)
        return rgb565(160, 120, 72);
    if (meters < 5000)
        return rgb565(200, 176, 144);
    return rgb565(248, 248, 248);
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
           static_cast<std::size_t>(grid.width()) * grid.height() * (sizeof(CellBands) + 1) + 1024;
}
std::optional<TerrainScene> TerrainScene::create(const Elevation& grid) {
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
    if (!scene.values_ || !scene.passes_)
        return std::nullopt;
    for (unsigned i = 0; i < 256; ++i) {
        const auto height = grid.value(static_cast<std::uint8_t>(i));
        if (height >= 0 && scene.first_land_ == 256)
            scene.first_land_ = i;
        scene.colors_[0][i] = terrain_color(height, 0, TerrainStyle::Hypsometric);
        scene.colors_[1][i] = terrain_color(height, 0, TerrainStyle::CelShaded);
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
void TerrainScene::assign_pass(std::size_t index, const Camera& camera, ScreenPoint focus) {
    if (bands_[index].first == bands_[index].end) {
        passes_[index] = 255;
        return;
    }
    // L(u) = -2 log(u), u at the midpoint of each of 256 bins. Generated offline;
    // a fixed integer hash supplies the bin, independent of scheduling and focus.
    static constexpr double falloff[256] = {
        12.476649250, 10.279424673, 9.257773425, 8.584828952, 8.082200095, 7.680858704, 7.346750535,
        7.060548848,  6.810222562,  6.587771292, 6.387604375, 6.205660818, 6.038897600, 5.884975518,
        5.742057590,  5.608674841,  5.483634127, 5.365953127, 5.254813425, 5.149525958, 5.049505117,
        4.954249019,  4.863324271,  4.776354047, 4.693008654, 4.612997985, 4.536065423, 4.461982880,
        4.390546714,  4.321574362,  4.254901522, 4.190379797, 4.127874710, 4.067264011, 4.008436241,
        3.951289496,  3.895730368,  3.841673023, 3.789038406, 3.737753545, 3.687750941, 3.638968034,
        3.591346737,  3.544833013,  3.499376511, 3.454930237, 3.411450264, 3.368895467, 3.327227293,
        3.286409550,  3.246408216,  3.207191274, 3.168728550, 3.130991581, 3.093953486, 3.057588847,
        3.021873613,  2.986784993,  2.952301380, 2.918402264, 2.885068159, 2.852280539, 2.820021775,
        2.788275077,  2.757024441,  2.726254604, 2.695950994, 2.666099693, 2.636687398, 2.607701384,
        2.579129469,  2.550959990,  2.523181765, 2.495784077, 2.468756638, 2.442089576, 2.415773407,
        2.389799016,  2.364157639,  2.338840846, 2.313840520, 2.289148848, 2.264758302, 2.240661625,
        2.216851820,  2.193322137,  2.170066061, 2.147077302, 2.124349785, 2.101877638, 2.079655188,
        2.057676944,  2.035937600,  2.014432016, 1.993155220, 1.972102394, 1.951268872, 1.930650133,
        1.910241793,  1.890039601,  1.870039434, 1.850237292, 1.830629292, 1.811211664, 1.791980746,
        1.772932983,  1.754064919,  1.735373194, 1.716854543, 1.698505790, 1.680323847, 1.662305707,
        1.644448446,  1.626749215,  1.609205243, 1.591813829, 1.574572343, 1.557478222, 1.540528968,
        1.523722146,  1.507055383,  1.490526363, 1.474132829, 1.457872577, 1.441743457, 1.425743372,
        1.409870273,  1.394122160,  1.378497080, 1.362993127, 1.347608435, 1.332341186, 1.317189598,
        1.302151933,  1.287226491,  1.272411608, 1.257705660, 1.243107055, 1.228614238, 1.214225686,
        1.199939911,  1.185755455,  1.171670890, 1.157684819, 1.143795874, 1.130002716, 1.116304032,
        1.102698537,  1.089184972,  1.075762103, 1.062428721, 1.049183639, 1.036025697, 1.022953755,
        1.009966696,  0.997063426,  0.984242869, 0.971503972, 0.958845702, 0.946267045, 0.933767004,
        0.921344604,  0.908998885,  0.896728908, 0.884533749, 0.872412499, 0.860364270, 0.848388186,
        0.836483389,  0.824649035,  0.812884296, 0.801188356, 0.789560416, 0.777999690, 0.766505406,
        0.755076803,  0.743713136,  0.732413671, 0.721177687, 0.710004473, 0.698893333, 0.687843582,
        0.676854543,  0.665925554,  0.655055962, 0.644245125, 0.633492411, 0.622797198, 0.612158875,
        0.601576840,  0.591050500,  0.580579272, 0.570162582, 0.559799864, 0.549490563, 0.539234130,
        0.529030026,  0.518877720,  0.508776689, 0.498726416, 0.488726395, 0.478776126, 0.468875116,
        0.459022879,  0.449218938,  0.439462821, 0.429754064, 0.420092210, 0.410476806, 0.400907410,
        0.391383583,  0.381904892,  0.372470912, 0.363081224, 0.353735412, 0.344433070, 0.335173794,
        0.325957188,  0.316782860,  0.307650424, 0.298559499, 0.289509710, 0.280500686, 0.271532061,
        0.262603475,  0.253714571,  0.244864999, 0.236054412, 0.227282468, 0.218548830, 0.209853164,
        0.201195142,  0.192574439,  0.183990735, 0.175443713, 0.166933062, 0.158458473, 0.150019642,
        0.141616268,  0.133248055,  0.124914710, 0.116615943, 0.108351468, 0.100121004, 0.091924271,
        0.083760994,  0.075630902,  0.067533725, 0.059469198, 0.051437059, 0.043437048, 0.035468910,
        0.027532392,  0.019627243,  0.011753217, 0.003910070};
    auto hash = static_cast<std::uint32_t>(index);
    hash ^= hash >> 16;
    hash *= 0x7feb352dU;
    hash ^= hash >> 15;
    hash *= 0x846ca68bU;
    hash ^= hash >> 16;
    const auto x = index % grid_.width(), y = index / grid_.width();
    const auto first = y * (grid_.width() + 1) + x;
    const auto a = projected_[first], b = projected_[first + 1],
               c = projected_[first + grid_.width() + 2], d = projected_[first + grid_.width() + 1];
    const double dx = (a.x + b.x + c.x + d.x) * 0.25 - focus.x;
    const double dy = (a.y + b.y + c.y + d.y) * 0.25 - focus.y;
    const double distance = dx * dx + dy * dy;
    double threshold = camera.radius_px * (360.0 / grid_.width()) * kDegToRad;
    threshold = threshold * threshold * falloff[hash & 255U];
    unsigned pass = 0;
    while (pass + 1 < kDissolvePasses && distance >= threshold) {
        threshold *= 2.25; // sigma grows geometrically by 1.5 per pass
        ++pass;
    }
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
        const auto* palette = colors_[style == TerrainStyle::CelShaded ? 1 : 0].data();
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
    fill_polygon(fb, clipped, count, terrain_color(meters, latitude, style));
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
