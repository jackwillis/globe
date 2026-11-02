// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Profiles the settled terrain refine frame: where the time goes between projecting the grid
// corners, bounding cells, scanning cells once per 16-row band, and filling. Platform program
// (uses the clock helper); the library is not modified. Prints one JSON object.
//
//   refine-profile [--repeat N]
#include "cli.hpp"
#include "clock.hpp"
#include "globe/atlas.hpp"
#include "globe/elevation.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/framebuffer.hpp"
#include "globe/projection.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace globe;

namespace {

constexpr Rgb565 kSentinel = 0x0001;

double ms(std::uint64_t ns) {
    return static_cast<double>(ns) / 1.0e6;
}

double median(std::vector<double>& v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

std::uint64_t checksum(const Framebuffer& fb) {
    std::uint64_t h = 14695981039346656037ULL;
    for (int y = 0; y < fb.height(); ++y) {
        for (int x = 0; x < fb.width(); ++x) {
            h = (h ^ fb.pixel(x, y)) * 1099511628211ULL;
        }
    }
    return h;
}

struct Counts {
    std::size_t corners = 0;
    std::size_t cells = 0;
    std::size_t cells_culled = 0;    // all four corners behind the horizon
    std::size_t cell_band_fills = 0; // (cell, band) pairs that pass the band test
    std::size_t corners_front = 0;   // corners with z >= 0
    std::size_t corners_kept = 0;    // corners the hemisphere pre-test would still project
};

struct Result {
    double project_ms, bound_ms, scan_only_ms, scan_and_fill_ms, fill_ms;
    double sum_ms, job_terrain_ms, job_lines_only_ms, project_pretest_ms, single_pass_lines_ms;
    std::uint64_t sum_checksum, job_checksum;
    std::size_t pixels_filled;
    Counts counts;
    int bands;
};

Result profile_case(const Elevation& elevation, const Scene& scene, double radius, int repeat) {
    const int w = 640, h = 480;
    auto fb = Framebuffer::create(w, h);
    auto camera = make_camera({{30.0, 10.0}, 0.0, radius, w, h});
    auto terrain = TerrainScene::create(elevation);
    if (!fb || !camera || !terrain) {
        std::fprintf(stderr, "setup failed\n");
        std::exit(1);
    }
    Result r{};
    r.bands = (h + kRefineBandRows - 1) / kRefineBandRows;
    const std::size_t corners = terrain->corner_count(), cells = terrain->cell_count();
    const std::uint32_t gw = elevation.width(), gh = elevation.height();

    // Independent copy of the world vectors (library keeps its own private) for counting and for
    // the pre-test experiment; built exactly as TerrainScene::create does.
    std::vector<Vec3> world(corners);
    for (std::uint32_t y = 0; y <= gh; ++y) {
        for (std::uint32_t x = 0; x <= gw; ++x) {
            const double lat =
                elevation.south_deg() + (elevation.north_deg() - elevation.south_deg()) * y / gh;
            const double lon =
                elevation.west_deg() + (elevation.east_deg() - elevation.west_deg()) * x / gw;
            world[static_cast<std::size_t>(y) * (gw + 1) + x] = to_unit_vector({lat, lon});
        }
    }
    // Counts (outside timing).
    std::vector<double> cz(corners), cy(corners);
    constexpr double kMargin = 0.0275; // sin(1.58 deg): beyond the 1-degree cell diagonal
    for (std::size_t i = 0; i < corners; ++i) {
        const Vec3 v = to_camera_space(*camera, world[i]);
        cz[i] = v.z;
        cy[i] = to_screen(*camera, v).y;
        r.counts.corners_front += v.z >= 0;
        r.counts.corners_kept += dot(world[i], camera->forward) >= -kMargin;
    }
    r.counts.corners = corners;
    r.counts.cells = cells;
    for (std::size_t c = 0; c < cells; ++c) {
        const std::size_t x = c % gw, y = c / gw, f = y * (gw + 1) + x;
        const std::size_t idx[4] = {f, f + 1, f + gw + 2, f + gw + 1};
        if (cz[idx[0]] < 0 && cz[idx[1]] < 0 && cz[idx[2]] < 0 && cz[idx[3]] < 0) {
            ++r.counts.cells_culled;
            continue;
        }
        const double lo = std::min({cy[idx[0]], cy[idx[1]], cy[idx[2]], cy[idx[3]]});
        const double hi = std::max({cy[idx[0]], cy[idx[1]], cy[idx[2]], cy[idx[3]]});
        const int begin = static_cast<int>(std::clamp(lo, 0.0, double(h))) / kRefineBandRows;
        const int end = std::min(
            r.bands, static_cast<int>(std::clamp(hi, 0.0, double(h))) / kRefineBandRows + 1);
        r.counts.cell_band_fills += static_cast<std::size_t>(std::max(0, end - begin));
    }

    std::vector<double> t_project, t_bound, t_scan, t_full, t_job, t_lines, t_pretest, t_single;
    for (int rep = 0; rep < repeat; ++rep) {
        terrain->claim();
        std::uint64_t t = monotonic_ns();
        for (std::size_t i = 0; i < corners; ++i) {
            terrain->project_corner(i, *camera);
        }
        t_project.push_back(ms(monotonic_ns() - t));

        t = monotonic_ns();
        for (std::size_t c = 0; c < cells; ++c) {
            terrain->bound_cell(c, h, kRefineBandRows);
        }
        t_bound.push_back(ms(monotonic_ns() - t));

        // Scan only: every call is rejected by the band test (band -1), once per band like the
        // refine job's per-band cell loop.
        fb->clear(kSentinel);
        t = monotonic_ns();
        for (int b = 0; b < r.bands; ++b) {
            for (std::size_t c = 0; c < cells; ++c) {
                terrain->draw_cell(*fb, c, -1, kRefineBandRows);
            }
        }
        t_scan.push_back(ms(monotonic_ns() - t));

        // The real per-band loop: all cells per band, rows clipped to the band.
        fb->clear(kSentinel);
        t = monotonic_ns();
        for (int b = 0; b < r.bands; ++b) {
            fb->clip_rows(b * kRefineBandRows, std::min(h, (b + 1) * kRefineBandRows));
            for (std::size_t c = 0; c < cells; ++c) {
                terrain->draw_cell(*fb, c, b, kRefineBandRows);
            }
        }
        t_full.push_back(ms(monotonic_ns() - t));
        fb->clip_rows(0, h);

        // Experiment: hemisphere pre-test before projecting (library unchanged).
        t = monotonic_ns();
        std::size_t projected = 0;
        for (std::size_t i = 0; i < corners; ++i) {
            if (dot(world[i], camera->forward) >= -kMargin) {
                terrain->project_corner(i, *camera);
                ++projected;
            }
        }
        t_pretest.push_back(ms(monotonic_ns() - t));
        if (projected != r.counts.corners_kept) {
            std::fprintf(stderr, "pretest count mismatch\n");
            std::exit(1);
        }
    }
    // Restore a full projection and fill once more for the checksum and the pixel count.
    terrain->claim();
    for (std::size_t i = 0; i < corners; ++i) {
        terrain->project_corner(i, *camera);
    }
    for (std::size_t c = 0; c < cells; ++c) {
        terrain->bound_cell(c, h, kRefineBandRows);
    }
    fb->clear(kSentinel);
    for (int b = 0; b < r.bands; ++b) {
        fb->clip_rows(b * kRefineBandRows, std::min(h, (b + 1) * kRefineBandRows));
        for (std::size_t c = 0; c < cells; ++c) {
            terrain->draw_cell(*fb, c, b, kRefineBandRows);
        }
    }
    fb->clip_rows(0, h);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            r.pixels_filled += fb->pixel(x, y) != kSentinel;
        }
    }
    r.sum_checksum = checksum(*fb);

    // The real refinement job, with and without terrain.
    for (int rep = 0; rep < repeat; ++rep) {
        for (int with_terrain = 1; with_terrain >= 0; --with_terrain) {
            RenderQuality q;
            q.terrain = with_terrain != 0;
            RefinementJob job(*camera, scene, kDefaultPalette, Layers{}, q, 1, &*terrain);
            const std::uint64_t t = monotonic_ns();
            while (job.active()) {
                job.step(*fb, 1);
            }
            (with_terrain ? t_job : t_lines).push_back(ms(monotonic_ns() - t));
        }
    }
    // The same line layers drawn once over the whole frame (the Moving-stage renderer, no
    // terrain): what the per-band job would cost if each arc were drawn once.
    for (int rep = 0; rep < repeat; ++rep) {
        const std::uint64_t t = monotonic_ns();
        render_globe(*fb, *camera, scene, kDefaultPalette, Layers{});
        t_single.push_back(ms(monotonic_ns() - t));
    }
    // The last job run was lines-only; run terrain once more for the checksum.
    {
        RenderQuality q;
        q.terrain = true;
        RefinementJob job(*camera, scene, kDefaultPalette, Layers{}, q, 1, &*terrain);
        while (job.active()) {
            job.step(*fb, 1);
        }
        r.job_checksum = checksum(*fb);
    }

    r.project_ms = median(t_project);
    r.bound_ms = median(t_bound);
    r.scan_only_ms = median(t_scan);
    r.scan_and_fill_ms = median(t_full);
    r.fill_ms = r.scan_and_fill_ms - r.scan_only_ms;
    r.sum_ms = r.project_ms + r.bound_ms + r.scan_and_fill_ms;
    r.job_terrain_ms = median(t_job);
    r.job_lines_only_ms = median(t_lines);
    r.project_pretest_ms = median(t_pretest);
    r.single_pass_lines_ms = median(t_single);
    return r;
}

void print_case(const char* name, double radius, const Result& r, bool last) {
    std::printf("  \"%s\": {\n    \"radius_px\": %.0f, \"bands\": %d,\n", name, radius, r.bands);
    std::printf("    \"counts\": {\"corners\": %zu, \"cells\": %zu, \"cells_culled\": %zu, "
                "\"corners_front\": %zu, \"corners_kept_by_pretest\": %zu, "
                "\"cell_band_fills\": %zu, \"pixels_filled\": %zu},\n",
                r.counts.corners, r.counts.cells, r.counts.cells_culled, r.counts.corners_front,
                r.counts.corners_kept, r.counts.cell_band_fills, r.pixels_filled);
    std::printf("    \"median_ms\": {\"project_all_corners\": %.3f, \"bound_all_cells\": %.3f, "
                "\"cell_scan_only\": %.3f, \"cell_scan_and_fill\": %.3f, "
                "\"fill_derived\": %.3f, \"phase_sum\": %.3f,\n",
                r.project_ms, r.bound_ms, r.scan_only_ms, r.scan_and_fill_ms, r.fill_ms, r.sum_ms);
    std::printf("      \"refine_job_with_terrain\": %.3f, \"refine_job_lines_only\": %.3f, "
                "\"project_with_hemisphere_pretest\": %.3f, \"lines_single_pass\": %.3f},\n",
                r.job_terrain_ms, r.job_lines_only_ms, r.project_pretest_ms,
                r.single_pass_lines_ms);
    std::printf("    \"checksum_phases\": \"%016llx\", \"checksum_job\": \"%016llx\"\n  }%s\n",
                static_cast<unsigned long long>(r.sum_checksum),
                static_cast<unsigned long long>(r.job_checksum), last ? "" : ",");
}

} // namespace

int main(int argc, char** argv) {
    int repeat = 9;
    if (argc != 1 && (argc != 3 || std::strcmp(argv[1], "--repeat") != 0 ||
                      !parse_int_arg(argv[2], 1, 1000, repeat))) {
        std::fprintf(stderr, "usage: refine-profile [--repeat 1..1000]\n");
        return 2;
    }
    AtlasError err = AtlasError::Ok;
    const auto atlas = Atlas::parse(kEmbeddedAtlasData, kEmbeddedAtlasSize, &err);
    const auto elevation = Elevation::parse(kEmbeddedElevationData, kEmbeddedElevationSize);
    if (!atlas || !elevation) {
        std::fprintf(stderr, "refine-profile: bad embedded data\n");
        return 1;
    }
    const auto scene = Scene::create(*atlas);
    if (!scene) {
        std::fprintf(stderr, "refine-profile: scene failed\n");
        return 1;
    }
    std::printf("{\n  \"width\": 640, \"height\": 480, \"repeat\": %d,\n", repeat);
    print_case("zoom_1x", 220.0, profile_case(*elevation, *scene, 220.0, repeat), false);
    print_case("zoom_2x", 440.0, profile_case(*elevation, *scene, 440.0, repeat), true);
    std::printf("}\n");
    return 0;
}
