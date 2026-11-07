// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
// Checks the terrain repaint strategy: renders random views with each Repaint mode and compares
// them with the Reference order (every unclipped cell, then every clipped cell, in index order),
// for a single job and for the production coarse + preserve-bitmap sequence. Expect LimbPass to
// match the reference exactly; the legacy full repaint and rings-only differ at a few limb pixels.
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
    unsigned views = 0, bad[2][4] = {};
    unsigned long pixels[2][4] = {}, slices[4] = {};
    for (int n = 0; n < 600; ++n) {
        const TerrainStyle style = n % 2 ? TerrainStyle::Relief : TerrainStyle::Map;
        const double radius = std::exp(uniform(std::log(50.), std::log(1500.)));
        const LatLon center{uniform(-89, 89), uniform(-180, 180)};
        const double heading = uniform(0, 360);
        const auto camera = make_camera({center, heading, radius, 640, 480});
        if (!camera)
            return 1;
        const ScreenPoint focus{uniform(-200, 840), uniform(-200, 680)};
        std::vector<Rgb565> single[4], two[4];
        for (unsigned mode = 0; mode < 4; ++mode) {
            RenderQuality q{1.5, true, 0, style};
            q.focus = focus;
            q.repaint = static_cast<TerrainRepaint>(mode);
            RefinementJob job(*camera, *scene, kDefaultPalette, none, q, 1,
                              n % 4 < 2 ? &*terrain : &*fine);
            while (job.active())
                job.step(*fb, 1, 256);
            slices[mode] += job.stats().terrain_slices;
            single[mode].assign(fb->data(), fb->data() + std::size_t{640} * 480);
            // The production sequence: a coarse job, then the fine job preserving its bitmap.
            RenderQuality coarse = q;
            coarse.repaint = static_cast<TerrainRepaint>(mode);
            coarse.terrain_only = true;
            RefinementJob first(*camera, *scene, kDefaultPalette, none, coarse, 1, &*terrain);
            while (first.active())
                first.step(*fb, 1, 256);
            RenderQuality next = q;
            next.preserve_bitmap = true;
            RefinementJob second(*camera, *scene, kDefaultPalette, none, next, 2, &*fine);
            while (second.active())
                second.step(*fb, 2, 256);
            two[mode].assign(fb->data(), fb->data() + std::size_t{640} * 480);
        }
        ++views;
        for (unsigned mode = 0; mode < 3; ++mode) {
            const auto a = differences(single[3], single[mode]);
            const auto b = differences(two[3], two[mode]);
            bad[0][mode] += a != 0;
            bad[1][mode] += b != 0;
            if (b != 0 && mode == 0)
                for (std::size_t i = 0; i < two[3].size(); ++i)
                    if (two[3][i] != two[0][i])
                        std::printf(
                            "two-level diff px %zu,%zu r=%.3f lat=%.6f lon=%.6f heading=%.6f "
                            "focus=%.3f,%.3f %s canonical %04x skip %04x\n",
                            i % 640, i / 640, radius, center.lat_deg, center.lon_deg, heading,
                            focus.x, focus.y, style == TerrainStyle::Map ? "map" : "relief",
                            two[3][i], two[2][i]);
            pixels[0][mode] += a;
            pixels[1][mode] += b;
        }
    }
    const char* names[] = {"limb pass", "full (legacy)", "rings only", "reference"};
    for (unsigned mode = 0; mode < 3; ++mode)
        std::printf("%-16s single job: %u of %u views differ from the reference (%lu px); "
                    "coarse+fine: %u views "
                    "differ (%lu px)\n",
                    names[mode], bad[0][mode], views, pixels[0][mode], bad[1][mode],
                    pixels[1][mode]);
    for (unsigned mode = 0; mode < 4; ++mode)
        std::printf("%-16s terrain slices over all views (single job): %lu\n", names[mode],
                    slices[mode]);
    return 0;
}
