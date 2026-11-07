// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Experiment: does the final canonical terrain repaint change pixels? Renders random views with
// three repaint modes (canonical, none, limb-clipped cells only) and compares them with the
// canonical frame, for a single job and for the production coarse + preserve-bitmap sequence.
#include "globe/embedded_atlas.hpp"
#include "globe/embedded_elevation.hpp"
#include "globe/embedded_fineelevation.hpp"
#include "globe/embedded_landclasses.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
using namespace globe;
namespace {
unsigned long differences(const std::vector<Rgb565>& a, const std::vector<Rgb565>& b) {
    unsigned long n = 0;
    for (std::size_t i = 0; i < a.size(); ++i)
        n += a[i] != b[i];
    return n;
}
} // namespace
int main() {
    auto atlas = Atlas::parse(kEmbeddedAtlasData, kEmbeddedAtlasSize);
    auto elevation = Elevation::parse(kEmbeddedElevationData, kEmbeddedElevationSize);
    auto fine_elevation = Elevation::parse(kEmbeddedFineElevationData, kEmbeddedFineElevationSize);
    auto classes = LandClasses::decode(kEmbeddedLandclassesData, kEmbeddedLandclassesSize);
    if (!atlas || !elevation || !fine_elevation || !classes)
        return 1;
    auto scene = Scene::create(*atlas);
    auto terrain = TerrainScene::create(*elevation, &*classes);
    auto fine = TerrainScene::create(*fine_elevation, &*classes);
    auto fb = Framebuffer::create(640, 480);
    if (!scene || !terrain || !fine || !fb)
        return 1;
    std::uint64_t rng = 88172645463325252ULL;
    const auto uniform = [&](double lo, double hi) {
        rng ^= rng << 13;
        rng ^= rng >> 7;
        rng ^= rng << 17;
        return lo + (hi - lo) * static_cast<double>(rng >> 11) / 9007199254740992.0;
    };
    const Layers none{false, false, false};
    unsigned views = 0, bad[2][3] = {};
    unsigned long pixels[2][3] = {}, slices[3] = {};
    for (int n = 0; n < 400; ++n) {
        const TerrainStyle style = n % 2 ? TerrainStyle::Relief : TerrainStyle::Map;
        const double radius = std::exp(uniform(std::log(50.), std::log(1500.)));
        const LatLon center{uniform(-89, 89), uniform(-180, 180)};
        const auto camera = make_camera({center, uniform(0, 360), radius, 640, 480});
        if (!camera)
            return 1;
        const ScreenPoint focus{uniform(-200, 840), uniform(-200, 680)};
        std::vector<Rgb565> single[3], two[3];
        for (unsigned mode = 0; mode < 3; ++mode) {
            RenderQuality q{1.5, true, 0, style};
            q.focus = focus;
            q.progressive_lines = false;
            q.repaint_mode = mode;
            RefinementJob job(*camera, *scene, kDefaultPalette, none, q, 1, &*terrain);
            while (job.active())
                job.step(*fb, 1, 256);
            slices[mode] += job.stats().terrain_slices;
            single[mode].assign(fb->data(), fb->data() + 640 * 480);
            // The production sequence: a coarse job, then the fine job preserving its bitmap.
            RenderQuality coarse = q;
            coarse.repaint_mode = 0;
            coarse.terrain_only = true;
            RefinementJob first(*camera, *scene, kDefaultPalette, none, coarse, 1, &*terrain);
            while (first.active())
                first.step(*fb, 1, 256);
            RenderQuality next = q;
            next.preserve_bitmap = true;
            RefinementJob second(*camera, *scene, kDefaultPalette, none, next, 2, &*fine);
            while (second.active())
                second.step(*fb, 2, 256);
            two[mode].assign(fb->data(), fb->data() + 640 * 480);
        }
        ++views;
        for (unsigned mode = 1; mode < 3; ++mode) {
            const auto a = differences(single[0], single[mode]);
            const auto b = differences(two[0], two[mode]);
            bad[0][mode] += a != 0;
            bad[1][mode] += b != 0;
            pixels[0][mode] += a;
            pixels[1][mode] += b;
        }
    }
    const char* names[] = {"canonical", "no repaint", "limb cells only"};
    for (unsigned mode = 1; mode < 3; ++mode)
        std::printf("%-16s single job: %u of %u views differ (%lu px); coarse+fine: %u views "
                    "differ (%lu px)\n",
                    names[mode], bad[0][mode], views, pixels[0][mode], bad[1][mode],
                    pixels[1][mode]);
    for (unsigned mode = 0; mode < 3; ++mode)
        std::printf("%-16s terrain slices over all views (single job): %lu\n", names[mode],
                    slices[mode]);
    return 0;
}
