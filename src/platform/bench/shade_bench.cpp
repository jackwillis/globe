// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Times the settled-frame terrain fill two ways on the same cells and camera: the existing flat
// per-cell polygon fill (TerrainScene::draw_cell) and two fill_triangle_shaded calls per cell.
// Projection is excluded from both timings. The shaded path skips cells with any corner behind
// the horizon (the flat path clips them), so it draws slightly fewer limb pixels.
//
//   shade-bench [repeats]   (default 20)
#include "clock.hpp"
#include "globe/elevation.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/framebuffer.hpp"
#include "globe/geometry.hpp"
#include "globe/projection.hpp"
#include "globe/raster.hpp"
#include "globe/terrain.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace globe;

namespace {

struct Quad {
    double x[4], y[4], v[4];
};

std::uint64_t checksum(const Framebuffer& fb) {
    std::uint64_t h = 1469598103934665603ULL;
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            h = (h ^ fb.pixel(x, y)) * 1099511628211ULL;
        }
    }
    return h;
}

long covered(const Framebuffer& fb, Rgb565 background) {
    long n = 0;
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            n += fb.pixel(x, y) != background;
        }
    }
    return n;
}

double median(std::vector<double>& v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

void report(const char* name, double radius, std::vector<double>& ms, long pixels,
            std::uint64_t sum, bool last) {
    const double m = median(ms);
    std::printf(
        "    {\"case\": \"%s\", \"radius_px\": %.0f, \"median_ms\": %.3f, \"min_ms\": %.3f, "
        "\"max_ms\": %.3f, \"pixels\": %ld, \"ns_per_pixel\": %.1f, "
        "\"checksum\": \"%016llx\"}%s\n",
        name, radius, m, ms.front(), ms.back(), pixels, m * 1.0e6 / pixels,
        static_cast<unsigned long long>(sum), last ? "" : ",");
}

} // namespace

int main(int argc, char** argv) {
    const int repeats = argc > 1 ? std::max(3, std::atoi(argv[1])) : 20;
    const auto grid = Elevation::parse(kEmbeddedElevationData, kEmbeddedElevationSize);
    auto scene = grid ? TerrainScene::create(*grid) : std::nullopt;
    auto created = Framebuffer::create(640, 480);
    if (!grid || !scene || !created) {
        std::fprintf(stderr, "shade-bench: setup failed\n");
        return 1;
    }
    Framebuffer fb = std::move(*created);
    constexpr Rgb565 kBackground = 0x0000;
    const std::uint32_t w = grid->width(), h = grid->height();
    Rgb565 palette[256];
    for (int i = 0; i < 256; ++i) {
        palette[i] = scene->color(static_cast<std::uint8_t>(i));
    }
    // Corner value: mean of the (up to four) cells meeting at the corner.
    std::vector<double> corner_value((w + 1) * (h + 1));
    for (std::uint32_t y = 0; y <= h; ++y) {
        for (std::uint32_t x = 0; x <= w; ++x) {
            double sum = 0;
            int n = 0;
            for (std::uint32_t dy = 0; dy < 2; ++dy) {
                for (std::uint32_t dx = 0; dx < 2; ++dx) {
                    if (x + dx >= 1 && x + dx - 1 < w && y + dy >= 1 && y + dy - 1 < h) {
                        sum += grid->index(x + dx - 1, y + dy - 1);
                        ++n;
                    }
                }
            }
            corner_value[y * (w + 1) + x] = n ? sum / n : 0;
        }
    }

    std::printf("{\n  \"width\": 640, \"height\": 480, \"repeats\": %d, \"grid\": \"%ux%u\",\n"
                "  \"results\": [\n",
                repeats, w, h);
    const double radii[] = {220.0, 440.0};
    for (int r = 0; r < 2; ++r) {
        const auto camera = make_camera({{20.0, 10.0}, 0.0, radii[r], 640, 480});
        if (!camera) {
            return 1;
        }
        // Flat path: project through the scene, then draw every cell as the refine stage does.
        const std::size_t corners = scene->corner_count();
        for (std::size_t i = 0; i < corners; ++i) {
            scene->project_corner(i, *camera);
        }
        for (std::size_t i = 0; i < scene->cell_count(); ++i) {
            scene->bound_cell(i, 480, 480);
        }
        std::vector<double> flat_ms;
        for (int k = 0; k < repeats; ++k) {
            fb.clear(kBackground);
            const std::uint64_t t0 = monotonic_ns();
            for (std::size_t i = 0; i < scene->cell_count(); ++i) {
                scene->draw_cell(fb, i, 0, 480);
            }
            flat_ms.push_back(static_cast<double>(monotonic_ns() - t0) / 1.0e6);
        }
        const long flat_px = covered(fb, kBackground);
        const std::uint64_t flat_sum = checksum(fb);

        // Shaded path: own projection, quads with all four corners in front, visible on screen.
        std::vector<Quad> quads;
        std::vector<double> sx((w + 1) * (h + 1)), sy(sx.size());
        std::vector<char> front(sx.size());
        for (std::uint32_t y = 0; y <= h; ++y) {
            for (std::uint32_t x = 0; x <= w; ++x) {
                const double lat =
                    grid->south_deg() + (grid->north_deg() - grid->south_deg()) * y / h;
                const double lon = grid->west_deg() + (grid->east_deg() - grid->west_deg()) * x / w;
                const Vec3 v = to_camera_space(*camera, to_unit_vector({lat, lon}));
                const ScreenPoint p = to_screen(*camera, v);
                const std::size_t i = y * (w + 1) + x;
                sx[i] = p.x;
                sy[i] = p.y;
                front[i] = v.z >= 0;
            }
        }
        for (std::uint32_t y = 0; y < h; ++y) {
            for (std::uint32_t x = 0; x < w; ++x) {
                const std::size_t c[4] = {y * (w + 1) + x, y * (w + 1) + x + 1,
                                          (y + 1) * (w + 1) + x + 1, (y + 1) * (w + 1) + x};
                if (!(front[c[0]] && front[c[1]] && front[c[2]] && front[c[3]])) {
                    continue;
                }
                Quad q;
                double lo_x = 1e9, hi_x = -1e9, lo_y = 1e9, hi_y = -1e9;
                for (int k = 0; k < 4; ++k) {
                    q.x[k] = sx[c[k]];
                    q.y[k] = sy[c[k]];
                    q.v[k] = corner_value[c[k]];
                    lo_x = std::min(lo_x, q.x[k]);
                    hi_x = std::max(hi_x, q.x[k]);
                    lo_y = std::min(lo_y, q.y[k]);
                    hi_y = std::max(hi_y, q.y[k]);
                }
                if (hi_x < 0 || lo_x > 640 || hi_y < 0 || lo_y > 480) {
                    continue;
                }
                quads.push_back(q);
            }
        }
        std::vector<double> shaded_ms;
        for (int k = 0; k < repeats; ++k) {
            fb.clear(kBackground);
            const std::uint64_t t0 = monotonic_ns();
            for (const Quad& q : quads) {
                fill_triangle_shaded(fb, q.x[0], q.y[0], q.v[0], q.x[1], q.y[1], q.v[1], q.x[2],
                                     q.y[2], q.v[2], palette);
                fill_triangle_shaded(fb, q.x[0], q.y[0], q.v[0], q.x[2], q.y[2], q.v[2], q.x[3],
                                     q.y[3], q.v[3], palette);
            }
            shaded_ms.push_back(static_cast<double>(monotonic_ns() - t0) / 1.0e6);
        }
        const long shaded_px = covered(fb, kBackground);
        const std::uint64_t shaded_sum = checksum(fb);
        std::printf("    {\"cells_drawn_shaded\": %zu},\n", quads.size());
        report("flat_draw_cell", radii[r], flat_ms, flat_px, flat_sum, false);
        report("shaded_two_triangles", radii[r], shaded_ms, shaded_px, shaded_sum, r == 1);
    }
    std::printf("  ]\n}\n");
    return 0;
}
