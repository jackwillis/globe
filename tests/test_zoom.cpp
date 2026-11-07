// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "atlas_util.hpp"
#include "check.hpp"
#include "globe/app.hpp"
#include "globe/refine_runner.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"
#include "globe/zoom.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
using namespace globe;
namespace {
std::vector<Rgb565> pixels(const Framebuffer& fb) {
    return {fb.data(), fb.data() + static_cast<std::size_t>(fb.width()) * fb.height()};
}
void test_scaler() {
    auto src = Framebuffer::create(40, 30);
    auto dst = Framebuffer::create(40, 30);
    CHECK(src && dst);
    if (!src || !dst)
        return;
    for (int y = 0; y < 30; ++y)
        for (int x = 0; x < 40; ++x)
            src->set_pixel(x, y, static_cast<Rgb565>(x * 31 + y * 7 + 1));
    std::vector<int> scratch;
    // Factor 1 is the identity.
    CHECK(scale_frame(*src, *dst, 20, 15, 1.0, 0, 0, 1000, scratch) == 0);
    CHECK(pixels(*src) == pixels(*dst));
    // 2x about the center: each destination pixel reads the nearest-neighbor source pixel,
    // computed here independently (the source point is (20 + (x + .5 - 20) / 2, ...)).
    CHECK(scale_frame(*src, *dst, 20, 15, 2.0, 0, 0, 1000, scratch) == 0);
    for (int y = 0; y < 30; ++y)
        for (int x = 0; x < 40; ++x) {
            const int sx = static_cast<int>(std::floor(20 + (x + 0.5 - 20) / 2));
            const int sy = static_cast<int>(std::floor(15 + (y + 0.5 - 15) / 2));
            CHECK(dst->pixel(x, y) == src->pixel(sx, sy));
        }
    // Zooming out leaves holes outside the old frame; they are filled, and reported only when they
    // fall inside the disc.
    const auto holes = scale_frame(*src, *dst, 20, 15, 0.5, 0xAAAA, 0xBBBB, 30, scratch);
    CHECK(holes > 0 && holes != kInvalidZoom); // the disc reaches the corners
    std::size_t counted = 0;
    for (int y = 0; y < 30; ++y)
        for (int x = 0; x < 40; ++x)
            counted += dst->pixel(x, y) == 0xBBBB;
    CHECK(counted == holes);
    CHECK(dst->pixel(0, 0) == 0xBBBB);
    CHECK(scale_frame(*src, *dst, 20, 15, 0.5, 0xAAAA, 0xBBBB, 3, scratch) == 0);
    CHECK(dst->pixel(0, 0) == 0xAAAA); // outside the disc: the outside color
    // A disc of radius 6 reads sources within 12 of the center: inside the frame.
    CHECK(scale_frame(*src, *dst, 20, 15, 0.5, 0xFFFF, 0xFFFF, 6, scratch) == 0);
    CHECK(dst->pixel(20, 15) == src->pixel(21, 16));
    // Refusals.
    CHECK(scale_frame(*src, *dst, 20, 15, 0.0, 0, 0, 10, scratch) == kInvalidZoom);
    CHECK(scale_frame(*src, *dst, 20, 15, std::nan(""), 0, 0, 10, scratch) == kInvalidZoom);
    auto other = Framebuffer::create(41, 30);
    CHECK(other && scale_frame(*src, *other, 20, 15, 1.0, 0, 0, 10, scratch) == kInvalidZoom);
}
// Refining over a scaled preview (the platform's zoom path) must end on the same pixels as a render
// from scratch at the new radius: the preview is only ever transient.
void test_refine_over_preview() {
    auto ab =
        atlas_util::make_atlas({{1, {{20000, 20000}, {26000, 24000}, {32000, 20000}}}}, "zoom");
    auto atlas = Atlas::parse(ab.data(), ab.size());
    CHECK(atlas);
    if (!atlas)
        return;
    auto scene = Scene::create(*atlas);
    std::FILE* file = std::fopen("data/elevation-1deg.gelv", "rb");
    CHECK(scene && file);
    if (!scene || !file)
        return;
    std::vector<std::uint8_t> raw;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        raw.push_back(static_cast<std::uint8_t>(c));
    std::fclose(file);
    auto elevation = Elevation::parse(raw.data(), raw.size());
    CHECK(elevation);
    if (!elevation)
        return;
    auto terrain = TerrainScene::create(*elevation);
    auto cache = Framebuffer::create(120, 90);
    auto fb = Framebuffer::create(120, 90);
    CHECK(terrain && cache && fb);
    if (!terrain || !cache || !fb)
        return;
    const Layers layers{};
    for (const double ratio : {1.1, 1.5, 0.8}) {
        const double radius = 40;
        const auto before = make_camera({{20, 30}, 10, radius, 120, 90});
        const auto after = make_camera({{20, 30}, 10, radius * ratio, 120, 90});
        CHECK(before && after);
        if (!before || !after)
            return;
        RenderQuality q{1.5, true, 0, TerrainStyle::Map};
        q.lines_first = true;
        q.progressive_lines = false;
        // 1. A settled frame, snapshotted before its lines go on.
        RefinementJob first(*before, *scene, kDefaultPalette, layers, q, 1, &*terrain);
        first.set_snapshot(&*cache);
        while (first.active())
            first.step(*fb, 1, 256);
        CHECK(first.snapshot_taken());
        // 2. The preview: scaled snapshot plus fresh lines.
        std::vector<int> scratch;
        const auto palette = palette_for_style(kDefaultPalette, q.terrain_style);
        const auto holes =
            scale_frame(*cache, *fb, after->center_x, after->center_y, ratio, palette.background,
                        palette.ocean, after->radius_px, scratch);
        CHECK(holes != kInvalidZoom);
        if (ratio < 1) // the disc fits the window at both sizes here
            CHECK(holes == 0);
        RenderQuality lines = q;
        lines.terrain = false;
        lines.lines_only = true;
        render_globe(*fb, *after, *scene, kDefaultPalette, layers, true, lines);
        // 3. Refine over it, keeping the bitmap.
        RenderQuality over = q;
        over.preserve_bitmap = true;
        RefinementJob refine(*after, *scene, kDefaultPalette, layers, over, 2, &*terrain);
        while (refine.active())
            refine.step(*fb, 2, 256);
        const auto got = pixels(*fb);
        // From scratch.
        RefinementJob direct(*after, *scene, kDefaultPalette, layers, q, 3, &*terrain);
        while (direct.active())
            direct.step(*fb, 3, 256);
        CHECK(got == pixels(*fb));
    }
}
// The cache bookkeeping both front ends share.
void test_zoom_preview_cache() {
    auto ab =
        atlas_util::make_atlas({{1, {{20000, 20000}, {26000, 24000}, {32000, 20000}}}}, "zoom");
    auto atlas = Atlas::parse(ab.data(), ab.size());
    std::FILE* file = std::fopen("data/elevation-1deg.gelv", "rb");
    CHECK(atlas && file);
    if (!atlas || !file)
        return;
    std::vector<std::uint8_t> raw;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        raw.push_back(static_cast<std::uint8_t>(c));
    std::fclose(file);
    auto elevation = Elevation::parse(raw.data(), raw.size());
    auto scene = Scene::create(*atlas);
    CHECK(elevation && scene);
    if (!elevation || !scene)
        return;
    auto terrain = TerrainScene::create(*elevation);
    auto fb = Framebuffer::create(120, 90);
    CHECK(terrain && fb);
    if (!terrain || !fb)
        return;
    const auto view = *make_camera({{20, 30}, 10, 40, 120, 90});
    const auto zoomed = *make_camera({{20, 30}, 10, 50, 120, 90});
    const auto turned = *make_camera({{20, 40}, 10, 40, 120, 90});
    const auto palette = palette_for_style(kDefaultPalette, TerrainStyle::Map);
    RenderQuality q{1.5, true, 0, TerrainStyle::Map};
    q.lines_first = true;
    q.progressive_lines = false;
    ZoomPreview zoom;
    CHECK(!zoom.preview(*fb, zoomed, TerrainStyle::Map, palette.background, palette.ocean));
    // A job started and cancelled part-way, with no cache: the half-refined frame becomes the
    // cache.
    RefinementJob partial(view, *scene, kDefaultPalette, Layers{}, q, 1, &*terrain);
    auto* snapshot = zoom.begin_job(view, TerrainStyle::Map, 120, 90);
    CHECK(snapshot != nullptr);
    partial.set_snapshot(snapshot);
    for (int i = 0; i < 40; ++i)
        partial.step(*fb, 1, 64);
    CHECK(partial.active() && !partial.snapshot_taken());
    zoom.after_step(partial);
    CHECK(!zoom.valid());
    zoom.keep_partial(*fb, true, turned); // the view turned: not a view to zoom from
    CHECK(!zoom.valid());
    zoom.keep_partial(*fb, true, zoomed);
    CHECK(zoom.valid() && zoom.baked());
    CHECK(zoom.preview(*fb, zoomed, TerrainStyle::Map, palette.background, palette.ocean));
    CHECK(!zoom.preview(*fb, turned, TerrainStyle::Map, palette.background, palette.ocean));
    CHECK(!zoom.preview(*fb, zoomed, TerrainStyle::Relief, palette.background, palette.ocean));
    // A finished job at the same view replaces it with real terrain: no longer baked.
    RefinementJob full(view, *scene, kDefaultPalette, Layers{}, q, 2, &*terrain);
    full.set_snapshot(zoom.begin_job(view, TerrainStyle::Map, 120, 90));
    while (full.active()) {
        full.step(*fb, 2, 256);
        zoom.after_step(full);
    }
    CHECK(zoom.valid() && !zoom.baked());
    // Outside the factor range there is no preview.
    const auto far = *make_camera({{20, 30}, 10, 40 * 4.5, 120, 90});
    CHECK(!zoom.preview(*fb, far, TerrainStyle::Map, palette.background, palette.ocean));
}
// The refinement lifecycle both front ends share: start, step, cancel, preview, refine over it.
void test_refine_runner() {
    auto ab =
        atlas_util::make_atlas({{1, {{20000, 20000}, {26000, 24000}, {32000, 20000}}}}, "zoom");
    auto atlas = Atlas::parse(ab.data(), ab.size());
    std::FILE* file = std::fopen("data/elevation-1deg.gelv", "rb");
    CHECK(atlas && file);
    if (!atlas || !file)
        return;
    std::vector<std::uint8_t> raw;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        raw.push_back(static_cast<std::uint8_t>(c));
    std::fclose(file);
    auto elevation = Elevation::parse(raw.data(), raw.size());
    auto scene = Scene::create(*atlas);
    CHECK(elevation && scene);
    if (!elevation || !scene)
        return;
    auto terrain = TerrainScene::create(*elevation);
    auto fb = Framebuffer::create(120, 90);
    auto reference = Framebuffer::create(120, 90);
    CHECK(terrain && fb && reference);
    if (!terrain || !fb || !reference)
        return;
    Model m = initial_model(120, 90);
    update(m, RegisterElevation{0, 360, 180});
    RefineRunner runner(*scene);
    CHECK(!runner.active() && runner.job() == nullptr);
    const auto settle = [&](std::uint64_t at) {
        update(m, Tick{at + 250});
        return update(m, Tick{at + 251});
    };
    const auto finish = [&] {
        while (runner.active())
            runner.step(*fb, m);
        while (m.refine_active)
            update(m, RefinementFinished{m.generation, m.refine_level});
    };
    // A first refinement from nothing: no preview yet, and the job hands over its snapshot.
    update(m, Wheel{100, 0});
    CHECK(!runner.render_moving(*fb, m, &*terrain));
    auto fx = settle(100);
    CHECK(fx.refine.has_value());
    runner.start(m, &*fb, &*terrain);
    CHECK(runner.active() && runner.job() != nullptr);
    finish();
    CHECK(!runner.active() && runner.zoom().valid() && !runner.zoom().baked());
    // Zoom: the cache stands in for a render, and the refinement over it ends on the same pixels
    // as one from scratch.
    update(m, Wheel{1000, 2});
    CHECK(runner.render_moving(*fb, m, &*terrain));
    fx = settle(1000);
    CHECK(fx.refine.has_value() &&
          m.refine_level == m.refine_target); // no coarse level to skip here
    runner.start(m, &*fb, &*terrain);
    finish();
    RefinementJob direct(m.camera, *scene, kDefaultPalette, effective_layers(m), quality_for(m), 99,
                         &*terrain);
    while (direct.active())
        direct.step(*reference, 99, 256);
    CHECK(pixels(*fb) == pixels(*reference));
    // Input part-way through a job stops it.
    update(m, Wheel{2000, 1});
    fx = settle(2000);
    CHECK(fx.refine.has_value());
    runner.start(m, &*fb, &*terrain);
    for (int i = 0; i < 5; ++i)
        runner.step(*fb, m);
    CHECK(runner.active());
    update(m, Wheel{3000, 1});
    runner.cancel(&*fb, m.camera);
    CHECK(!runner.active());
    // Turned off, the runner never previews.
    runner.enable_preview(false);
    update(m, Wheel{4000, 1});
    CHECK(!runner.render_moving(*fb, m, &*terrain));
}
} // namespace
int main() {
    test_scaler();
    test_refine_over_preview();
    test_zoom_preview_cache();
    test_refine_runner();
    return TEST_MAIN_RESULT();
}
