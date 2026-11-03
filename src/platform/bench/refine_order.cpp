// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Matched headless experiment: rows, focus-outward bands, and bell-curve cell passes.
#include "cli.hpp"
#include "clock.hpp"
#include "globe/app.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/terrain.hpp"
#include "globe/trace.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace globe;
namespace {
double ms(std::uint64_t ns) {
    return static_cast<double>(ns) / 1e6;
}
double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}
std::uint64_t checksum(const Framebuffer& fb) {
    std::uint64_t h = 14695981039346656037ULL;
    for (int y = 0; y < fb.height(); ++y)
        for (int x = 0; x < fb.width(); ++x)
            h = (h ^ fb.pixel(x, y)) * 1099511628211ULL;
    return h;
}
bool run(const char* name, const Camera& camera, ScreenPoint focus, const Scene& scene,
         TerrainScene& terrain, int repeat, bool comma, Layers layers = {}) {
    auto fb = Framebuffer::create(camera.width, camera.height);
    if (!fb)
        return false;
    std::uint64_t reference = 0;
    for (unsigned o = 0; o < 3; ++o) {
        RenderQuality quality{1.5, true};
        quality.order = static_cast<RefinementOrder>(o);
        quality.focus = focus;
        std::vector<double> total, first, longest, lines;
        std::uint64_t hash = 0;
        for (int r = 0; r < repeat; ++r) {
            render_globe(*fb, camera, scene, kDefaultPalette, layers);
            RefinementJob job(camera, scene, kDefaultPalette, layers, quality, 1, &terrain);
            const auto start = monotonic_ns();
            double first_ms = -1, worst = 0;
            std::uint64_t line_start = 0;
            unsigned ready = 0;
            while (job.active()) {
                const auto a = monotonic_ns();
                const auto step = job.step(*fb, 1, 64);
                const auto b = monotonic_ns();
                worst = std::max(worst, ms(b - a));
                if (step.band_ready) {
                    if (first_ms < 0)
                        first_ms = ms(b - start);
                    if (++ready == kDissolvePasses && o == 2)
                        line_start = b;
                }
            }
            const auto end = monotonic_ns();
            total.push_back(ms(end - start));
            first.push_back(first_ms);
            longest.push_back(worst);
            lines.push_back(line_start ? ms(end - line_start) : 0);
            const auto current = checksum(*fb);
            if (r && hash != current)
                return false;
            hash = current;
        }
        if (o == 0)
            reference = hash;
        if (hash != reference)
            return false;
        std::printf("%s{\"view\":\"%s\",\"order\":\"%s\",\"focus\":[%.1f,%.1f],"
                    "\"first_pass_ms\":%.3f,\"total_ms\":%.3f,\"median_longest_step_ms\":%.3f,"
                    "\"worst_step_ms\":%.3f,"
                    "\"final_lines_ms\":%.3f,\"checksum\":\"%016llx\"}",
                    comma || o ? ",\n" : "", name,
                    o == 0   ? "rows"
                    : o == 1 ? "center"
                             : "dissolve",
                    focus.x, focus.y, median(first), median(total), median(longest),
                    *std::max_element(longest.begin(), longest.end()), median(lines),
                    static_cast<unsigned long long>(hash));
        std::fflush(stdout);
    }
    return true;
}
} // namespace
int main(int argc, char** argv) {
    int repeat = 9;
    const char* trace = "tests/data/bench_drag.txt";
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--repeat") == 0 && i + 1 < argc &&
            parse_int_arg(argv[i + 1], 1, 1000, repeat))
            ++i;
        else if (std::strcmp(argv[i], "--trace") == 0 && i + 1 < argc)
            trace = argv[++i];
        else
            return 2;
    }
    auto atlas = Atlas::parse(kEmbeddedAtlasData, kEmbeddedAtlasSize);
    auto grid = Elevation::parse(kEmbeddedElevationData, kEmbeddedElevationSize);
    if (!atlas || !grid)
        return 1;
    auto scene = Scene::create(*atlas);
    auto terrain = TerrainScene::create(*grid);
    if (!scene || !terrain)
        return 1;
    auto model = initial_model(640, 480);
    auto* file = std::fopen(trace, "r");
    if (!file)
        return 1;
    char line[256];
    while (std::fgets(line, sizeof line, file))
        if (auto msg = parse_message(line))
            update(model, *msg);
    std::fclose(file);
    std::printf("{\"repeat\":%d,\"work_units\":64,\"results\":[\n", repeat);
    const auto c1 = make_camera({{30, 10}, 0, 220, 640, 480});
    const auto c2 = make_camera({{30, 10}, 0, 440, 640, 480});
    if (!c1 || !c2)
        return 1;
    if (!run("profile_1x", *c1, {320, 240}, *scene, *terrain, repeat, false) ||
        !run("profile_2x_offcenter", *c2, {80, 80}, *scene, *terrain, repeat, true) ||
        !run("trace", model.camera, quality_for(model).focus, *scene, *terrain, repeat, true,
             model.layers)) {
        std::fprintf(stderr, "order checksum mismatch or setup failure\n");
        return 1;
    }
    std::printf("\n]}\n");
    return 0;
}
