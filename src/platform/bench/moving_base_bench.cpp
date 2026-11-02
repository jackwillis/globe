// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Isolated gray land-mask cost, excluding the background/ocean clear and line layers.
#include "clock.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/raster.hpp"
#include "globe/terrain.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
using namespace globe;
int main() {
    const auto elevation = Elevation::parse(kEmbeddedElevationData, kEmbeddedElevationSize);
    if (!elevation)
        return 1;
    auto terrain = TerrainScene::create(*elevation);
    auto fb = Framebuffer::create(640, 480);
    if (!terrain || !fb)
        return 1;
    std::printf("{\"samples\":9,\"viewport\":\"640x480\",\"cases\":[");
    bool comma = false;
    for (double radius : {220.0, 440.0}) {
        const auto camera = make_camera({{30, 10}, 0, radius, 640, 480});
        if (!camera)
            return 1;
        std::array<double, 9> times{};
        for (auto& sample : times) {
            fb->clear(0);
            fill_disc(*fb, 320, 240, radius, rgb565(12, 54, 112));
            const auto start = monotonic_ns();
            terrain->draw_base(*fb, *camera);
            sample = static_cast<double>(monotonic_ns() - start) / 1e6;
        }
        std::sort(times.begin(), times.end());
        std::uint64_t hash = 14695981039346656037ULL;
        for (int y = 0; y < 480; ++y)
            for (int x = 0; x < 640; ++x)
                hash = (hash ^ fb->pixel(x, y)) * 1099511628211ULL;
        const auto high = static_cast<unsigned>(hash >> 32), low = static_cast<unsigned>(hash);
        std::printf("%s{\"radius\":%.0f,\"median_ms\":%.4f,\"min_ms\":%.4f,\"max_ms\":%.4f,"
                    "\"checksum\":\"%08x%08x\"}",
                    comma ? "," : "", radius, times[4], times.front(), times.back(), high, low);
        comma = true;
    }
    std::printf("]}\n");
}
