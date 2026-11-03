// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Headless, including actual requested pacing via nanosleep. No uploads/scanout.
#include "cli.hpp"
#include "clock.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/embedded_landclasses.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
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
void sleep_ms(std::uint32_t delay) {
    timespec requested{static_cast<time_t>(delay / 1000),
                       static_cast<long>(delay % 1000) * 1000000L},
        remaining{};
    while (nanosleep(&requested, &remaining) != 0 && errno == EINTR)
        requested = remaining;
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
    std::vector<LineChunk> metadata(line_chunk_count(*atlas));
    std::vector<double> chunk_build;
    for (int r = 0; r < repeat; ++r) {
        const auto start = monotonic_ns();
        if (!build_line_chunks(*atlas, metadata.data(), metadata.size()))
            return 1;
        chunk_build.push_back(ms(monotonic_ns() - start));
    }
    std::vector<double> setup;
    for (int r = 0; r < repeat; ++r) {
        auto start = monotonic_ns();
        auto trial = Scene::create(*atlas);
        if (!trial)
            return 1;
        setup.push_back(ms(monotonic_ns() - start));
    }
    auto scene = Scene::create(*atlas);
    auto terrain = TerrainScene::create(*elevation, &*classes);
    auto fb = Framebuffer::create(640, 480);
    if (!scene || !terrain || !fb)
        return 1;
    std::printf("{\"repeat\":%d,\"delay_ms\":%u,\"chunks\":%lu,\"chunk_struct_bytes\":%lu,\"line_"
                "workspace_bytes\":%lu,\"scene_bytes\":%lu,\"job_bytes\":%lu,\"scene_create_median_"
                "ms\":%.3f,\"chunk_build_median_ms\":%.3f,\"results\":[\n",
                repeat, kLineRevealDelayMs, static_cast<unsigned long>(scene->line_chunk_count()),
                static_cast<unsigned long>(sizeof(LineChunk)),
                static_cast<unsigned long>(scene->line_workspace_bytes()),
                static_cast<unsigned long>(scene->bytes()),
                static_cast<unsigned long>(sizeof(RefinementJob)), median(setup),
                median(chunk_build));
    bool comma = false;
    for (double radius : {220., 440.}) {
        const auto camera = make_camera({{30, 10}, 0, radius, 640, 480});
        if (!camera)
            return 1;
        std::uint64_t reference = 0;
        for (bool paced : {false, true}) {
            std::vector<double> wall, compute, prepare, lines, repair, first;
            double worst = 0;
            std::uint64_t hash = 0;
            unsigned scheduled = 0, pauses = 0;
            for (int r = 0; r < repeat; ++r) {
                RenderQuality q{1.5, true, 0, TerrainStyle::Map};
                q.focus = radius == 220 ? ScreenPoint{320, 240} : ScreenPoint{200, 160};
                q.progressive_lines = paced;
                const auto start = monotonic_ns();
                RefinementJob job(*camera, *scene, kDefaultPalette, Layers{}, q, 1, &*terrain);
                std::uint64_t slept = 0, prepare_ns = 0, terrain_end = 0, repair_start = 0,
                              first_at = 0;
                unsigned ready = 0;
                scheduled = 0;
                pauses = 0;
                while (job.active()) {
                    auto a = monotonic_ns();
                    const auto step = job.step(*fb, 1);
                    auto b = monotonic_ns();
                    worst = std::max(worst, ms(b - a));
                    if (step.line_prepare)
                        prepare_ns += b - a;
                    if (step.band_ready && ++ready == kRefinePasses)
                        terrain_end = b;
                    if (step.wake_after_ms) {
                        if (!first_at)
                            first_at = b;
                        ++pauses;
                        scheduled += step.wake_after_ms;
                        auto s = monotonic_ns();
                        sleep_ms(step.wake_after_ms);
                        slept += monotonic_ns() - s;
                        if (pauses == kRefinePasses)
                            repair_start = monotonic_ns();
                    }
                }
                const auto end = monotonic_ns();
                wall.push_back(ms(end - start));
                compute.push_back(ms(end - start - slept));
                prepare.push_back(ms(prepare_ns));
                lines.push_back(terrain_end ? ms(end - terrain_end - slept) : 0);
                repair.push_back(repair_start ? ms(end - repair_start) : 0);
                first.push_back(first_at ? ms(first_at - start) : 0);
                auto current = checksum(*fb);
                if (r && current != hash)
                    return 1;
                hash = current;
                if (paced && (pauses != kRefinePasses ||
                              job.stats().line_chunks_prepared != scene->line_chunk_count()))
                    return 1;
            }
            if (!reference)
                reference = hash;
            if (reference != hash)
                return 1;
            std::printf("%s{\"radius\":%.0f,\"paced\":%s,\"wall_median_ms\":%."
                        "3f,\"compute_median_ms\":%.3f,\"prepare_median_ms\":%.3f,\"lines_"
                        "compute_median_ms\":%.3f,\"canonical_median_ms\":%.3f,\"first_reveal_"
                        "median_ms\":%.3f,\"worst_step_ms\":%.3f,\"scheduled_delay_ms\":%u,"
                        "\"pauses\":%u,\"checksum\":\"%016llx\"}",
                        comma ? ",\n" : "", radius, paced ? "true" : "false", median(wall),
                        median(compute), median(prepare), median(lines), median(repair),
                        median(first), worst, scheduled, pauses,
                        static_cast<unsigned long long>(hash));
            comma = true;
            std::fflush(stdout);
        }
    }
    std::printf("\n]}\n");
    return 0;
}
