// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "atlas_util.hpp"
#include "check.hpp"
#include "globe/elevation.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace globe;
using atlas_util::Bytes;
namespace {
Bytes land_grid() {
    std::FILE* f = std::fopen("data/elevation-1deg.gelv", "rb");
    CHECK(f);
    if (!f)
        return {};
    Bytes b;
    int c;
    while ((c = std::fgetc(f)) != EOF)
        b.push_back(static_cast<std::uint8_t>(c));
    std::fclose(f);
    std::fill(b.begin() + 552, b.end(), 192);
    store_le32(b.data() + 36, crc32_extend(crc32(b.data(), 36), b.data() + 40, b.size() - 40));
    return b;
}
void test_terrain() {
    auto bytes = land_grid();
    const auto elevation = Elevation::parse(bytes.data(), bytes.size());
    CHECK(elevation);
    if (!elevation)
        return;
    auto terrain = TerrainScene::create(*elevation);
    CHECK(terrain);
    if (!terrain)
        return;
    CHECK(terrain->bytes() <= kTerrainCacheBudget);
    CHECK(terrain->color(0) != terrain->color(128));
    CHECK(terrain->color(128) != terrain->color(255));
    auto atlas_bytes =
        atlas_util::make_atlas({{2, {{32768, 5461}, {32768, 27307}}}}, "terrain test");
    auto atlas = Atlas::parse(atlas_bytes.data(), atlas_bytes.size());
    CHECK(atlas);
    if (!atlas)
        return;
    auto scene = Scene::create(*atlas);
    auto fb = Framebuffer::create(80, 61);
    CHECK(scene && fb);
    if (!scene || !fb)
        return;
    const auto camera = *make_camera({{0, 0}, 0, 25, 80, 61});
    const Layers layers{false, false, true};
    fb->clear(0xAAAA);
    RefinementJob job(camera, *scene, kDefaultPalette, layers, {1.5, true}, 1, &*terrain);
    CHECK(job.step(*fb, 1, 1).work_units == 1);
    CHECK(fb->pixel(40, 30) == 0xAAAA); // preparing cached projection must not blank the old view
    unsigned steps = 0;
    while (job.active() && ++steps < 20000)
        CHECK(job.step(*fb, 1, 256).work_units <= 256);
    CHECK(!job.active());
    CHECK(fb->pixel(44, 34) == terrain->color(192));
    CHECK(fb->pixel(40, 30) == rgb565(0, 0, 0)); // borders above terrain are black
    CHECK(fb->pixel(0, 0) == kDefaultPalette.background);
    RefinementJob moving(camera, *scene, kDefaultPalette, {false, false, false}, {3, false}, 2,
                         &*terrain);
    while (moving.active())
        moving.step(*fb, 2, 256);
    CHECK(fb->pixel(44, 34) == kDefaultPalette.ocean);
    // Opposite hemispheres use different colors; a hidden cell must never paint through.
    for (unsigned y = 0; y < 180; ++y)
        for (unsigned x = 0; x < 360; ++x)
            bytes[552 + y * 360 + x] = x < 180 ? 64 : 192;
    store_le32(bytes.data() + 36,
               crc32_extend(crc32(bytes.data(), 36), bytes.data() + 40, bytes.size() - 40));
    auto split = TerrainScene::create(*elevation);
    CHECK(split);
    if (split) {
        for (double longitude : {-90.0, 90.0}) {
            const auto view = *make_camera({{0, longitude}, 0, 25, 80, 61});
            RefinementJob halves(view, *scene, kDefaultPalette, {false, false, false}, {1.5, true},
                                 4, &*split);
            while (halves.active())
                halves.step(*fb, 4, 256);
            CHECK(fb->pixel(44, 34) == split->color(longitude < 0 ? 64 : 192));
        }
    }
    // A deliberately coarse grid exposes back-facing corners intruding several pixels at
    // the limb. Checking only the center would miss a broken horizon clipping branch.
    auto coarse_bytes = bytes;
    coarse_bytes.resize(552 + 12 * 6);
    store_le32(coarse_bytes.data() + 8, 12);
    store_le32(coarse_bytes.data() + 12, 6);
    for (unsigned y = 0; y < 6; ++y)
        for (unsigned x = 0; x < 12; ++x)
            coarse_bytes[552 + y * 12 + x] = x < 6 ? 64 : 192;
    store_le32(coarse_bytes.data() + 36,
               crc32_extend(crc32(coarse_bytes.data(), 36), coarse_bytes.data() + 40,
                            coarse_bytes.size() - 40));
    auto coarse_grid = Elevation::parse(coarse_bytes.data(), coarse_bytes.size());
    CHECK(coarse_grid);
    if (coarse_grid) {
        auto coarse = TerrainScene::create(*coarse_grid);
        CHECK(coarse);
        if (coarse) {
            const auto view = *make_camera({{10, -90}, 0, 25, 80, 61});
            RefinementJob limb(view, *scene, kDefaultPalette, {false, false, false}, {1.5, true}, 5,
                               &*coarse);
            while (limb.active())
                limb.step(*fb, 5, 256);
            unsigned wrong = 0;
            for (int y = 0; y < 61; ++y)
                for (int x = 0; x < 80; ++x)
                    if ((x + 0.5 - 40) * (x + 0.5 - 40) + (y + 0.5 - 30.5) * (y + 0.5 - 30.5) <
                            22.5 * 22.5 &&
                        fb->pixel(x, y) == coarse->color(192))
                        ++wrong;
            CHECK(wrong == 0);
        }
    }
    RefinementJob old(camera, *scene, kDefaultPalette, layers, {1.5, true}, 3, &*terrain);
    old.step(*fb, 3, 1);
    RefinementJob replacement(camera, *scene, kDefaultPalette, layers, {1.5, true}, 3, &*terrain);
    CHECK(old.step(*fb, 3, 1).work_units == 0 && !old.active()); // workspace ownership also cancels
}
} // namespace
int main() {
    test_terrain();
    return TEST_MAIN_RESULT();
}
