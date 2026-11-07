// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Headless cost of a terrain refinement with the lines drawn at the end (unpaced) against the lines
// drawn first into a layer and composited after every pass (layer). Same final pixels either way.
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
namespace {
double ms(std::uint64_t ns) {
    return static_cast<double>(ns) / 1e6;
}
double median(std::vector<double> values) {
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}
std::uint64_t checksum(const Framebuffer& fb) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (int y = 0; y < fb.height(); ++y)
        for (int x = 0; x < fb.width(); ++x)
            hash = (hash ^ fb.pixel(x, y)) * 1099511628211ULL;
    return hash;
}
} // namespace
int main(int argc, char** argv) {
    int repeat = 9;
    if (argc != 1 && (argc != 3 || std::strcmp(argv[1], "--repeat") != 0 ||
                      !parse_int_arg(argv[2], 1, 100, repeat)))
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
    std::printf("{\"repeat\":%d,\"scene_bytes\":%lu,\"job_bytes\":%lu,\"results\":[\n", repeat,
                static_cast<unsigned long>(scene->bytes()),
                static_cast<unsigned long>(sizeof(RefinementJob)));
    bool comma = false;
    for (double radius : {220., 440.}) {
        const auto camera = make_camera({{30, 10}, 0, radius, 640, 480});
        if (!camera)
            return 1;
        std::uint64_t reference = 0;
        for (const bool layer : {false, true}) {
            std::vector<double> wall, first;
            double worst = 0;
            std::uint64_t hash = 0;
            for (int r = 0; r < repeat; ++r) {
                RenderQuality q{1.5, true, 0, TerrainStyle::Map};
                q.focus = radius == 220 ? ScreenPoint{320, 240} : ScreenPoint{200, 160};
                q.lines_first = layer;
                const auto start = monotonic_ns();
                RefinementJob job(*camera, *scene, kDefaultPalette, Layers{}, q, 1, &*terrain);
                std::uint64_t first_at = 0;
                while (job.active()) {
                    const auto a = monotonic_ns();
                    const auto step = job.step(*fb, 1);
                    const auto b = monotonic_ns();
                    worst = std::max(worst, ms(b - a));
                    if (step.band_ready && first_at == 0)
                        first_at = b;
                }
                wall.push_back(ms(monotonic_ns() - start));
                first.push_back(first_at != 0 ? ms(first_at - start) : 0);
                const auto current = checksum(*fb);
                if (r != 0 && current != hash)
                    return 1;
                hash = current;
            }
            if (reference == 0)
                reference = hash;
            if (reference != hash)
                return 1;
            std::printf("%s{\"radius\":%.0f,\"mode\":\"%s\",\"wall_median_ms\":%.3f,"
                        "\"first_frame_median_ms\":%.3f,\"worst_step_ms\":%.3f,"
                        "\"checksum\":\"%016llx\"}",
                        comma ? ",\n" : "", radius, layer ? "layer" : "unpaced", median(wall),
                        median(first), worst, static_cast<unsigned long long>(hash));
            comma = true;
            std::fflush(stdout);
        }
    }
    std::printf("\n]}\n");
    return 0;
}
