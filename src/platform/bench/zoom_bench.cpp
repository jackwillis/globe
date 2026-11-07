// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Headless cost of one zoom-preview frame: the scale, the lines on top, and the plain vector frame
// it replaces. 640x480, medians.
#include "clock.hpp"
#include "globe/embedded_atlas.hpp"
#include "globe/render.hpp"
#include "globe/zoom.hpp"
#include <algorithm>
#include <cstdio>
#include <vector>
using namespace globe;
namespace {
double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}
} // namespace
int main() {
    auto atlas = Atlas::parse(kEmbeddedAtlasData, kEmbeddedAtlasSize);
    if (!atlas)
        return 1;
    auto scene = Scene::create(*atlas);
    auto cache = Framebuffer::create(640, 480);
    auto fb = Framebuffer::create(640, 480);
    if (!scene || !cache || !fb)
        return 1;
    cache->clear(rgb565(40, 120, 60));
    std::vector<int> columns;
    std::printf("{\"results\":[\n");
    bool comma = false;
    for (const double factor : {1.1, 1.5, 0.8}) {
        const auto camera = make_camera({{30, 10}, 0, 220 * factor, 640, 480});
        if (!camera)
            return 1;
        std::vector<double> scale, lines, plain;
        for (int r = 0; r < 15; ++r) {
            auto t = monotonic_ns();
            scale_frame(*cache, *fb, camera->center_x, camera->center_y, factor, 0, 1,
                        camera->radius_px, columns);
            scale.push_back(static_cast<double>(monotonic_ns() - t) / 1e6);
            RenderQuality q;
            q.lines_only = true;
            t = monotonic_ns();
            render_globe(*fb, *camera, *scene, kDefaultPalette, Layers{}, true, q);
            lines.push_back(static_cast<double>(monotonic_ns() - t) / 1e6);
            t = monotonic_ns();
            render_globe(*fb, *camera, *scene, kDefaultPalette, Layers{}, true, RenderQuality{});
            plain.push_back(static_cast<double>(monotonic_ns() - t) / 1e6);
        }
        std::printf(
            "%s{\"factor\":%.1f,\"scale_ms\":%.3f,\"lines_only_ms\":%.3f,\"plain_frame_ms\":%.3f}",
            comma ? ",\n" : "", factor, median(scale), median(lines), median(plain));
        comma = true;
    }
    std::printf("\n]}\n");
    return 0;
}
