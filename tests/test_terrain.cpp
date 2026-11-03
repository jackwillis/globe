// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 Jack Willis
#include "atlas_util.hpp"
#include "check.hpp"
#include "globe/elevation.hpp"
#include "globe/render.hpp"
#include "globe/terrain.hpp"
#include <algorithm>
#include <cmath>
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
    CHECK(terrain->bytes() == TerrainScene::required_bytes(*elevation));
    CHECK(terrain->color(192, TerrainStyle::CelShaded) != terrain->color(192));
    CHECK(terrain_color(1000, -80, TerrainStyle::Hypsometric) == rgb565(255, 255, 255));
    CHECK(terrain_color(-1000, -80, TerrainStyle::Hypsometric) != rgb565(255, 255, 255));
    auto base = Framebuffer::create(80, 61);
    CHECK(base);
    if (base) {
        const auto view = *make_camera({{0, 0}, 0, 25, 80, 61});
        base->clear(0);
        terrain->draw_base(*base, view);
        CHECK(base->pixel(40, 30) == kMovingLand);
        CHECK(base->pixel(0, 0) == 0);
        const auto polar = *make_camera({{-80, 0}, 0, 25, 80, 61});
        base->clear(0);
        terrain->draw_base(*base, polar);
        CHECK(base->pixel(40, 30) == rgb565(255, 255, 255));
    }
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
    for (auto style : {TerrainStyle::Hypsometric, TerrainStyle::CelShaded}) {
        const auto polar = *make_camera({{-80, 0}, 0, 25, 80, 61});
        RefinementJob ice(polar, *scene, kDefaultPalette, {false, false, false},
                          {1.5, true, 0, style}, 2, &*terrain);
        while (ice.active())
            ice.step(*fb, 2, 256);
        CHECK(fb->pixel(40, 30) == rgb565(255, 255, 255));
    }
    RefinementJob moving(camera, *scene, kDefaultPalette, {false, false, false}, {3, false}, 2,
                         &*terrain);
    while (moving.active())
        moving.step(*fb, 2, 256);
    CHECK(fb->pixel(44, 34) == kDefaultPalette.ocean);
    {
        // The moving frame is plain vectors by default; the gray base only appears when asked for.
        RenderQuality plain;
        render_globe(*fb, camera, *scene, kDefaultPalette, {false, false, false}, true, plain,
                     &*terrain);
        CHECK(fb->pixel(44, 34) == kDefaultPalette.ocean);
        RenderQuality gray;
        gray.gray_base = true;
        render_globe(*fb, camera, *scene, kDefaultPalette, {false, false, false}, true, gray,
                     &*terrain);
        CHECK(fb->pixel(44, 34) == kMovingLand);
    }
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
            fb->clear(7);
            split->draw_base(*fb, view);
            CHECK(fb->pixel(40, 30) == (longitude < 0 ? 7 : kMovingLand));
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
void test_appearance() {
    auto bytes = land_grid();
    for (unsigned y = 0; y < 180; ++y)
        for (unsigned x = 0; x < 360; ++x)
            bytes[552 + y * 360 + x] = (x % 2) ? 160 : 224;
    store_le32(bytes.data() + 36,
               crc32_extend(crc32(bytes.data(), 36), bytes.data() + 40, bytes.size() - 40));
    const auto grid = Elevation::parse(bytes.data(), bytes.size());
    CHECK(grid);
    if (!grid)
        return;
    auto terrain = TerrainScene::create(*grid);
    auto ab = atlas_util::make_atlas({{1, {{0, 0}, {1, 1}}}}, "appearance");
    auto atlas = Atlas::parse(ab.data(), ab.size());
    CHECK(atlas);
    if (!atlas)
        return;
    auto scene = Scene::create(*atlas);
    auto fb = Framebuffer::create(100, 80);
    CHECK(scene && fb && terrain);
    if (!scene || !fb || !terrain)
        return;
    const auto camera = *make_camera({{0, 0}, 0, 35, 100, 80});
    const auto render = [&](TerrainStyle style, bool smooth) {
        RefinementJob job(camera, *scene, kDefaultPalette, {false, false, false},
                          {1.5, true, 0, style, smooth}, 10, &*terrain);
        while (job.active())
            job.step(*fb, 10, 256);
        return std::vector<Rgb565>(fb->data(), fb->data() + 100 * 80);
    };
    fb->clear(123);
    auto flat = render(TerrainStyle::Hypsometric, false);
    auto smooth = render(TerrainStyle::Hypsometric, true);
    CHECK(flat != smooth);
    auto cel = render(TerrainStyle::CelShaded, true);
    CHECK(cel != smooth);
    terrain->draw_base(*fb, camera);
    CHECK(render(TerrainStyle::Hypsometric, true) ==
          smooth); // settled image independent of moving base
    fb->clear(456);
    CHECK(render(TerrainStyle::Hypsometric, true) == smooth);
}
// Pass of a cell computed independently of the production code, from the screen position of the
// cell center (Rectangle, Circle) or the angle of its center on the sphere (Globe).
void check_assignment(TerrainScene& terrain, const Camera& camera, RefinementOrder order,
                      ScreenPoint focus) {
    const auto plan = make_pass_plan(camera, order, focus);
    const double w = camera.width, h = camera.height;
    const auto& grid = terrain.grid();
    unsigned seen = 0;
    for (std::size_t i = 0; i < terrain.cell_count(); ++i) {
        terrain.bound_cell(i, camera.height, kRefineBandRows);
        terrain.assign_pass(i, plan);
        const unsigned pass = terrain.cell_pass(i);
        if (pass == 255)
            continue;
        CHECK(pass < kRefinePasses);
        ++seen;
        const double lon = grid.west_deg() + (grid.east_deg() - grid.west_deg()) *
                                                 (static_cast<double>(i % grid.width()) + 0.5) /
                                                 grid.width();
        const double lat = grid.south_deg() + (grid.north_deg() - grid.south_deg()) *
                                                  (static_cast<double>(i / grid.width()) + 0.5) /
                                                  grid.height();
        const auto v = to_camera_space(camera, to_unit_vector({lat, lon}));
        const auto p = to_screen(camera, v);
        double distance = 0.0, step = 1.0;
        if (order == RefinementOrder::Globe) {
            // The plan averages the four corner dots; the oracle does the same from lat/lon.
            const Vec3 f = plan.focus_world;
            const double half_w = (grid.east_deg() - grid.west_deg()) / grid.width() / 2.0;
            const double half_h = (grid.north_deg() - grid.south_deg()) / grid.height() / 2.0;
            double mean = 0.0;
            for (double sy : {-1.0, 1.0})
                for (double sx : {-1.0, 1.0})
                    mean += dot(f, to_unit_vector({lat + sy * half_h, lon + sx * half_w})) * 0.25;
            distance = std::acos(std::clamp(mean, -1.0, 1.0)) / kDegToRad;
            step = 180.0 / kRefinePasses;
        } else if (order == RefinementOrder::Circle) {
            const double rx = focus.x - camera.center_x, ry = focus.y - camera.center_y;
            distance = std::hypot(p.x - plan.focus.x, p.y - plan.focus.y);
            step = (std::hypot(rx, ry) + camera.radius_px) / kRefinePasses;
        } else {
            const double dx = std::max(std::fabs(plan.focus.x), std::fabs(w - plan.focus.x));
            const double dy = std::max(std::fabs(plan.focus.y), std::fabs(h - plan.focus.y));
            // Scaled by width * height, as the plan does.
            distance =
                std::max(std::fabs(p.x - plan.focus.x) * h, std::fabs(p.y - plan.focus.y) * w);
            step = std::max(dx * h, dy * w) / kRefinePasses;
        }
        const double expect = distance / step;
        // Skip cells within a hair of a ring boundary (rounding of the oracle's own arithmetic).
        const double tolerance = 0.01;
        if (std::fabs(expect - std::round(expect)) < tolerance)
            continue;
        CHECK(pass == std::min<unsigned>(kRefinePasses - 1, static_cast<unsigned>(expect)));
    }
    CHECK(seen > 50);
}
void test_orders() {
    auto bytes = land_grid();
    // Contrasting neighbors expose order-dependent edge ownership and clipped limb overlaps.
    for (std::size_t i = 552; i < bytes.size(); ++i)
        bytes[i] = static_cast<std::uint8_t>((i * 73) % 256);
    store_le32(bytes.data() + 36,
               crc32_extend(crc32(bytes.data(), 36), bytes.data() + 40, bytes.size() - 40));
    auto grid = Elevation::parse(bytes.data(), bytes.size());
    auto ab = atlas_util::make_atlas({{2, {{32768, 5461}, {32768, 27307}}}}, "orders");
    auto atlas = Atlas::parse(ab.data(), ab.size());
    CHECK(grid && atlas);
    if (!grid || !atlas)
        return;
    auto terrain = TerrainScene::create(*grid);
    auto scene = Scene::create(*atlas);
    auto fb = Framebuffer::create(100, 81);
    CHECK(terrain && scene && fb);
    if (!terrain || !scene || !fb)
        return;
    const RefinementOrder orders[] = {RefinementOrder::Rectangle, RefinementOrder::Circle,
                                      RefinementOrder::Globe};
    for (double radius : {35.0, 90.0}) {
        auto camera = *make_camera({{30, 10}, 13, radius, 100, 81});
        std::vector<Rgb565> reference;
        for (auto order : orders) {
            for (ScreenPoint focus : {ScreenPoint{50, 40}, ScreenPoint{5, 7}, ScreenPoint{95, 75},
                                      ScreenPoint{-100, 200}, ScreenPoint{15, 40}}) {
                RenderQuality q{1.5, true};
                q.order = order;
                q.focus = focus;
                fb->clear(0xABCD);
                RefinementJob job(camera, *scene, kDefaultPalette, Layers{}, q, 1, &*terrain);
                unsigned steps = 0, ready = 0;
                std::uint32_t before_last = 0;
                while (job.active() && ++steps < 200000) {
                    const auto step = job.step(*fb, 1, 256);
                    CHECK(step.work_units <= 256);
                    ready += step.band_ready;
                    if (step.band_ready && ready == kRefinePasses - 1)
                        before_last = job.stats().terrain_slices;
                }
                CHECK(!job.active() && ready > 0);
                // The last pass repaints every visible cell, once per 16-row slice.
                std::uint32_t slices = 0;
                for (std::size_t i = 0; i < terrain->cell_count(); ++i) {
                    const auto b = terrain->cell_bands(i);
                    slices += static_cast<unsigned>(b.end - b.first);
                }
                CHECK(job.stats().terrain_slices - before_last == slices);
                auto pixels = std::vector<Rgb565>(fb->data(), fb->data() + 100 * 81);
                if (reference.empty())
                    reference = pixels;
                CHECK(pixels == reference); // identical for every order and every focus
                // Generation cancellation must preserve every pixel, even at a pass boundary.
                RefinementJob cancelled(camera, *scene, kDefaultPalette, Layers{}, q, 4, &*terrain);
                while (cancelled.active() && !cancelled.step(*fb, 4, 256).band_ready) {
                }
                auto before = std::vector<Rgb565>(fb->data(), fb->data() + 100 * 81);
                CHECK(cancelled.step(*fb, 5, 256).work_units == 0);
                CHECK(!cancelled.active());
                CHECK(std::equal(before.begin(), before.end(), fb->data()));
            }
        }
    }
    // Rings: each cell lands in the pass given by its distance from the focus, per shape.
    // A large globe so cells are several pixels wide and a one-corner error moves a cell across a
    // ring boundary.
    const auto camera = *make_camera({{0, 0}, 0, 200, 100, 81});
    for (std::size_t i = 0; i < terrain->corner_count(); ++i)
        terrain->project_corner(i, camera);
    for (auto order : orders)
        for (ScreenPoint focus : {ScreenPoint{50, 40}, ScreenPoint{20, 30}, ScreenPoint{70, 55}})
            check_assignment(*terrain, camera, order, focus);
    // Globe out takes the view center when the focus is off the disc.
    {
        const auto off = make_pass_plan(camera, RefinementOrder::Globe, {-100, 200});
        const auto center = make_pass_plan(camera, RefinementOrder::Globe, {50, 40.5});
        CHECK(dot(off.focus_world, center.focus_world) > 1.0 - 1e-12);
        CHECK(off.focus.x == camera.center_x && off.focus.y == camera.center_y);
        const auto on = make_pass_plan(camera, RefinementOrder::Globe, {90, 40});
        CHECK(dot(on.focus_world, center.focus_world) < 0.99);
    }
    // A non-finite focus is the view center, for every shape.
    for (auto order : orders) {
        const auto bad = make_pass_plan(camera, order, {std::nan(""), 3});
        CHECK(bad.focus.x == camera.center_x && bad.focus.y == camera.center_y);
    }
    // Rectangle: on a non-square viewport the left/right edges and the top/bottom edges are
    // reached in the same pass (normalized distance), and the corners in the last pass.
    {
        const auto wide = *make_camera({{0, 0}, 0, 50, 160, 120});
        const auto plan = make_pass_plan(wide, RefinementOrder::Rectangle, {80, 60});
        const unsigned left = plan.pass_at(0, 60), right = plan.pass_at(159.9, 60);
        const unsigned top = plan.pass_at(80, 0), bottom = plan.pass_at(80, 119.9);
        CHECK(left == right && top == bottom && left == top);
        CHECK(plan.pass_at(0, 0) == kRefinePasses - 1 &&
              plan.pass_at(160, 120) == kRefinePasses - 1);
        CHECK(plan.pass_at(80, 60) == 0);
        // Same fraction of the way to the edges, same pass: this is what normalizing buys.
        for (double t : {0.1, 0.3, 0.45}) {
            const unsigned across = plan.pass_at(80 - t * 160, 60);
            CHECK(across == plan.pass_at(80 + t * 160, 60));
            CHECK(across == plan.pass_at(80, 60 - t * 120));
            CHECK(across == plan.pass_at(80, 60 + t * 120));
            CHECK(across > 0 && across < kRefinePasses - 1);
        }
        // Off-center focus: edges are reached in order of their normalized distance.
        const auto skew = make_pass_plan(wide, RefinementOrder::Rectangle, {40, 30});
        CHECK(skew.pass_at(0, 30) <= skew.pass_at(159.9, 30));
        CHECK(skew.pass_at(40, 0) <= skew.pass_at(40, 119.9));
        // The farthest viewport corner is the last pass for any focus.
        CHECK(skew.pass_at(159.9, 119.9) == kRefinePasses - 1);
    }
}
void test_landclass_looks() {
    CHECK(landclass_color(100, LandClass::Desert, TerrainStyle::Natural) !=
          landclass_color(100, LandClass::Forest, TerrainStyle::Natural));
    CHECK(landclass_color(100, LandClass::Desert, TerrainStyle::Map) !=
          landclass_color(100, LandClass::Forest, TerrainStyle::Map));
    CHECK(landclass_color(-100, LandClass::Desert, TerrainStyle::Map) ==
          landclass_color(-100, LandClass::Forest, TerrainStyle::Map));
    CHECK(terrain_color(2000, -80, TerrainStyle::Natural) == rgb565(255, 255, 255));
    CHECK(terrain_color(2000, 75, TerrainStyle::Natural) != rgb565(255, 255, 255));
    auto bytes = land_grid();
    auto grid = Elevation::parse(bytes.data(), bytes.size());
    CHECK(grid);
    std::FILE* file = std::fopen("data/landclasses-1deg.ggrd", "rb");
    CHECK(file);
    if (!grid || !file)
        return;
    std::vector<std::uint8_t> raw;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        raw.push_back(static_cast<std::uint8_t>(c));
    std::fclose(file);
    auto classes = LandClasses::decode(raw.data(), raw.size());
    CHECK(classes);
    if (!classes)
        return;
    auto terrain = TerrainScene::create(*grid, &*classes);
    CHECK(terrain);
    if (!terrain)
        return;
    auto ab = atlas_util::make_atlas({{1, {{0, 0}, {1, 1}}}}, "class looks");
    auto atlas = Atlas::parse(ab.data(), ab.size());
    CHECK(atlas);
    if (!atlas)
        return;
    auto scene = Scene::create(*atlas);
    auto fb = Framebuffer::create(100, 80);
    CHECK(scene && fb);
    if (!scene || !fb)
        return;
    for (auto style : {TerrainStyle::Map, TerrainStyle::Natural}) {
        for (auto point : {LatLon{23.5, 13.5}, LatLon{-3.5, -62.5}}) {
            auto camera = make_camera({point, 0, 35, 100, 80});
            CHECK(camera);
            if (!camera)
                return;
            RefinementJob job(*camera, *scene, kDefaultPalette, {false, false, false},
                              {1.5, true, 0, style}, 1, &*terrain);
            while (job.active())
                job.step(*fb, 1, 256);
            // Uniform 1,000 m heights make the exact center-pixel palette value an independent
            // oracle.
            const auto kind = point.lat_deg > 0 ? LandClass::Desert : LandClass::Forest;
            CHECK(fb->pixel(50, 40) == landclass_color(grid->value(192), kind, style));
        }
    }
}
} // namespace
int main() {
    test_orders();
    test_landclass_looks();
    test_terrain();
    test_appearance();
    return TEST_MAIN_RESULT();
}
