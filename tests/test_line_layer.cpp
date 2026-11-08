// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "atlas_util.hpp"
#include "check.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
using namespace globe;
int main() {
    std::vector<atlas_util::Poly> polys;
    for (unsigned layer = 1; layer <= 2; ++layer) {
        atlas_util::Poly poly{static_cast<std::uint8_t>(layer), {}};
        for (int i = 0; i < 65; ++i) {
            double lon = -170 + i * 5.;
            double lat = (layer == 1 ? 30. : -30.) * std::sin(i * .17);
            poly.points.emplace_back(static_cast<std::uint16_t>((lon + 180) * 65536 / 360),
                                     static_cast<std::uint16_t>((lat + 90) * 65536 / 360));
        }
        polys.push_back(poly);
    }
    auto bytes = atlas_util::make_atlas(polys, "line reveal");
    auto atlas = Atlas::parse(bytes.data(), bytes.size());
    CHECK(atlas);
    if (!atlas)
        return 1;
    auto scene = Scene::create(*atlas);
    CHECK(scene);
    if (!scene)
        return 1;
    auto* file = std::fopen("data/elevation-1deg.gelv", "rb");
    CHECK(file);
    if (!file)
        return 1;
    std::vector<std::uint8_t> raw;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        raw.push_back(static_cast<std::uint8_t>(c));
    std::fclose(file);
    auto elevation = Elevation::parse(raw.data(), raw.size());
    CHECK(elevation);
    if (!elevation)
        return 1;
    auto terrain = TerrainScene::create(*elevation);
    auto fb = Framebuffer::create(100, 81);
    CHECK(terrain && fb);
    if (!terrain || !fb)
        return 1;
    for (double radius : {35., 90.}) {
        const auto camera = make_camera({{25, 10}, 13, radius, 100, 81});
        CHECK(camera);
        if (!camera)
            return 1;
        for (ScreenPoint focus : {ScreenPoint{50, 40}, ScreenPoint{5, 10}, ScreenPoint{-20, 100}}) {
            RenderQuality q{1.5, true, 0, TerrainStyle::Map};
            q.focus = focus;
            RefinementJob reference(*camera, *scene, kDefaultPalette, Layers{}, q, 1, &*terrain);
            while (reference.active())
                reference.step(*fb, 1, 256);
            auto expected = std::vector<Rgb565>(fb->data(), fb->data() + 100 * 81);
            // Line layer: the lines are on screen before any terrain, every later frame keeps them,
            // and the settled image is identical.
            q.lines_first = true;
            RefinementJob layered(*camera, *scene, kDefaultPalette, Layers{}, q, 5, &*terrain);
            const auto palette = palette_for_style(kDefaultPalette, q.terrain_style);
            unsigned frames = 0;
            double last_progress = 0;
            while (layered.active()) {
                const auto step = layered.step(*fb, 5, 64);
                CHECK(step.work_units <= 64);
                const double progress = layered.progress();
                CHECK(progress >= last_progress && progress <= 1.0); // a bar never runs backwards
                last_progress = progress;
                if (!step.band_ready)
                    continue;
                if (++frames == 1) {
                    unsigned lines = 0;
                    for (int i = 0; i < 100 * 81; ++i) {
                        const auto v = fb->data()[i];
                        if (v != palette.background && v != palette.ocean) {
                            ++lines;
                            CHECK(v == expected[static_cast<std::size_t>(i)]);
                        }
                    }
                    CHECK(lines > 50);
                }
            }
            CHECK(frames == kRefinePasses + 3); // lines, 16 rings, the limb pass, the end
            CHECK(last_progress > 0.9 && layered.progress() == 1.0);
            CHECK(std::equal(expected.begin(), expected.end(), fb->data()));
            // The same with a line-pixel list too small for the view (the composite scans the layer
            // instead) and with an exactly sufficient one: identical pixels either way.
            for (const std::size_t limit : {std::size_t{1}, std::size_t{100000}}) {
                q.line_list_limit = limit;
                RefinementJob listed(*camera, *scene, kDefaultPalette, Layers{}, q, 7, &*terrain);
                unsigned shown = 0;
                while (listed.active()) {
                    shown += listed.step(*fb, 7, 64).band_ready;
                }
                CHECK(shown == kRefinePasses + 3);
                CHECK(std::equal(expected.begin(), expected.end(), fb->data()));
            }
            q.line_list_limit = 0;
            // A generation change stops a job part-way and leaves the frame as it was.
            RefinementJob cancelled(*camera, *scene, kDefaultPalette, Layers{}, q, 3, &*terrain);
            for (int i = 0; i < 40; ++i)
                cancelled.step(*fb, 3, 64);
            auto before = std::vector<Rgb565>(fb->data(), fb->data() + 100 * 81);
            CHECK(cancelled.active());
            CHECK(cancelled.step(*fb, 4).work_units == 0);
            CHECK(!cancelled.active());
            CHECK(std::equal(before.begin(), before.end(), fb->data()));
        }
    }
    // Jobs on independent terrain caches do not disturb each other; a second job on the same cache
    // claims it and the first one stops.
    auto other = TerrainScene::create(*elevation);
    CHECK(other);
    auto view = make_camera({{25, 10}, 0, 35, 100, 81});
    CHECK(view);
    if (other && view) {
        RefinementJob old(*view, *scene, kDefaultPalette, Layers{}, {1.5, true}, 9, &*terrain);
        old.step(*fb, 9, 1);
        RefinementJob independent(*view, *scene, kDefaultPalette, Layers{}, {1.5, true}, 9,
                                  &*other);
        CHECK(old.step(*fb, 9, 1).work_units == 1 && old.active());
        RefinementJob same(*view, *scene, kDefaultPalette, Layers{}, {1.5, true}, 9, &*terrain);
        CHECK(old.step(*fb, 9, 1).work_units == 0 && !old.active());
    }
    if (view) {
        RefinementJob outline(*view, *scene, kDefaultPalette, Layers{}, {}, 10);
        while (outline.active()) {
            const auto step = outline.step(*fb, 10);
        }
    }
    return TEST_MAIN_RESULT();
}
