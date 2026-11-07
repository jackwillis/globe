// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "atlas_util.hpp"
#include "check.hpp"
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
} // namespace
int main() {
    test_scaler();
    test_refine_over_preview();
    return TEST_MAIN_RESULT();
}
