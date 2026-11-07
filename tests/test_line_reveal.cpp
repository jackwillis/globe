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
    std::vector<LineChunk> metadata(scene->line_chunk_count());
    CHECK(!build_line_chunks(*atlas, nullptr, metadata.size()));
    CHECK(!build_line_chunks(*atlas, metadata.data(), metadata.size() - 1));
    CHECK(build_line_chunks(*atlas, metadata.data(), metadata.size()));
    // Every original segment is covered exactly once by the immutable chunk table.
    std::vector<unsigned> covered(atlas->point_count() + 1337);
    for (std::size_t i = 0; i < scene->line_chunk_count(); ++i) {
        const auto chunk = scene->line_chunk(i);
        CHECK(chunk.count >= 2 && chunk.count <= 16);
        for (unsigned j = 1; j < chunk.count; ++j) {
            CHECK(chunk.first + j < covered.size());
            if (chunk.first + j < covered.size())
                ++covered[chunk.first + j - 1];
        }
    }
    for (unsigned i = 0; i < covered.size(); ++i) {
        bool last = i < 130         ? (i % 65 == 64)
                    : i < 130 + 605 ? ((i - 130) % 121 == 120)
                                    : ((i - 130 - 605) % 61 == 60);
        CHECK(covered[i] == (last ? 0U : 1U));
    }
    // The maximum v1 point count must not wrap a 16-bit unsigned loop on classic targets.
    auto large_bytes = atlas_util::make_atlas(
        {{1, std::vector<atlas_util::Point>(65535, {32000, 16000})}}, "maximum chunk count");
    auto large = Atlas::parse(large_bytes.data(), large_bytes.size());
    CHECK(large);
    if (large) {
        std::vector<LineChunk> chunks(line_chunk_count(*large));
        CHECK(build_line_chunks(*large, chunks.data(), chunks.size()));
        std::size_t edges = 0;
        for (const auto& c : chunks)
            if (c.layer == LineLayer::Coastline)
                edges += c.count - 1;
        CHECK(edges == 65534);
    }
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
            q.progressive_lines = false;
            RefinementJob reference(*camera, *scene, kDefaultPalette, Layers{}, q, 1, &*terrain);
            while (reference.active())
                CHECK(reference.step(*fb, 1, 256).wake_after_ms == 0);
            auto expected = std::vector<Rgb565>(fb->data(), fb->data() + 100 * 81);
            q.progressive_lines = true;
            RefinementJob reveal(*camera, *scene, kDefaultPalette, Layers{}, q, 2, &*terrain);
            unsigned pauses = 0;
            bool checked = false;
            while (reveal.active()) {
                const auto step = reveal.step(*fb, 2, 64);
                CHECK(step.work_units <= 64);
                if (step.line_prepare &&
                    reveal.stats().line_chunks_prepared == scene->line_chunk_count()) {
                    checked = true;
                    const auto plan = make_pass_plan(*camera, focus);
                    for (std::size_t i = 0; i < scene->line_chunk_count(); ++i) {
                        const auto chunk = scene->line_chunk(i);
                        const unsigned middle = chunk.first + (chunk.count - 1) / 2;
                        auto mid = scene->vertices()[middle];
                        if (chunk.count % 2 == 0)
                            mid = (mid + scene->vertices()[middle + 1]) * .5;
                        const auto p = to_screen(*camera, to_camera_space(*camera, mid));
                        const unsigned pass = plan.pass_at(p.x, p.y);
                        CHECK(scene->line_chunk_pass(i) == pass);
                    }
                }
                if (step.wake_after_ms) {
                    CHECK(step.band_ready);
                    CHECK(step.wake_after_ms == kLineRevealDelayMs);
                    ++pauses;
                }
            }
            CHECK(checked && pauses == kRefinePasses);
            CHECK(reveal.stats().line_chunks_prepared == scene->line_chunk_count());
            CHECK(std::equal(expected.begin(), expected.end(), fb->data()));
            // Line layer: the lines are on screen before any terrain, every later frame keeps them,
            // and the settled image is identical.
            q.progressive_lines = false;
            q.lines_first = true;
            RefinementJob layered(*camera, *scene, kDefaultPalette, Layers{}, q, 5, &*terrain);
            const auto palette = palette_for_style(kDefaultPalette, q.terrain_style);
            unsigned frames = 0;
            while (layered.active()) {
                const auto step = layered.step(*fb, 5, 64);
                CHECK(step.work_units <= 64);
                CHECK(step.wake_after_ms == 0);
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
            CHECK(std::equal(expected.begin(), expected.end(), fb->data()));
            q.lines_first = false;
            q.progressive_lines = true;
            RefinementJob cancelled(*camera, *scene, kDefaultPalette, Layers{}, q, 3, &*terrain);
            while (cancelled.active() && !cancelled.step(*fb, 3).wake_after_ms) {
            }
            auto before = std::vector<Rgb565>(fb->data(), fb->data() + 100 * 81);
            CHECK(cancelled.active());
            CHECK(cancelled.step(*fb, 4).work_units == 0);
            CHECK(!cancelled.active());
            CHECK(std::equal(before.begin(), before.end(), fb->data()));
        }
    }
    // Independent terrain caches still share the Scene line-pass workspace.
    auto other = TerrainScene::create(*elevation);
    CHECK(other);
    auto view = make_camera({{25, 10}, 0, 35, 100, 81});
    CHECK(view);
    if (other && view) {
        RefinementJob old(*view, *scene, kDefaultPalette, Layers{}, {1.5, true}, 9, &*terrain);
        old.step(*fb, 9, 1);
        RefinementJob next(*view, *scene, kDefaultPalette, Layers{}, {1.5, true}, 9, &*other);
        CHECK(old.step(*fb, 9, 1).work_units == 0 && !old.active());
    }
    if (view) {
        RefinementJob outline(*view, *scene, kDefaultPalette, Layers{}, {}, 10);
        while (outline.active()) {
            const auto step = outline.step(*fb, 10);
            CHECK(step.wake_after_ms == 0 && !step.line_prepare);
        }
    }
    return TEST_MAIN_RESULT();
}
