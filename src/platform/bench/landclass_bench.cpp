// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Compare each look on matched 1x/2x views; load/decode/allocation excluded from render timing.
#include "cli.hpp"
#include "clock.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/embedded_landclasses.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace globe;
int main(int argc, char** argv) {
    int repeat = 9;
    if (argc != 1 && (argc != 3 || std::strcmp(argv[1], "--repeat") != 0 ||
                      !parse_int_arg(argv[2], 1, 1000, repeat)))
        return 2;
    auto atlas = Atlas::parse(kEmbeddedAtlasData, kEmbeddedAtlasSize);
    auto elevation = Elevation::parse(kEmbeddedElevationData, kEmbeddedElevationSize);
    auto classes = LandClasses::decode(kEmbeddedLandclassesData, kEmbeddedLandclassesSize);
    if (!atlas || !elevation || !classes)
        return 1;
    auto scene = Scene::create(*atlas);
    auto terrain = TerrainScene::create(*elevation, &*classes);
    auto fb = Framebuffer::create(640, 480);
    if (!scene || !terrain || !fb)
        return 1;
    std::printf("{\"repeat\":%d,\"pack_bytes\":%lu,\"class_bytes\":%lu,\"terrain_bytes\":%lu,"
                "\"results\":[\n",
                repeat, static_cast<unsigned long>(kEmbeddedLandclassesSize),
                static_cast<unsigned long>(LandClasses::bytes()),
                static_cast<unsigned long>(terrain->bytes()));
    bool comma = false;
    for (auto style : {TerrainStyle::Map, TerrainStyle::Relief})
        for (double radius : {220., 440.}) {
            const auto camera = make_camera({{30, 10}, 0, radius, 640, 480});
            if (!camera)
                return 1;
            std::vector<double> times;
            double worst = 0;
            std::uint64_t hash = 0;
            for (int r = 0; r < repeat; ++r) {
                RenderQuality q{1.5, true, 0, style};
                q.focus = {320, 240};
                RefinementJob job(*camera, *scene, kDefaultPalette, Layers{}, q, 1, &*terrain);
                auto start = monotonic_ns();
                while (job.active()) {
                    auto a = monotonic_ns();
                    job.step(*fb, 1);
                    worst = std::max(worst, static_cast<double>(monotonic_ns() - a) / 1e6);
                }
                times.push_back(static_cast<double>(monotonic_ns() - start) / 1e6);
                std::uint64_t h = 14695981039346656037ULL;
                for (int y = 0; y < 480; ++y)
                    for (int x = 0; x < 640; ++x)
                        h = (h ^ fb->pixel(x, y)) * 1099511628211ULL;
                if (r && hash != h)
                    return 1;
                hash = h;
            }
            std::sort(times.begin(), times.end());
            const char* name = style == TerrainStyle::Map ? "Map" : "Relief";
            std::printf("%s{\"look\":\"%s\",\"radius\":%.0f,\"median_ms\":%.3f,\"worst_step_ms\":%."
                        "3f,\"checksum\":\"%016llx\"}",
                        comma ? ",\n" : "", name, radius, times[times.size() / 2], worst,
                        static_cast<unsigned long long>(hash));
            comma = true;
            std::fflush(stdout);
        }
    std::printf("\n]}\n");
    return 0;
}
